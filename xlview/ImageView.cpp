#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"
#include "MainWindow.h"

//////////////////////////////////////////////////////////////////////////
// static

unsigned int __stdcall CImageView::_LoadThread (void *param) {
	assert(param);
	CImageView *pThis = (CImageView *)param;
	for (;;) {
		::WaitForSingleObject(pThis->m_semaphoreLoad, INFINITE);
		if (pThis->m_exit) {
			break;
		}

		xl::CSimpleLock lock(&pThis->m_cs);
		pThis->m_currLoading = pThis->m_currIndex;
		CDisplayImagePtr image = pThis->m_imageRealSize;
		lock.unlock();
		assert(image != NULL);

		bool loaded = image->load(pThis);

		lock.lock(&pThis->m_cs);
		if (!pThis->cancelLoading()) {
			pThis->_OnImageLoaded(loaded);
		}
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

		xl::CSimpleLock lock(&pThis->m_cs);
		int currIndex = pThis->m_currIndex;
		CDisplayImagePtr image = pThis->m_imageRealSize;
		CDisplayImagePtr imageZoomed = image->clone();// = pThis->m_imageZoomed;
		lock.unlock();

		CRect rc = pThis->getClientRect();
		CSize sz = CImage::getSuitableSize(CSize(rc.Width(), rc.Height()), image->getImageSize());

		imageZoomed->resize(sz.cx, sz.cy);

		lock.lock(&pThis->m_cs);
		if (pThis->m_currIndex == currIndex) {
			pThis->m_imageZoomed = imageZoomed;
			pThis->_OnImageResized();
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
// protected

void CImageView::_CreateThreads () {
	assert(m_semaphoreLoad == NULL);
	::InitializeCriticalSection(&m_cs);

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
		for (int i = 0; i < COUNT_OF(handles); ++ i) {
			TerminateThread(handles[i], 0);
			::CloseHandle(handles[i]);
		}
	}
	m_threadResize = m_threadLoad = NULL;

	::DeleteCriticalSection(&m_cs);
}


void CImageView::_ResetParameter () {
	xl::CSimpleLock lock(&m_cs);
	m_suitable = true;
	m_zoom = 0;
	m_imageZoomed.reset();
	m_imageRealSize.reset();
}

void CImageView::_PrepareDisplay () {
	CDisplayImagePtr image = m_pImageManager->getImage(m_currIndex);

	xl::CSimpleLock lock(&m_cs);
	assert(image);
	assert(m_imageZoomed == NULL);
	assert(m_imageRealSize == NULL);

	m_imageZoomed = image->clone();
	m_imageRealSize.reset(new CDisplayImage(m_imageZoomed->getFileName()));
	_BeginLoad();
}

void CImageView::_BeginLoad () {
	::ReleaseSemaphore(m_semaphoreLoad, 1, NULL);
}

void CImageView::_BeginResize () {
	::ReleaseSemaphore(m_semaphoreResize, 1, NULL);
}


void CImageView::_OnIndexChanged () {
	xl::CSimpleLock lock(&m_cs);
	int newIndex = m_pImageManager->getCurrIndex();
	if (newIndex != m_currIndex) {
		m_currIndex = newIndex;
		_ResetParameter();
		_PrepareDisplay();
		invalidate();
	}
}

void CImageView::_OnImageLoaded (bool success) {
	xl::ui::CCtrlMain *pCtrlMain = _GetMainCtrl();
	assert(pCtrlMain);
	ATL::CWindow *pWindow = pCtrlMain->getWindow();
	assert(pWindow);
	xl::CSimpleLock lock(&m_cs);

	// test code
	if (m_imageZoomed->getImageCount() == 0) {
		_BeginResize();
	} else {
		invalidate();
	}
}

void CImageView::_OnImageResized () {
	assert(m_imageZoomed->getImageCount() > 0);
	invalidate();
}

//////////////////////////////////////////////////////////////////////////

CImageView::CImageView (CImageManager *pImageManager)
	: xl::ui::CControl(ID_VIEW)
	, m_pImageManager(pImageManager)
	, m_currIndex(-1)
	, m_exit(false)
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
	CRect rc = getClientRect();
	assert(m_pImageManager);
	m_pImageManager->onViewSizeChanged(rc);
}

void CImageView::drawMe (HDC hdc) {
	CRect rc = getClientRect();

	xl::CSimpleLock lock(&m_cs);
	if (m_imageZoomed != NULL && m_imageZoomed->getImageCount() > 0) {

		xl::ui::CDIBSectionPtr bitmap = m_imageZoomed->getImage(0);

		xl::ui::CDCHandle dc(hdc);
		xl::ui::CDC mdc;
		mdc.CreateCompatibleDC(hdc);
		HBITMAP oldBmp = mdc.SelectBitmap(*bitmap);

		int w = bitmap->getWidth();
		int h = bitmap->getHeight();
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

		dc.BitBlt(x, y, w, h, mdc, 0, 0, SRCCOPY);

		mdc.SelectBitmap(oldBmp);
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
		_OnIndexChanged();
		break;
	default:
		break;
	}
}

bool CImageView::cancelLoading () {
	return (m_currIndex != m_currLoading) || m_exit;
}