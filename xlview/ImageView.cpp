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
		CSize szZoomTo = pThis->m_szZoom;
		if (pThis->m_imageZoomed && pThis->m_imageZoomed->getImageSize() == szZoomTo) {
			continue; // zoom not needed
		}

		int index = pThis->m_pImageManager->getCurrIndex();
		CImagePtr imageRS = pThis->m_imageRealSize;
		lock.unlock();

		CImagePtr imageZoomed = imageRS->resize(szZoomTo.cx, szZoomTo.cy, true);

		lock.lock(pThis);
		if (index == pThis->m_pImageManager->getCurrIndex()) {
			pThis->m_imageZoomed = imageZoomed;
			pThis->invalidate();

			pThis->m_pImageManager->setCurrentSuitableImage(imageZoomed, imageRS->getImageSize(), index);
		}
		lock.unlock();
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
		m_szDisplay = CImage::getSuitableSize(CSize(rc.Width(), rc.Height()), m_szRealSize);
	}
	m_szZoom = m_szDisplay;
	_BeginZoom();
	invalidate();
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

void CImageView::showLarger () {
	// m_di.setDisplaySize(szLarge);
	_BeginZoom();
	invalidate();
}

void CImageView::onSize () {
	assert(m_pImageManager != NULL);
	CRect rc = getClientRect();
	m_pImageManager->onViewSizeChanged(rc);

	lock();
	if (m_imageRealSize != NULL) {
		_BeginZoom();
	}
	unlock();
	invalidate();
}

void CImageView::drawMe (HDC hdc) {
	assert(m_pImageManager != NULL);
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
		int oldMode = dc.SetStretchBltMode(COLORONCOLOR);
		int sx = szImage.cx * ptSrc.x / szDisplay.cx;
		int sy = szImage.cy * ptSrc.y / szDisplay.cy;
		int sw = szImage.cx * dw / szDisplay.cx;
		int sh = szImage.cy * dh / szDisplay.cy;
		dc.StretchBlt(dx, dy, dw, dh, mdc, sx, sy, sw, sh, SRCCOPY);
		dc.SetStretchBltMode(oldMode);
	}

	dibHelper.detach();

	// free the image
	lock.lock(m_pImageManager);
	image.reset();
	lock.unlock();
}

void CImageView::onLButtonDown (CPoint pt, xl::uint key) {
}

void CImageView::onLButtonUp (CPoint pt, xl::uint key) {
}

void CImageView::onMouseMove (CPoint pt, xl::uint key) {
}

void CImageView::onTimer (xl::uint id) {
	invalidate();
}

void CImageView::onLostCapture () {
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
