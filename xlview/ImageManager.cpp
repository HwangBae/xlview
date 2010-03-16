#include <assert.h>
#include <process.h>
#include <vector>
#include <set>
#include <algorithm>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageManager.h"


//////////////////////////////////////////////////////////////////////////
static const int PREFETCH_RANGE = 4;

void CImageManager::_SetIndexNoLock (int index) {
	assert(getLockLevel() > 0);
	assert(index >= 0 && index < (int)m_cachedImages.size());
	if ((int)m_currIndex != index) {
		// XLTRACE(_T("--== change index from %d to %d ==--\n"), m_currIndex, index);
		if ((int)m_currIndex < index) {
			m_direction = FORWARD;
		} else {
			m_direction = BACKWARD;
		}

		// check marginal condition
		if (m_currIndex == 0 && index == (int)m_cachedImages.size() - 1) {
			m_direction = BACKWARD;
		} else if (m_currIndex == m_cachedImages.size() - 1 && index == 0) {
			m_direction = FORWARD;
		}

		if (m_currIndex == (xl::uint)-1) {
			m_direction = FORWARD;
		}

		m_currIndex = index;

		// start prefetch first
		_BeginLoad();
		_BeginPrefetch();

		_TriggerEvent(EVT_INDEX_CHANGED, &index);
	}
}


#pragma warning (push)
#pragma warning (disable:4127)
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
		assert((int)indexes.size() == 2 * range);
	}
}
#undef IM_CHECK_INDEX
#undef IM_INSERT_INDEX
#pragma warning (pop)


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
		CSize m_szPrefetch;
	public:
		CZoomingCallback (CSize szPrefetch, int index, CImageManager *pManager)
			: m_szPrefetch(szPrefetch)
			, CLoadingCallback(index, pManager)
		{
		}

		virtual bool shouldStop () const {
			return m_szPrefetch != m_pManager->getPrefetchSize() 
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
		int currIndex = pThis->getCurrIndex();
		CLoadingCallback callback(currIndex, pThis);
		CCachedImagePtr cachedImage = pThis->getCurrentCachedImage();
		bool preloadThumbnail = cachedImage->getCachedImage() == NULL;
		if (!preloadThumbnail) {
			cachedImage.reset();
		}
		lock.unlock();

		if (preloadThumbnail) {
			xl::CTimerLogger logger(_T("Load thumbnail %s cost"), fileName.c_str());
			if (cachedImage->loadThumbnail(true, &callback)) {
				lock.lock(pThis);
				if (!callback.shouldStop()) {
					preloadThumbnail = false;
					pThis->_TriggerEvent(EVT_THUMBNAIL_LOADED, &currIndex);
				}
				lock.unlock();
			}
		}

		xl::CTimerLogger logger(_T("Load %s cost"), fileName.c_str());
		CImagePtr image = pImageLoader->load(fileName, &callback);
		if (image == NULL) {
			// assert(callback.shouldStop());
			XLTRACE(_T("**** load %s failed\n"), fileName.c_str());
		} else {
			lock.lock(pThis);
			if (currIndex == pThis->getCurrIndex()) { // make sure the image is the "current" one
				pThis->_TriggerEvent(EVT_IMAGE_LOADED, &image);
			}
			lock.unlock();
			image.reset();
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
		// continue;

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
		// note that 1.1 and 1.2 should be fast enough for a lock operation
		for (xl::uint i = 0; i < pThis->m_cachedImages.size(); ++ i) {
			bool removed = true;
			if ((int)i == currIndex) {
				removed = false;
			} else {
				if (std::find(indexes.begin(), indexes.end(), i) != indexes.end()) {
					removed = false;
				}
			}
			if (removed) {
				pThis->m_cachedImages[i]->clear(false); // remain the thumbnail
			}
		}
		lock.unlock();

		CZoomingCallback callback(szPrefetch, currIndex, pThis);

		// 1.3 load most 2 zoomed images
		int prefetched_count = 0;
		for (_CachedImages::iterator it = images.begin();
			it != images.end() && prefetched_count < 2 && !callback.shouldStop(); 
			++ it)
		{
			CCachedImagePtr image = *it;
			if (image->loadSuitable(szPrefetch, &callback)) {
				++ prefetched_count;
			}
		}

		// 1.4 load the thumbnail
		size_t processed_count = 1;
		int offset = 0; // 0 to check the current, because if the image loader doesn't support
				// load thumbnail fast (such as the PNG loader), it thumbnail is not 
				// ready for display even if the image itself it loaded completely.
		xl::CTimerLogger logger(_T("** process %d thumbnails cost"), count);
		while (processed_count < count && !callback.shouldStop()) {
			// forward
			int index = currIndex + offset;
			lock.lock(pThis);
			count = pThis->m_cachedImages.size();
			if (index >= (int)count) {
				index %= count;
			}
			CCachedImagePtr cachedImage = pThis->m_cachedImages.at(index);
			lock.unlock();

			if (cachedImage->loadThumbnail(false, &callback)) {
				lock.lock(pThis);
				if (!callback.shouldStop()) {
					pThis->_TriggerEvent(EVT_THUMBNAIL_LOADED, &index);
				}
				lock.unlock();
			}
			++ processed_count;

			// backward
			index = currIndex - offset;
			lock.lock(pThis);
			count = pThis->m_cachedImages.size();
			while (index < 0) {
				index += count;
			}
			cachedImage = pThis->m_cachedImages.at(index);
			lock.unlock();

			if (cachedImage->loadThumbnail(false, &callback)) {
				lock.lock(pThis);
				if (!callback.shouldStop()) {
					pThis->_TriggerEvent(EVT_THUMBNAIL_LOADED, &index);
				}
				lock.unlock();
			}
			++ processed_count;
			++ offset;
		}
		logger.log();

		// 1.5 load the remaining prefetch images
		for (_CachedImages::iterator it = images.begin();
			it != images.end() && !callback.shouldStop(); 
			++ it)
		{
			CCachedImagePtr image = *it;
			if (image->loadSuitable(szPrefetch, &callback)) {
			}
		}

		lock.lock(pThis);
		for (_CachedImages::iterator it = images.begin(); it != images.end(); ++ it) {
			(*it).reset();
		}
		lock.unlock();
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
	lock();
	_TriggerEvent(EVT_I_AM_DEAD, NULL);
	unlock();
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

void CImageManager::setSuitableImage (CImagePtr image, CSize szImage, int index) {
	xl::CScopeLock lock(this);
	if ((int)m_currIndex == index) {
		m_cachedImages[index]->setSuitableImage(image, szImage);
	}
}

CCachedImagePtr CImageManager::getCurrentCachedImage () {
	xl::CScopeLock lock(this);
	assert(m_currIndex >= 0 && m_currIndex < getImageCount());
	CCachedImagePtr cachedImage = m_cachedImages[m_currIndex];
	lock.unlock();
	return cachedImage;
}

CCachedImagePtr CImageManager::getCachedImage (int index) {
	xl::CScopeLock lock(this);
	assert(index >= 0 && index < (int)getImageCount());
	CCachedImagePtr cachedImage = m_cachedImages[index];
	lock.unlock();
	return cachedImage;
}

CImagePtr CImageManager::getThumbnail (int index) {
	xl::CScopeLock lock(this);
	assert(index >= 0 && index < (int)m_cachedImages.size());
	CCachedImagePtr cachedImage = m_cachedImages[index];
	CImagePtr thumbnail = cachedImage->getThumbnailImage();
	cachedImage.reset();
	lock.unlock();
	return thumbnail;
}

xl::tstring CImageManager::getCurrentFileName () {
	xl::CScopeLock lock(this);
	assert(m_currIndex >= 0 && m_currIndex < getImageCount());
	CCachedImagePtr cachedImage = m_cachedImages[m_currIndex];
	lock.unlock();
	return cachedImage->getFileName();
}

const xl::tchar* CImageManager::_GetThreadName() {
	return _T("xlview::ImageManager");
}


void CImageManager::_MarkThreadExit () {
	m_exiting = true;
}

void CImageManager::_AssignThreadProc () {
	m_procThreads[THREAD_LOAD] = &_LoadThread;
	m_procThreads[THREAD_PREFETCH] = &_PrefetchThread;
}

void CImageManager::_Lock () {
	lock();
}

void CImageManager::_Unlock () {
	unlock();
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
