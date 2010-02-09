#include <assert.h>
#include <process.h>
#include <vector>
#include <set>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageManager.h"


//////////////////////////////////////////////////////////////////////////
static const int PREFETCH_RANGE = 5;
// when zooming, we don't generate image that is smaller than that
static const int MIN_VIEW_WIDTH = 160;
static const int MIN_VIEW_HEIGHT = 120;

void CImageManager::_SetIndexNoLock (int index) {
	if (m_currIndex != index) {
		XLTRACE(_T("--== change index from %d to %d ==--\n"), m_currIndex, index);
		if ((int)m_currIndex < index) {
			m_direction = FORWARD;
		} else {
			m_direction = BACKWARD;
		}

		// check marginal condition
		if (m_currIndex == 0 && index == m_images.size() - 1) {
			m_direction = BACKWARD;
		} else if (m_currIndex == m_images.size() - 1 && index == 0) {
			m_direction = FORWARD;
		}

		if (m_currIndex == (xl::uint)-1) {
			m_direction = FORWARD;
		}

		xl::uint lastIndex = m_currIndex;
		m_currIndex = index;
		m_indexChanged = true;

		if (lastIndex != (xl::uint)-1) {
			assert(m_images[lastIndex]->getRealSizeImage() == NULL);
			// m_images[lastIndex]->clearRealSize();
		}

		// start prefetch first
		_BeginPrefetch();

		_TriggerEvent(EVT_INDEX_CHANGED, &index);
	}
}


/**
 * the order is (for current index is N):
 * forward: [N + 1, N - 1, N + 2, N + 3, ... N - 2, N - 3, ...]
 * backward: [N -1, N + 1, N - 2, N - 3, ... N + 2, N + 3, ...]
 */
#define IM_CHECK_INDEX(index, count) \
do {\
	if (index < 0) {\
		index += count;\
	} else if (index >= count) {\
		index -= count;\
	}\
} while(0)
#define IM_INSERT_INDEX(container, index, getcount, count) \
do {\
	if (getcount == count - 1) {\
		return;\
	}\
	IM_CHECK_INDEX(index, count);\
	container.push_back(index);\
	getcount ++;\
} while (0)
void CImageManager::_GetPrefetchIndexes (_Indexes &indexes, int currIndex, int count, DIRECTION direction, int range) {
	indexes.reserve(2 * range);

	int offset = direction == FORWARD ? 1 : -1;
	int getcount = 0;

	int index = currIndex + offset;
	IM_INSERT_INDEX(indexes, index, getcount, count); // N + 1

	index = currIndex - offset;
	IM_INSERT_INDEX(indexes, index, getcount, count); // N - 1

	int r = range - 1;
	for (int i = 0; i < r; ++ i) {
		index = currIndex + offset * (i + 2);
		IM_INSERT_INDEX(indexes, index, getcount, count); // N + 1
	}

	offset = -offset;
	for (int i = 0; i < r; ++ i) {
		index = currIndex + offset * (i + 2);
		IM_INSERT_INDEX(indexes, index, getcount, count); // N + 1
	}

	if (count > 2 * range + 1) {
		assert(indexes.size() == 2 * range);
	}
}
#undef IM_CHECK_INDEX
#undef IM_INSERT_INDEX


//////////////////////////////////////////////////////////////////////////
// thread
unsigned __stdcall CImageManager::_PrefetchThread (void *param) {
	CImageManager *pThis = (CImageManager *)param;
	assert(pThis != NULL);
	HANDLE hEvent = pThis->m_hPrefetchEvent;
	for (;;) {
		::WaitForSingleObject(hEvent, INFINITE);
		if (pThis->m_exiting) {
			break;
		}

		xl::CScopeLock lock(pThis);
		CSize szPrefetch = pThis->m_szPrefetch;
		if (szPrefetch.cx <= 0 || szPrefetch.cy <= 0 || pThis->m_images.size() == 0) {
			continue;
		}

		int currIndex = pThis->getCurrIndex();
		size_t count = pThis->getImageCount();
		CDisplayImagePtr displayImage = pThis->getImage(currIndex);
		assert(displayImage->getRealSizeImage() == NULL);
		pThis->m_indexChanged = false;
		lock.unlock();

		// 1. load current
		assert(currIndex >= 0 && currIndex < (int)count);
		if (displayImage->loadRealSize(pThis)) {
			lock.lock(pThis);
			if (pThis->shouldCancel()) {
				continue;
			}
			int indexLoaded = currIndex;
			pThis->_TriggerEvent(EVT_IMAGE_LOADED, &indexLoaded);
			displayImage->clearRealSize(); // transport the ownership to the subscriber (CImageView)
			lock.unlock();
		} else {
			if (pThis->shouldCancel()) {
				continue;
			}
		}

		// 2. prefetch
		// 2.1 lock, and get the image ptrs
		lock.lock(pThis);
		if (pThis->shouldCancel()) {
			continue;
		} // make sure the index and count is not changed
		_Indexes indexes;
		_Images images;
		pThis->_GetPrefetchIndexes(indexes, currIndex, count, pThis->m_direction, PREFETCH_RANGE);
		images.reserve(indexes.size());
		for (_Indexes::iterator it = indexes.begin(); it != indexes.end(); ++ it) {
			images.push_back(pThis->getImage(*it));
		}

		// 2.2 clear the useless zoomed images, and unlock
		// note that 2.1 and 2.2 should be fast enough for a lock operation
		for (xl::uint i = 0; i < pThis->m_images.size(); ++ i) {
			bool removed = true;
			if (i == currIndex) {
				removed = false;
			} else {
				for (xl::uint j = 0; j < indexes.size(); ++ j) {
					if (i == indexes[j]) {
						removed = false;
					}
				}
			}
			if (removed) {
				pThis->m_images[i]->clearZoomed();
			}
		}
		lock.unlock();

		// 2.3 load the zoomed images
		for (_Images::iterator it = images.begin(); it != images.end() && !pThis->shouldCancel(); ++ it) {
			CDisplayImagePtr image = *it;
			if (image->getZoomedImage() == NULL) {
				if (image->getRealSizeImage() == NULL) {
					image->loadRealSize(pThis);
				}

				if (image->getRealSizeImage() != NULL && !pThis->shouldCancel()) {
					CSize szImage = image->getRealSize();
					CSize szArea = pThis->m_szPrefetch;
					CSize szZoom = CImage::getSuitableSize(szArea, szImage, true);

					image->loadZoomed(szZoom.cx, szZoom.cy, pThis);

					lock.lock(pThis);
					if (!pThis->shouldCancel()) {
						int indexZoomed = indexes[std::distance(images.begin(), it)];
						pThis->_TriggerEvent(EVT_IMAGE_ZOOMED, &indexZoomed);
					}
					lock.unlock();
				}

				image->clearRealSize();
				XLTRACE(_T("prefetched: %s\n"), image->getFileName().c_str());
			}
		} // the for loop
	}

	return 0;
}

void CImageManager::_CreateThreads () {
	assert(m_hPrefetchThread == INVALID_HANDLE_VALUE);
	assert(m_hPrefetchEvent == NULL);

	xl::tchar name[MAX_PATH];

	xl::CScopeLock lock(this);
	_stprintf_s(name, MAX_PATH, 
		_T("Local\\xlview::ImageManger::PrefetchEvent [created at %d]"), ::GetTickCount());
	m_hPrefetchEvent = ::CreateEvent(NULL, FALSE, FALSE, name);
	assert(m_hPrefetchEvent != NULL);
	m_hPrefetchThread = (HANDLE)_beginthreadex(NULL, 0, _PrefetchThread, this, 0, NULL);
	assert(m_hPrefetchThread != INVALID_HANDLE_VALUE);
}

void CImageManager::_TerminateThreads () {
	assert(m_hPrefetchEvent != NULL);
	assert(m_hPrefetchThread != INVALID_HANDLE_VALUE);

	bool exiting = m_exiting;
	m_exiting = true;
	::SetEvent(m_hPrefetchEvent);
	if (::WaitForSingleObject(m_hPrefetchThread, 3000) != WAIT_OBJECT_0) {
		::TerminateThread(m_hPrefetchThread, -1);
		XLTRACE(_T("** Thread prefetch does not exit normally\n"));
	}

	CloseHandle(m_hPrefetchEvent);
	CloseHandle(m_hPrefetchThread);
	m_hPrefetchEvent = NULL;
	m_hPrefetchThread = INVALID_HANDLE_VALUE;

	m_exiting = exiting;
}

void CImageManager::_BeginPrefetch () {
	// xl::CScopeLock lock(this);
	if (m_hPrefetchEvent != NULL) {
		::SetEvent(m_hPrefetchEvent);
	} else {
		assert(false);
	}
}


//////////////////////////////////////////////////////////////////////////

CImageManager::CImageManager ()
	: m_currIndex((xl::uint)-1)
	, m_direction(CImageManager::FORWARD)
	, m_szPrefetch(MIN_VIEW_WIDTH, MIN_VIEW_HEIGHT)
	, m_exiting(false)
	, m_indexChanged(false)
	, m_hPrefetchThread(INVALID_HANDLE_VALUE)
	, m_hPrefetchEvent(NULL)
{
	_CreateThreads();
}

CImageManager::~CImageManager () {
	m_exiting = true;
	_TerminateThreads();
}

int CImageManager::getCurrIndex () const {
	return m_currIndex;
}

int CImageManager::getImageCount () const {
	xl::CScopeLock lock(this);
	return (int)m_images.size();
}

bool CImageManager::setFile (const xl::tstring &file) {
	xl::CScopeLock lock(this);

	assert(m_images.size() == 0);
	xl::tstring fileName = xl::file_get_name(file);
	m_directory = xl::file_get_directory(file);
	m_directory += _T("\\");
	xl::tstring pattern = m_directory + _T("*.*");
	xl::CTimerLogger logger(_T("Searching images cost"));

	int new_index = -1;

	// find the files
	WIN32_FIND_DATA wfd;
	memset(&wfd, 0, sizeof(wfd));
	HANDLE hFind = ::FindFirstFile(pattern, &wfd);
	if (hFind != INVALID_HANDLE_VALUE) {
		CImageLoader *pLoader = CImageLoader::getInstance();
		do {
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				continue; // skip directory
			}

			xl::tstring name = m_directory + wfd.cFileName;
			if (pLoader->isFileSupported(name)) {
				if (_tcsicmp(wfd.cFileName, fileName) == 0) {
					new_index = (int)m_images.size();
				}
				m_images.push_back(CDisplayImagePtr(new CDisplayImage(name)));
			}
		} while (::FindNextFile(hFind, &wfd));
		::FindClose(hFind);

		xl::trace(_T("get %d files\n"), m_images.size());
	}

	if (m_images.size() == 0) {
		::MessageBox(NULL, _T("Can not find any image files"), 0, MB_OK);
		return false;
	}

	size_t count = m_images.size();
	_TriggerEvent(EVT_FILELIST_READY, &count);

	if (new_index == -1 && count > 0) {
		new_index = 0;
	}
	_SetIndexNoLock(new_index);
	return true;
}

void CImageManager::setIndex (int index) {
	xl::CScopeLock lock(this);
	_SetIndexNoLock(index);
}

CDisplayImagePtr CImageManager::getImage (int index) {
	xl::CScopeLock lock(this);
	assert(index >= 0 && index < getImageCount());
	CDisplayImagePtr image = m_images[index];
	lock.unlock();
	return image;
}

void CImageManager::onViewSizeChanged (CRect rc) {
	xl::CScopeLock lock(this);
	CSize sz(rc.Width(), rc.Height());
	if (sz.cx < MIN_VIEW_WIDTH) {
		sz.cx = MIN_VIEW_WIDTH;
	}
	if (sz.cy < MIN_VIEW_HEIGHT) {
		sz.cy = MIN_VIEW_HEIGHT;
	}

	if (m_szPrefetch == sz) {
		return;
	}

	m_szPrefetch = sz;
}

bool CImageManager::shouldCancel () {
	return m_indexChanged || m_exiting;
}