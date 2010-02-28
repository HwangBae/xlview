#include <assert.h>
#include <process.h>
#include <vector>
#include <set>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageManager.h"


//////////////////////////////////////////////////////////////////////////
static const int PREFETCH_RANGE = 5;

void CImageManager::_SetIndexNoLock (int index) {
	assert(getLockLevel() > 0);
	if (m_currIndex != index) {
		XLTRACE(_T("--== change index from %d to %d ==--\n"), m_currIndex, index);
		if ((int)m_currIndex < index) {
			m_direction = FORWARD;
		} else {
			m_direction = BACKWARD;
		}

		// check marginal condition
		if (m_currIndex == 0 && index == m_cachedImages.size() - 1) {
			m_direction = BACKWARD;
		} else if (m_currIndex == m_cachedImages.size() - 1 && index == 0) {
			m_direction = FORWARD;
		}

		if (m_currIndex == (xl::uint)-1) {
			m_direction = FORWARD;
		}

		xl::uint lastIndex = m_currIndex;
		m_currIndex = index;

		// start prefetch first
		_BeginLoad();
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
// callbacks
namespace {
	class CLoadingCallback : public xl::ILongTimeRunCallback {
	protected:
		int m_currIndex;
		CImageManager *m_pManager;
	public:
		CLoadingCallback (int index, CImageManager *pManager) 
			: m_currIndex(index)
			, m_pManager(pManager)
		{
			assert(m_pManager != NULL);
		}

		virtual bool shouldStop () const {
			return m_currIndex != m_pManager->getCurrIndex() || m_pManager->isExiting();
		}
	};

	class CZoomingCallback : public CLoadingCallback {
		CSize m_szZoom;
	public:
		CZoomingCallback (CSize szZoom, int index, CImageManager *pManager)
			: m_szZoom(szZoom)
			, CLoadingCallback(index, pManager)
		{
		}

		virtual bool shouldStop () const {
			return m_szZoom != m_pManager->getPrefetchSize() 
				|| CLoadingCallback::shouldStop();
		}
	};
}

//////////////////////////////////////////////////////////////////////////
// thread
unsigned __stdcall CImageManager::_LoadThread (void *param) {
	CImageManager *pThis = (CImageManager *)param;
	assert(pThis != NULL);
	HANDLE hEvent = pThis->m_hEvents[THREAD_LOAD];

	CImageLoader *pImageLoader = CImageLoader::getInstance();

	for (;;) {
		::WaitForSingleObject(hEvent, INFINITE);
		if (pThis->m_exiting) {
			break;
		}

		xl::CScopeLock lock(pThis);
		xl::tstring fileName = pThis->getCurrentFileName();
		CLoadingCallback callback(pThis->getCurrIndex(), pThis);
		lock.unlock();

		xl::CTimerLogger logger(false, _T("Load  %s cost"), fileName.c_str());
		CImagePtr image = pImageLoader->load(fileName, &callback);
		if (image == NULL) {
			assert(callback.shouldStop());
		} else {
			lock.lock(pThis);
			pThis->_TriggerEvent(EVT_IMAGE_LOADED, &image);
			image.reset();
			lock.unlock();
		}
	}

	return 0;
}


unsigned __stdcall CImageManager::_PrefetchThread (void *param) {
	CImageManager *pThis = (CImageManager *)param;
	assert(pThis != NULL);
	HANDLE hEvent = pThis->m_hEvents[THREAD_PREFETCH];
	for (;;) {
		::WaitForSingleObject(hEvent, INFINITE);
		if (pThis->m_exiting) {
			break;
		}

		xl::CScopeLock lock(pThis);
		CSize szPrefetch = pThis->m_szPrefetch;
		if (szPrefetch.cx <= 0 || szPrefetch.cy <= 0 || pThis->m_cachedImages.size() == 0) {
			continue;
		}

		int currIndex = pThis->getCurrIndex();
		size_t count = pThis->getImageCount();

		// 1. prefetch
		// 1.1 get the image Ptrs
		_Indexes indexes;
		_CachedImages images;
		pThis->_GetPrefetchIndexes(indexes, currIndex, count, pThis->m_direction, PREFETCH_RANGE);
		images.reserve(indexes.size());
		for (_Indexes::iterator it = indexes.begin(); it != indexes.end(); ++ it) {
			images.push_back(pThis->m_cachedImages[*it]);
		}

		// 1.2 clear the useless zoomed images, and unlock
		// note that 2.1 and 2.2 should be fast enough for a lock operation
		for (xl::uint i = 0; i < pThis->m_cachedImages.size(); ++ i) {
			bool removed = true;
			if (i == currIndex) {
				removed = false;
			} else {
				// TODO: use std::find instead
				for (xl::uint j = 0; j < indexes.size(); ++ j) {
					if (i == indexes[j]) {
						removed = false;
					}
				}
			}
			if (removed) {
				pThis->m_cachedImages[i]->clear(false); // remain the thumbnail
			}
		}

		CZoomingCallback callback(szPrefetch, currIndex, pThis);
		lock.unlock();

		// 1.3 load the zoomed images
		for (_CachedImages::iterator it = images.begin();
			it != images.end() && !callback.shouldStop(); 
			++ it)
		{
			CCachedImagePtr image = *it;
			image->loadSuitable(szPrefetch, &callback);
		}

		// TODO: 1.4 load the thumbnail
		if (callback.shouldStop()) {
			continue;
		}
	}

	return 0;
}


void CImageManager::_BeginLoad () {
	_RunThread(THREAD_LOAD);
}

void CImageManager::_BeginPrefetch () {
	_RunThread(THREAD_PREFETCH);
}


//////////////////////////////////////////////////////////////////////////

CImageManager::CImageManager ()
	: m_currIndex((xl::uint)-1)
	, m_direction(CImageManager::FORWARD)
	, m_szPrefetch(-1, -1)//MIN_VIEW_WIDTH, MIN_VIEW_HEIGHT)
	, m_exiting(false)
{
	_CreateThreads();

	::SetThreadPriority(m_hThreads[THREAD_PREFETCH], THREAD_PRIORITY_BELOW_NORMAL);
}

CImageManager::~CImageManager () {
	m_exiting = true;
	_TerminateThreads();
}

int CImageManager::getCurrIndex () const {
	return m_currIndex;
}

size_t CImageManager::getImageCount () const {
	xl::CScopeLock lock(this);
	return m_cachedImages.size();
}

bool CImageManager::setFile (const xl::tstring &file) {
	xl::CScopeLock lock(this);

	assert(m_cachedImages.size() == 0);
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
					new_index = (int)m_cachedImages.size();
				}
				m_cachedImages.push_back(CCachedImagePtr(new CCachedImage(name)));
			}
		} while (::FindNextFile(hFind, &wfd));
		::FindClose(hFind);

		xl::trace(_T("get %d files\n"), m_cachedImages.size());
	}

	if (m_cachedImages.size() == 0) {
		::MessageBox(NULL, _T("Can not find any image files"), 0, MB_OK);
		return false;
	}

	size_t count = m_cachedImages.size();
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

void CImageManager::setCurrentSuitableImage (CImagePtr image, CSize szImage, int curr) {
	xl::CScopeLock lock(this);
	if (m_currIndex == curr) {
		m_cachedImages[curr]->setSuitableImage(image, szImage);
	}
}

CCachedImagePtr CImageManager::getCurrentCachedImage () {
	xl::CScopeLock lock(this);
	assert(m_currIndex >= 0 && m_currIndex < getImageCount());
	CCachedImagePtr cachedImage = m_cachedImages[m_currIndex];
	lock.unlock();
	return cachedImage;
}

xl::tstring CImageManager::getCurrentFileName () {
	xl::CScopeLock lock(this);
	assert(m_currIndex >= 0 && m_currIndex < getImageCount());
	CCachedImagePtr cachedImage = m_cachedImages[m_currIndex];
	lock.unlock();
	return cachedImage->getFileName();
}

void CImageManager::markThreadExit () {
	m_exiting = true;
}

void CImageManager::assignThreadProc() {
	m_procThreads[THREAD_LOAD] = &_LoadThread;
	m_procThreads[THREAD_PREFETCH] = &_PrefetchThread;
}

const xl::tchar* CImageManager::getThreadName() {
	return _T("xlview::ImageManager");
}

void CImageManager::onViewSizeChanged (CRect rc) {
	CSize sz(rc.Width(), rc.Height());
	CHECK_ZOOM_SIZE(sz);

	xl::CScopeLock lock(this);
	if (m_szPrefetch == sz) {
		return;
	}
	m_szPrefetch = sz;
	_BeginPrefetch();
}
