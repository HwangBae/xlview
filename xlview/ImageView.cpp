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
		CSize szZoomTo = pThis->m_di.getZoomSize();
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

	m_di.reset();
	m_imageRealSize.reset();
	m_imageZoomed.reset();

	if (index == m_pImageManager->getCurrIndex()) {
		CCachedImagePtr cachedImage = m_pImageManager->getCurrentCachedImage();
		m_imageZoomed = cachedImage->getCachedImage();
		if (m_imageZoomed != NULL) {
			m_di.setImageSize(cachedImage->getImageSize());
			assert(cachedImage->getImageSize() != CSize(-1, -1));
		}
	}
	invalidate();
}

void CImageView::_OnImageLoaded (CImagePtr image) {
	assert(getLockLevel() > 0); // must be called in lock
	assert(m_pImageManager != NULL);

	m_imageRealSize = image;
	m_di.setImageSize(image->getImageSize());
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
}

void CImageView::onSize () {
	assert(m_pImageManager != NULL);
	CRect rc = getClientRect();
	m_pImageManager->onViewSizeChanged(rc);
	m_di.setViewSize(rc);

	lock();
	if (m_imageRealSize != NULL) {
		_BeginZoom();
	}
	unlock();
	invalidate();
}

void CImageView::drawMe (HDC hdc) {
	int stretchMode = COLORONCOLOR;//HALFTONE;
	assert(m_pImageManager != NULL);

	xl::CScopeLock lock(this);
	CRect rc = getClientRect();
	if (rc.Width() <= 0 || rc.Height() <= 0) {
		return;
	}
	CImagePtr image = m_imageZoomed;
	lock.unlock();

	if (image == NULL) {
		stretchMode = COLORONCOLOR;
		lock.lock(m_pImageManager);
		image = m_imageRealSize;
		lock.unlock();
	}

	if (image == NULL) {
		return;
	}
	CSize szImage = image->getImageSize();
	CSize szDisplay = m_di.getSuitableSize();
	int x = rc.left + (rc.Width() - szDisplay.cx) / 2;
	int y = rc.top + (rc.Height() - szDisplay.cy) / 2;

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);
	xl::ui::CDIBSectionHelper selector(image->getImage(0), mdc);
	int oldmode = dc.SetStretchBltMode(stretchMode);
	dc.StretchBlt(x, y, szDisplay.cx, szDisplay.cy, mdc, 0, 0, szImage.cx, szImage.cy, SRCCOPY);
	// dc.BitBlt(x, y, szDisplay.cx, szDisplay.cy, mdc, 0, 0, SRCCOPY);
	dc.SetStretchBltMode(oldmode);


	lock.lock(m_pImageManager);
	image.reset();
	lock.unlock();

	m_di.drawDebugInfo(hdc, rc);

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
