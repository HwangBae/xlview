#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"
#include "MainWindow.h"

//////////////////////////////////////////////////////////////////////////
// static

// TODO: BUG, consider, curr is 5, now loading 5 (release semaphore), 
// then fast move to 4 (release semaphore), then move back to 5,
// then the loading finished,  and get the semaphore, it try to load the same image
unsigned int __stdcall CImageView::_LoadThread (void *param) {
	assert(param);
	CImageView *pThis = (CImageView *)param;
	for (;;) {
		::WaitForSingleObject(pThis->m_semaphoreLoad, INFINITE);
		if (pThis->m_exit) {
			break;
		}

		xl::CScopeLock lock(pThis);
		pThis->m_loading = true;
		pThis->m_currLoading = pThis->m_currIndex;
		CDisplayImagePtr displayImage = pThis->m_image;
		lock.unlock();
		assert(displayImage != NULL);

		bool loaded = displayImage->loadRealSize(pThis);

		lock.lock(pThis);
		pThis->m_loading = false;
		if (!pThis->shouldCancel()) {
			pThis->_OnImageLoaded(loaded);
		}
		pThis->m_currLoading = -1;
	}
	return 0;
}

unsigned int __stdcall CImageView::_ResizeThread (void *param) {
	assert(param);
	CImageView *pThis = (CImageView *)param;
	for (;;) {
		::WaitForSingleObject(pThis->m_semaphoreResize, INFINITE);
		if (pThis->m_exit) {
			break;
		}

		CRect rc = pThis->getClientRect();
		if (rc.Width() <= 0 || rc.Height() <= 0) {
			continue;
		}

		xl::CScopeLock lock(pThis);
		if (pThis->m_loading) {
			continue;
		} else if (pThis->m_image->getRealWidth() == -1 || pThis->m_image->getRealHeight() == -1) {
			assert(false);
			continue;
		}
		pThis->m_resizing = true;
		int currIndex = pThis->m_currIndex;
		CDisplayImagePtr image = pThis->m_image;
		lock.unlock();

		CSize szImage(image->getRealWidth(), image->getRealHeight());
		CSize sz = CImage::getSuitableSize(CSize(rc.Width(), rc.Height()), szImage);

		image->loadZoomed(sz.cx, sz.cy, pThis);

		lock.lock(pThis);
		pThis->m_resizing = false;
		if (pThis->m_currIndex == currIndex) {
			pThis->_OnImageResized();
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
// protected

void CImageView::_CreateThreads () {
	assert(m_semaphoreLoad == NULL);

	xl::tchar name[128];
	_stprintf_s(name, 128, _T("xlview::imageview::semaphore::load for 0x%08x on %d"), this, ::GetTickCount());
	m_semaphoreLoad = ::CreateSemaphore(NULL, 0, 1, name);
	m_threadLoad = (HANDLE)_beginthreadex(NULL, 0, _LoadThread, this, 0, NULL);

	_stprintf_s(name, 128, _T("xlview::imageview::semaphore::resize for 0x%08x on %d"), this, ::GetTickCount());
	m_semaphoreResize = ::CreateSemaphore(NULL, 0, 1, name);
	m_threadResize = (HANDLE)_beginthreadex(NULL, 0, _ResizeThread, this, 0, NULL);
}

void CImageView::_TerminateThreads () {
	m_exit = true;
	::ReleaseSemaphore(m_semaphoreLoad, 1, NULL);
	::ReleaseSemaphore(m_semaphoreResize, 1, NULL);
	HANDLE handles[] = {m_threadLoad, m_threadResize};
	if (::WaitForMultipleObjects(COUNT_OF(handles), handles, TRUE, 3000) == WAIT_TIMEOUT) {
		XLTRACE(_T("CImageView::_TerminateThreads() wait for thread exit failed\n"));
		for (int i = 0; i < COUNT_OF(handles); ++ i) {
			TerminateThread(handles[i], 0);
			::CloseHandle(handles[i]);
		}
	}
	m_threadResize = m_threadLoad = NULL;
}


void CImageView::_ResetParameter () {
	xl::CScopeLock lock(this);;
	m_suitable = true;
	m_zoomTo = m_zoomNow = 0;
	if (m_image != NULL) {
		m_image->clearRealSize();
	}
	m_image.reset();
}

void CImageView::_PrepareDisplay () {

	xl::CScopeLock lock(this);;
	m_image = m_pImageManager->getImage(m_currIndex);
	assert(m_image);
	if (m_image->getRealSizeImage() == NULL) {
		_BeginLoad();
	} else if (m_image->getZoomedImage() == NULL) {
		_BeginResize();
	} else {
		CSize sz = _GetZoomedSize();
		CSize szImage = m_image->getZoomedImage()->getImageSize();
		if (sz.cx != -1 && sz.cy != -1 && sz != szImage) {
			_BeginResize();
		}
	}
}

CSize CImageView::_GetZoomedSize () {
	xl::CScopeLock lock(this);;
	CSize sz(-1, -1);
	if (m_image) {
		if (m_suitable) {
			CRect rc = getClientRect();
			if (rc.Width() > 0 && rc.Height() > 0) {
				int w = m_image->getRealWidth();
				int h = m_image->getRealHeight();
				sz = CImage::getSuitableSize(CSize(rc.Width(), rc.Height()), CSize(w, h));
			}
		} else {
			assert(false); // TODO
		}
	}
	assert(sz.cx != -1 && sz.cy != -1);
	return sz;
}

void CImageView::_BeginLoad () {
	::ReleaseSemaphore(m_semaphoreLoad, 1, NULL);
}

void CImageView::_BeginResize () {
	CRect rc = getClientRect();
	if (rc.Width() <= 0 || rc.Height() <= 0) {
		return;
	}
	if (m_image->getRealWidth() <= 0 || m_image->getRealHeight() <= 0) {
		return;
	}

	CSize sz = _GetZoomedSize();
	CImagePtr zoomedPtr = m_image->getZoomedImage();
	if (zoomedPtr != NULL && sz == zoomedPtr->getImageSize()) {
		int w = m_image->getRealWidth();
		int h = m_image->getRealHeight();
		if (sz.cx == w && sz.cy == h) {
			return; // not needed
		}
	}
	::ReleaseSemaphore(m_semaphoreResize, 1, NULL);
}


void CImageView::_OnIndexChanged (int idx) {
	xl::CScopeLock lock(this);;
	int newIndex = idx;
	assert(idx == m_pImageManager->getCurrIndex());
	if (newIndex != m_currIndex) {
		m_currIndex = newIndex;
		_ResetParameter();
		_PrepareDisplay();
		invalidate();
	}
}

void CImageView::_OnImageLoaded (bool success) {
	if (m_image->getZoomedImage() == NULL) {
		_BeginResize();
		invalidate();
	} else {
		CSize sz = _GetZoomedSize();
		CSize szImage = m_image->getZoomedImage()->getImageSize();
		if (sz.cx != -1 && sz.cy != -1 && sz != szImage) {
			_BeginResize();
		}
		invalidate();
	}
}

void CImageView::_OnImageResized () {
	assert(m_image->getZoomedImage()->getImageCount() > 0);
	invalidate();
}

//////////////////////////////////////////////////////////////////////////

CImageView::CImageView (CImageManager *pImageManager)
	: xl::ui::CControl(ID_VIEW)
	, m_pImageManager(pImageManager)
	, m_currIndex(-1)
	, m_exit(false)
	, m_loading(false)
	, m_resizing(false)
	, m_semaphoreLoad(NULL)
	, m_threadLoad(NULL)
{
	_CreateThreads();

	assert(m_pImageManager != NULL);
	m_currIndex = m_pImageManager->getCurrIndex();
	m_pImageManager->subscribe(this);

	_ResetParameter();

	setStyle(_T("px:left;py:top;width:fill;height:fill;padding:10;"));
	setStyle(_T("background-color:#808080;"));
}

CImageView::~CImageView (void) {
	_TerminateThreads();
}

void CImageView::onSize () {
	if (!m_suitable) {
		return;
	}
	CRect rc = getClientRect();
	if (rc.Width() > 0 && rc.Height() > 0) {
		assert(m_pImageManager);
		m_pImageManager->onViewSizeChanged(rc);
		_BeginResize();
	}
}

void CImageView::drawMe (HDC hdc) {
	// xl::CTimerLogger logger(_T("Paint cost"));
	CRect rc = getClientRect();
	if (rc.Width() <= 0 || rc.Height() <= 0) {
		return;
	}

	xl::CScopeLock lock(this);;
	if (m_image == NULL) {
		return;
	}
	CImagePtr zoomedImage = m_image->getZoomedImage();
	CImagePtr realsizeImage = m_image->getRealSizeImage();
	CImagePtr thumbnail = m_image->getThumbnail();
	if (zoomedImage == NULL && realsizeImage == NULL && thumbnail == NULL) {
		return;
	}
	CImagePtr drawImage;

	if (zoomedImage && zoomedImage->getImageCount() > 0) {
		drawImage = zoomedImage;
	} else if (realsizeImage && realsizeImage->getImageCount() > 0 && !m_loading) {
		XLTRACE(_T("*****************use realsize image****************\n"));
		drawImage = realsizeImage;
	} else if (thumbnail != NULL && thumbnail->getImageCount() > 0) {
		drawImage = thumbnail;
	}
	lock.unlock();

	if (drawImage != NULL) {

		int w = m_image->getRealWidth(); // bitmap->getWidth();
		int h = m_image->getRealHeight(); // bitmap->getHeight();
		assert(w > 0 && h > 0);

		if (m_suitable) {
			CSize sz = _GetZoomedSize();
			w = sz.cx;
			h = sz.cy;
		} else {
			assert(false); // TODO
		}

		int x = (rc.Width() - w) / 2;
		int y = (rc.Height() - h) / 2;
		if (w > rc.Width()) {
			w = rc.Width();
		}
		if (h > rc.Height()) {
			h = rc.Height();
		}
		if (x < 0) {
			x = 0;
		}
		if (y < 0) {
			y = 0;
		}
		x += rc.left;
		y += rc.top;


		xl::ui::CDIBSectionPtr bitmap = drawImage->getImage(0);
		xl::ui::CDCHandle dc(hdc);
		xl::ui::CDC mdc;
		mdc.CreateCompatibleDC(hdc);
		xl::ui::CDIBSectionHelper selector(bitmap, mdc);

		if (w != drawImage->getImageWidth() || h != drawImage->getImageHeight()) {
			int oldMode = dc.SetStretchBltMode(drawImage == thumbnail ? HALFTONE : COLORONCOLOR);
			dc.StretchBlt(x, y, w, h, mdc, 0, 0, drawImage->getImageWidth(), drawImage->getImageHeight(), SRCCOPY);
			dc.SetStretchBltMode(oldMode);
		} else {
			dc.BitBlt(x, y, w, h, mdc, 0, 0, SRCCOPY);
		}
	}
}

void CImageView::onLButtonDown (CPoint pt, xl::uint key) {
}

void CImageView::onLButtonUp (CPoint pt, xl::uint key) {
}

void CImageView::onMouseMove (CPoint pt, xl::uint key) {
}

void CImageView::onLostCapture () {
}


void CImageView::onEvent (EVT evt, void *param) {
	switch (evt) {
	case CImageManager::EVT_INDEX_CHANGED:
		assert(param != NULL);
		_OnIndexChanged(*(int *)param);
		break;
	default:
		break;
	}
}

bool CImageView::shouldCancel () {
	return (m_currIndex != m_currLoading) || m_exit;
}