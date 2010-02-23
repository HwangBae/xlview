#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/Language.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"
#include "MainWindow.h"


//////////////////////////////////////////////////////////////////////////
// static
unsigned __stdcall CImageView::_ZoomThread (void *param) {
	CImageView *pThis = (CImageView *)param;
	assert(pThis != NULL);
	HANDLE hEvent = pThis->m_hEvents[0];
	assert(hEvent != NULL);
	for (;;) {
		::WaitForSingleObject(hEvent, INFINITE);
		if (pThis->m_exiting) {
			break;
		}

		xl::CScopeLock lock(pThis);
		if (pThis->m_imageRealSize == NULL) {
			continue; // no source
		}
		CSize szRS = pThis->m_imageRealSize->getImageSize();
		CSize szZoomTo = pThis->m_szZoom;
		if (pThis->m_imageZoomed && pThis->m_imageZoomed->getImageSize() == szZoomTo) {
			continue; // zoom not needed
		}
		if (szZoomTo.cx * 2 >= szRS.cx || szZoomTo.cy * 2 >= szRS.cy) {
			pThis->m_imageZoomed = pThis->m_imageRealSize;
			pThis->invalidate();
			continue; // zoomed too large, so we use the real size image instead
		}

		int index = pThis->m_pImageManager->getCurrIndex();
		CImagePtr imageRS = pThis->m_imageRealSize;
		pThis->m_zooming = true;
		lock.unlock();

		CImagePtr imageZoomed = imageRS->resize(szZoomTo.cx, szZoomTo.cy, true);

		lock.lock(pThis);
		pThis->m_zooming = false;
		if (index == pThis->m_pImageManager->getCurrIndex()) {
			pThis->m_imageZoomed = imageZoomed;
			pThis->invalidate();
			lock.unlock();

			pThis->m_pImageManager->setCurrentSuitableImage(imageZoomed, szRS, index);

			lock.lock(pThis);
			imageZoomed.reset(); // avoid race condition 
			lock.unlock();
		} else {
			lock.unlock();
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// protected
void CImageView::_OnIndexChanged (int index) {
	assert(getLockLevel() > 0); // must be called in lock
	assert(m_pImageManager != NULL);

	m_szDisplay = CSize(-1, -1);
	m_szRealSize = CSize(-1, -1);
	m_szZoom = CSize(-1, -1);
	m_suitable = true;
	m_ptSrc = CPoint(0, 0);
	m_imageRealSize.reset();
	m_imageZoomed.reset();

	if (index == m_pImageManager->getCurrIndex()) {
		CCachedImagePtr cachedImage = m_pImageManager->getCurrentCachedImage();
		m_imageZoomed = cachedImage->getCachedImage();
		if (m_imageZoomed != NULL) {
			assert(cachedImage->getImageSize() != CSize(-1, -1));
			m_szRealSize = cachedImage->getImageSize();
			CRect rc = getClientRect();
			m_szDisplay = CImage::getSuitableSize(CSize(rc.Width(), rc.Height()), m_szRealSize);
		}
	}
	invalidate();
}

void CImageView::_OnImageLoaded (CImagePtr image) {
	assert(getLockLevel() > 0); // must be called in lock
	assert(m_pImageManager != NULL);

	m_imageRealSize = image;
	if (m_szDisplay == CSize(-1, -1)) {
		m_szRealSize = image->getImageSize();
		CRect rc = getClientRect();
		if (rc.Width() <= 0 || rc.Height() <= 0) {
			invalidate();
			return;
		}

		assert(m_suitable);
		m_szDisplay = CImage::getSuitableSize(CSize(rc.Width(), rc.Height()), m_szRealSize);
		invalidate();
	}
	m_szZoom = m_szDisplay;
	_BeginZoom();
}

void CImageView::_CheckPtSrc (CPoint &ptSrc) {
	int x = ptSrc.x;
	int y = ptSrc.y;

	CRect rc = getClientRect();
	int w = rc.Width();
	int h = rc.Height();

	if (x > m_szDisplay.cx - w) {
		x = m_szDisplay.cx - w;
	}
	if (x < 0) {
		x = 0;
	}
	if (y > m_szDisplay.cy - h) {
		y = m_szDisplay.cy - h;
	}
	if (y < 0) {
		y = 0;
	}

	if (ptSrc.x != x || ptSrc.y != y) {
		ptSrc = CPoint(x, y);
	}
}


void CImageView::_BeginZoom () {
	_RunThread(0);
}


//////////////////////////////////////////////////////////////////////////
// public

CImageView::CImageView (CImageManager *pImageManager)
	: xl::ui::CControl(ID_VIEW)
	, m_pImageManager(pImageManager)
	, m_szRealSize(-1, -1)
	, m_szDisplay(-1, -1)
	, m_szZoom(-1, -1)
	, m_ptSrc(0, 0)
	, m_suitable(true)
	, m_zooming(false)
	, m_ptCapture(-1, -1)
	, m_hCurNormal(::LoadCursor(NULL, IDC_ARROW))
	, m_hCurMove(::LoadCursor(NULL, IDC_SIZEALL))
	, m_exiting(false)
{
	setStyle(_T("px:left;py:top;width:fill;height:fill;padding:10;"));
	setStyle(_T("background-color:#808080;"));

	m_pImageManager->subscribe(this);

	_CreateThreads();
}

CImageView::~CImageView (void) {
	_TerminateThreads();
}

void CImageView::showSuitable () {
	xl::CScopeLock lock(this);
	if (m_szRealSize == CSize(-1, -1) || m_suitable) {
		return;
	}

	m_suitable = true;
	m_ptSrc = CPoint(0, 0);
	CRect rc = getClientRect();
	CSize szArea(rc.Width(), rc.Height());
	CSize szDisplay = CImage::getSuitableSize(szArea, m_szRealSize, true);
	m_szDisplay = szDisplay;
	m_szZoom = szDisplay;
	CHECK_VIEW_SIZE(m_szZoom);
	// create an image for fast display if ...
	if (m_imageZoomed == m_imageRealSize) {
		assert(m_imageZoomed != NULL);
		m_imageZoomed = m_imageRealSize->resize(m_szZoom.cx + 1, m_szZoom.cy + 1, false);
	}

	_BeginZoom();
	invalidate();
}

void CImageView::showRealSize () {
	xl::CScopeLock lock(this);
	if (m_szRealSize == CSize(-1, -1) || m_szDisplay == m_szRealSize) {
		return;
	}

	m_suitable = false;
	m_szDisplay = m_szZoom = m_szRealSize;
	CRect rc = getClientRect();
	if (rc.Width() >= m_szRealSize.cx) {
		m_ptSrc.x = 0;
	} else {
		m_ptSrc.x = (m_szRealSize.cx - rc.Width()) / 2;
	}
	if (rc.Height() >= m_szRealSize.cy) {
		m_ptSrc.y = 0;
	} else {
		m_ptSrc.y = (m_szRealSize.cy - rc.Height()) / 2;
	}
// 	XLTRACE(_T("RS: pt(%d - %d); rc(%d - %d), RS(%d - %d)\n"), 
// 		m_ptSrc.x, m_ptSrc.y,
// 		rc.Width(), rc.Height(),
// 		m_szRealSize.cx, m_szRealSize.cy);

	_BeginZoom();
	invalidate();
}

void CImageView::showLarger () {
	_BeginZoom();
	invalidate();
}

void CImageView::onSize () {
	xl::CTimerLogger logger(_T("onSize "));
	assert(m_pImageManager != NULL);
	CRect rc = getClientRect();
	m_pImageManager->onViewSizeChanged(rc);

	lock();
	if (m_imageRealSize != NULL) {
		if (m_suitable) {
			assert(m_szRealSize != CSize(-1, -1));
			CSize szArea(rc.Width(), rc.Height());
			m_szDisplay = CImage::getSuitableSize(szArea, m_szRealSize, true);
			m_szZoom = m_szDisplay;
			_BeginZoom();
		} else {
			_CheckPtSrc(m_ptSrc);
		}
	}
	unlock();
	invalidate();
}

void CImageView::drawMe (HDC hdc) {
	assert(m_pImageManager != NULL);
	xl::CTimerLogger logger(_T("drawMe "));
	DWORD tick = ::GetTickCount();

	xl::CScopeLock lock(this);
	CRect rc = getClientRect();
	CSize szDisplay = m_szDisplay;
	if (rc.Width() <= 0 || rc.Height() <= 0 || m_szRealSize == CSize(-1, -1) || szDisplay.cx <= 0 || szDisplay.cy <= 0) {
		return;
	}
	CImagePtr image = m_imageZoomed;
	if (image == NULL) {
		return;
	}
	CSize szImage = image->getImageSize();
	CPoint ptSrc = m_ptSrc;
	lock.unlock();

	// XLTRACE(_T("image size (%d - %d)\n"), szImage.cx, szImage.cy);
	int dx = (rc.Width() - szDisplay.cx) / 2;
	int dy = (rc.Height() - szDisplay.cy) / 2;
	int dw = szDisplay.cx - ptSrc.x;
	int dh = szDisplay.cy - ptSrc.y;
	if (dx < 0) {
		dx = 0;
		dw = rc.Width();
	} else {
		assert(ptSrc.x == 0);
	}
	if (dy < 0) {
		dy = 0;
		dh = rc.Height();
	} else {
		assert(ptSrc.y == 0);
	}
	dx += rc.left;
	dy += rc.top;

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);
	xl::ui::CDIBSectionHelper dibHelper(image->getImage(0), mdc);

	if (szImage == szDisplay) {
		// use BitBlt
		dc.BitBlt(dx, dy, dw, dh, mdc, ptSrc.x, ptSrc.y, SRCCOPY);
		XLTRACE(_T("Use BitBlt()\n"));
	} else {
		// use StretchBlt
		int sx = szImage.cx * ptSrc.x / szDisplay.cx;
		int sy = szImage.cy * ptSrc.y / szDisplay.cy;
		int sw = szImage.cx * dw / szDisplay.cx;
		int sh = szImage.cy * dh / szDisplay.cy;
		lock.lock(this);
		int oldMode = dc.SetStretchBltMode(m_zooming ? COLORONCOLOR : HALFTONE);
		dc.StretchBlt(dx, dy, dw, dh, mdc, sx, sy, sw, sh, SRCCOPY);
		dc.SetStretchBltMode(oldMode);
		lock.unlock();
	}

	dibHelper.detach();

	// free the image
	lock.lock(m_pImageManager);
	image.reset();
	lock.unlock();
}

void CImageView::onLButtonDown (CPoint pt, xl::uint key) {
	CRect rc = getClientRect();
	if (m_szDisplay.cx > rc.Width() || m_szDisplay.cy > rc.Height()) {
		::SetCursor(m_hCurMove);
		_Capture(true);
		m_ptCapture = pt;
		m_ptCaptureSrc = m_ptSrc;
	}
}

void CImageView::onLButtonUp (CPoint pt, xl::uint key) {
	if (m_ptCapture != CPoint(-1, -1)) {
		::SetCursor(m_hCurNormal);
		_Capture(false);
		m_ptCapture = CPoint(-1, -1);
	}
}

void CImageView::onMouseMove (CPoint pt, xl::uint key) {
	if (m_ptCapture != CPoint(-1, -1)) {
		::SetCursor(m_hCurMove);
		int x = m_ptCaptureSrc.x - (pt.x - m_ptCapture.x);
		int y = m_ptCaptureSrc.y - (pt.y - m_ptCapture.y);
		CPoint ptSrc = CPoint(x, y);
		_CheckPtSrc(ptSrc);

		if (m_ptSrc != ptSrc) {
			m_ptSrc = ptSrc;
			invalidate();
		}
	} else {
		::SetCursor(m_hCurNormal);
	}
}

void CImageView::onTimer (xl::uint id) {
	invalidate();
}

void CImageView::onLostCapture () {
	m_ptCapture = CPoint(-1, -1);
}


void CImageView::onEvent (EVT evt, void *param) {
	assert(m_pImageManager->getLockLevel() > 0);

	xl::CScopeLock lock(this);

	switch (evt) {
	case CImageManager::EVT_INDEX_CHANGED:
		assert(param);
		_OnIndexChanged(*(int *)param);
		break;
	case CImageManager::EVT_IMAGE_LOADED:
		assert(param);
		_OnImageLoaded(*(CImagePtr *)param);
		break;
	case CImageManager::EVT_FILELIST_READY:
		break;
	default:
		assert(false);
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
// used by ClassWithThreads
const xl::tchar* CImageView::getThreadName() {
	return _T("xlview::CImageView");
}

void CImageView::assignThreadProc() {
	m_procThreads[0] = &_ZoomThread;
}

void CImageView::markThreadExit() {
	m_exiting = true;
}	
