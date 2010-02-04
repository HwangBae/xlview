#include <assert.h>
#include <process.h>
#include <vector>
#include <set>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageManager.h"


//////////////////////////////////////////////////////////////////////////
static xl::tchar* s_extensions[] = {
	_T("jpeg"),
	_T("jpg"),
	_T("jif"),
};

static const int PREFETCH_RANGE = 5;

unsigned int __stdcall CImageManager::_WorkingThread (void *param) {
	assert(param);
	CImageManager *pThis = (CImageManager *)param;
	for (;;) {
		::WaitForSingleObject(pThis->m_semaphoreWorking, INFINITE);
		if (pThis->m_exit) {
			break;
		}

		xl::CSimpleLock lockThis(&pThis->m_cs);
		CRect rc = pThis->m_rcView;
		if (rc.Width() <= 0 || rc.Height() <= 0) {
			continue;
		}
		pThis->m_indexChanged = false;
		pThis->m_sizeChanged = false;
		int currIndex = (int)pThis->m_currIndex;
		int count = (int)pThis->m_images.size();
		DIRECTION direction = pThis->m_direction;
		lockThis.unlock();

		_Indexes indexes;
		std::set<xl::uint> indexSet;
		_GetPrefetchIndexes(indexes, currIndex, count, direction, PREFETCH_RANGE);

		for (int i = 0; i < (int)indexes.size(); ++ i) {
			xl::uint index = indexes[i];
			indexSet.insert(index);
			lockThis.lock(&pThis->m_cs);
			if (pThis->cancelLoading()) {
				break;
			}
			CDisplayImagePtr displayImage = pThis->m_images.at(index);
			lockThis.unlock();

			CImagePtr zoomedImage = displayImage->getZoomedImage();
			displayImage->lock();
			if (zoomedImage == NULL) {
				if (displayImage->getRealWidth() == -1) {
					if (!displayImage->loadRealSize(pThis)) {
						displayImage->clearRealSize();
						displayImage->unlock();
						if (pThis->cancelLoading()) {
							break;
						} else {
							continue;
						}
						break;
					}
				}
				CSize szArea(rc.Width(), rc.Height());
				CSize szImage(displayImage->getRealWidth(), displayImage->getRealHeight());
				CSize sz = CImage::getSuitableSize(szArea, szImage);

				bool zoomed = displayImage->loadZoomed(sz.cx, sz.cy, pThis);
				displayImage->clearRealSize();
				if (!zoomed && pThis->cancelLoading()) {
					displayImage->unlock();
					break;
				}
#if 0
				CImagePtr zoomedImage = displayImage->getZoomedImage();
				xl::ui::CDIBSectionPtr dib = zoomedImage->getImage(0);
				unsigned char *data = (unsigned char *)dib->getData();
				size_t len = dib->getStride();
				// data += (dib->getHeight() * len / 2);
				memset(data, 255, len);
#endif
				XLTRACE(_T("loaded: %s\n"), displayImage->getFileName().c_str());
			} else {
				assert(zoomedImage->getImageCount() > 0);
			}
			displayImage->unlock();
		} // for ()

		if (pThis->cancelLoading()) {
			XLTRACE(_T("**cancel begin clear zoomed images**\n"));
			continue;
		}

		// clear zoomed image which is far from current, for saving memory usage
		XLTRACE(_T("**begin clear zoomed images**\n"));
		lockThis.lock(&pThis->m_cs);
		currIndex = pThis->m_currIndex;
		indexSet.insert(currIndex);
		for (size_t i = 0; i < (size_t)count && !pThis->cancelLoading(); ++ i) {
			CDisplayImagePtr image = pThis->m_images[i];
			image->lock();
			if (indexSet.find(i) == indexSet.end()) {
				if (image->getZoomedImage() != NULL) {
					XLTRACE(_T("clear zoomed image (%s) (%d) from (%d)\n"),
						image->getFileName().c_str(), i, currIndex);
					image->clearZoomed();
				}
			}
			image->unlock();
		}
		lockThis.unlock();

		// get thumbnail
	}
	return 0;
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



bool CImageManager::_IsFileSupported (const xl::tstring &fileName) {
	size_t index = fileName.rfind(_T('.'));
	if (index != fileName.npos) {
		xl::tstring ext = fileName.substr(index + 1);
		bool match = false;
		for (int i = 0; i < COUNT_OF(s_extensions); ++ i) {
			if (_tcsicmp(ext.c_str(), s_extensions[i]) == 0) {
				match = true;
				break;
			}
		}
		return match;
	}
	return false;
}

void CImageManager::_CreateThreads () {
	assert(m_semaphoreWorking == NULL);
	::InitializeCriticalSection(&m_cs);

	xl::tchar name[128];
	_stprintf_s(name, 128, _T("xlview::imagemanager::semaphore::load for 0x%08x on %d"), this, ::GetTickCount());
	m_semaphoreWorking = ::CreateSemaphore(NULL, 0, 1, name);
	m_threadWorking = (HANDLE)_beginthreadex(NULL, 0, _WorkingThread, this, 0, NULL);
	if (m_threadWorking != INVALID_HANDLE_VALUE) {
		SetThreadPriority(m_threadWorking, THREAD_PRIORITY_BELOW_NORMAL);
	}
}

void CImageManager::_TerminateThreads () {
	m_exit = true;
	::ReleaseSemaphore(m_semaphoreWorking, 1, NULL);
	HANDLE handles[] = {m_threadWorking};
	if (::WaitForMultipleObjects(COUNT_OF(handles), handles, TRUE, 3000) == WAIT_TIMEOUT) {
		XLTRACE(_T("CImageManager::_TerminateThreads() wait for thread exit failed\n"));
		for (int i = 0; i < COUNT_OF(handles); ++ i) {
			TerminateThread(handles[i], 0);
			::CloseHandle(handles[i]);
		}
	}
	m_threadWorking = NULL;

	::DeleteCriticalSection(&m_cs);
}

// note, don't prefetch the 'current' image, 
// left it to the view to fetch, to avoid conflict
void CImageManager::_BeginPrefetch () {
	if (m_rcView.Width() <= 0 || m_rcView.Height() <= 0) {
		return;
	}

	::ReleaseSemaphore(m_semaphoreWorking, 1, NULL);
}



//////////////////////////////////////////////////////////////////////////

CImageManager::CImageManager ()
	: m_currIndex((xl::uint)-1)
	, m_direction(CImageManager::FORWARD)
	, m_rcView(0, 0, 0, 0)
	, m_exit(false)
	, m_indexChanged(false)
	, m_sizeChanged(false)
	, m_semaphoreWorking(NULL)
	, m_threadWorking(NULL)
{
	_CreateThreads();
}

CImageManager::~CImageManager () {
	_TerminateThreads();
}

int CImageManager::getCurrIndex () const {
	return m_currIndex;
}

int CImageManager::getImageCount () const {
	return (int)m_images.size();
}

void CImageManager::setFile (const xl::tstring &file) {
	assert(m_images.size() == 0);
	xl::tstring fileName = xl::file_get_name(file);
	m_directory = xl::file_get_directory(file);
	m_directory += _T("\\");
	xl::tstring pattern = m_directory + _T("*.*");
	xl::CTimerLogger logger(_T("searching files cost"));

	int new_index = -1;

	// find the files
	WIN32_FIND_DATA wfd;
	memset(&wfd, 0, sizeof(wfd));
	HANDLE hFind = ::FindFirstFile(pattern, &wfd);
	if (hFind != INVALID_HANDLE_VALUE) {

		xl::CSimpleLock lock(&m_cs);
		do {
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				continue; // skip directory
			}

			xl::tstring name = m_directory + wfd.cFileName;
			if (_IsFileSupported(name)) {
				if (_tcsicmp(wfd.cFileName, fileName) == 0) {
					new_index = (int)m_images.size();
				}
				m_images.push_back(CDisplayImagePtr(new CDisplayImage(name)));
			}

		} while (::FindNextFile(hFind, &wfd));
		lock.unlock();

		::FindClose(hFind);
		
		xl::trace(_T("get %d files\n"), m_images.size());
	} else {
		::MessageBox(NULL, _T("Can not find any image file"), 0, MB_OK);
	}

	size_t count = m_images.size();
	_TriggerEvent(EVT_READY, &count);

	if (new_index == -1 && count > 0) {
		new_index = 0;
	}
	setIndex(new_index);
}

void CImageManager::setIndex (int index) {
	xl::CSimpleLock lock(&m_cs);
	if (m_currIndex != index) {
		if ((int)m_currIndex < index) {
			m_direction = FORWARD;
		} else {
			m_direction = BACKWARD;
		}

		// check marginal condiction
		if (m_currIndex == 0 && index == m_images.size() - 1) {
			m_direction = BACKWARD;
		} else if (m_currIndex == m_images.size() - 1 && index == 0) {
			m_direction = FORWARD;
		}

		if (m_currIndex != (xl::uint)-1) {
			m_indexChanged = true;
		} else {
			m_direction = FORWARD;
		}


		m_currIndex = index;

		_BeginPrefetch();
		_TriggerEvent(EVT_INDEX_CHANGED, &index);
	}
}

CDisplayImagePtr CImageManager::getImage (int index) {
	xl::CSimpleLock lock(&m_cs);
	assert(index >= 0 && index < getImageCount());
	CDisplayImagePtr image = m_images[index];
	lock.unlock();
	return image;
}

void CImageManager::onViewSizeChanged (CRect rc) {

	xl::CSimpleLock lock(&m_cs);
	if (m_rcView == rc) {
		return;
	}

	m_rcView = rc;
	if (m_rcView.Width() <= 0 || m_rcView.Height() <= 0) {
		return;
	}

	m_sizeChanged = true;
	_BeginPrefetch();
}

bool CImageManager::cancelLoading () {
	return m_indexChanged || m_sizeChanged || m_exit;
}