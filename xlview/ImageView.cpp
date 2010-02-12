#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/Language.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"
#include "MainWindow.h"

//////////////////////////////////////////////////////////////////////////
// CDisplayParameter

//////////////////////////////////////////////////////////////////////////
// private

void CDisplayParameter::_DrawSuitable (HDC hdc, CImagePtr image) {
	CRect rc = rcView;
	CSize szArea(rc.Width(), rc.Height());
	CSize szImage = image->getImageSize();
	CSize sz = CImage::getSuitableSize(szArea, realSize);

	int x = rc.left + (rc.Width() - sz.cx) / 2;
	int y = rc.top + (rc.Height() - sz.cy) / 2;

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(hdc);
	xl::ui::CDIBSectionHelper selector(image->getImage(frameIndex), mdc);

	if (sz == szImage) {
		dc.BitBlt(x, y, sz.cx, sz.cy, mdc, 0, 0, SRCCOPY);
#ifndef NDEBUG
		xl::tchar info[128];
		_stprintf_s(info, 128, _T("BitBlt: %d-%d; RS: %d-%d"), sz.cx, sz.cy, realSize.cx, realSize.cy);
		dc.TextOut(10, 10, info);
#endif
	} else {
		int oldMode = dc.SetStretchBltMode(COLORONCOLOR);
		dc.StretchBlt(x, y, sz.cx, sz.cy, mdc, 0, 0, szImage.cx, szImage.cy, SRCCOPY);
		dc.SetStretchBltMode(oldMode);

#ifndef NDEBUG
		xl::tchar info[128];
		_stprintf_s(info, 128, _T("SBlt[COC]: %d-%d; RS: %d-%d"), sz.cx, sz.cy, realSize.cx, realSize.cy);
		dc.TextOut(10, 10, info);
#endif
	}

	selector.detach();
}

void CDisplayParameter::_DrawZoom (HDC hdc, CImagePtr image) {
	CRect rc = rcView;
	CSize szArea(rc.Width(), rc.Height());
	CSize szZoom = getZoomNowSize();
	CSize szImage = image->getImageSize();

	int x = rc.left;
	int w = rc.Width();
	if (szZoom.cx < rc.Width()) {
		x += (rc.Width() - szZoom.cx) / 2;
		w = szZoom.cx;
	}
	int y = rc.top;
	int h = rc.Height();
	if (szZoom.cy < rc.Height()) {
		y += (rc.Height() - szZoom.cy) / 2;
		h = szZoom.cy;
	}

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(hdc);
	xl::ui::CDIBSectionHelper selector(image->getImage(frameIndex), mdc);

	if (szZoom == szImage) {
		dc.BitBlt(x, y, w, h, mdc, srcX, srcY, SRCCOPY);
	} else {
		int srcW = w * szImage.cx / szZoom.cx;
		int srcH = h * szImage.cy / szZoom.cy;
		int oldMode = dc.SetStretchBltMode(COLORONCOLOR);
		dc.StretchBlt(x, y, w, h, mdc, srcX, srcY, srcW, srcH, SRCCOPY);
		dc.SetStretchBltMode(oldMode);
	}

	selector.detach();
}


void CDisplayParameter::_CalacuteParameter () {
	assert(realSize.cx > 0 && realSize.cy > 0);
	assert(rcView.Width() > 0 && rcView.Height() > 0);
	if (suitable) {
		CSize szArea(rcView.Width(), rcView.Height());
		CSize szZoom = CImage::getSuitableSize(szArea, realSize);
		int zoom = (int)(100 * szZoom.cx / realSize.cx);
		zoomTo = zoom;
		zoomNow = zoom;
		srcX = 0;
		srcY = 0;
	} else {
		// assert(false);
	}
}

//////////////////////////////////////////////////////////////////////////
// public
CDisplayParameter::CDisplayParameter ()
	: loaded(false)
	, suitable(true)
	, zoomTo(0)
	, zoomNow(0)
	, srcX(0)
	, srcY(0)
	, realSize(-1, -1)
	// , zoomSize(-1, -1)
	, rcView(0, 0, 0, 0)
	, frameIndex(0)
{
}

CDisplayParameter::~CDisplayParameter () {

}

void CDisplayParameter::reset (CRect rc) {
	loaded = false;
	suitable = true;
	zoomTo = zoomNow = 0;
	srcX = srcY = 0;
	realSize.cx = realSize.cy = -1;
	// zoomSize.cx = zoomSize.cy = -1;
	rcView = rc;
	frameIndex = 0;
}

void CDisplayParameter::setRealSize (CSize rs) {
	assert(rs.cx > 0 && rs.cy > 0);
	realSize = rs;
	if (rcView.Width() <= 0 || rcView.Height() <= 0) {
		return;
	}

	_CalacuteParameter();
}

void CDisplayParameter::setLoaded () {
	assert(!loaded);
	loaded = true;
}

void CDisplayParameter::setViewRect (CRect rc) {
	if (rcView == rc) {
		return;
	}

	rcView = rc;
	if (realSize != CSize(-1, -1)) {
		_CalacuteParameter();
	}
}

CSize CDisplayParameter::getZoomToSize () const {
	assert(realSize.cx > 0 && realSize.cy > 0);
	int w = (int)(realSize.cx * zoomTo / 100);
	if (w == 0) {
		w = 1;
	}
	int h = (int)(realSize.cy * zoomTo / 100);
	if (h == 0) {
		h = 0;
	}

	return CSize(w, h);
}


CSize CDisplayParameter::getZoomNowSize () const {
	assert(realSize.cx > 0 && realSize.cy > 0);
	int w = (int)(realSize.cx * zoomNow / 100);
	if (w == 0) {
		w = 1;
	}
	int h = (int)(realSize.cy * zoomNow / 100);
	if (h == 0) {
		h = 0;
	}

	return CSize(w, h);
}

bool CDisplayParameter::showLarger () {
	suitable = false;
	int offset = zoomTo * 15 / 100;
	if (offset == 0) {
		offset = 1;
	}
	zoomTo += offset;
	if (abs(zoomTo - 100) < 5) {
		zoomTo = 100;
	}

	if (zoomTo >= 100) {
		return false;
	}

	return true;
}

bool CDisplayParameter::onTimer () {
	if (zoomNow == zoomTo) {
		return false;
	}

	++ zoomNow;
	if (zoomNow == zoomTo) {
		return false;
	}

	if (zoomTo >= 100) {
		return false;
	}

	return true;
}

void CDisplayParameter::draw (HDC hdc, CImagePtr image) {
	if (rcView.Width() <= 0 || rcView.Height() <= 0) {
		return;
	}

	if (image != NULL) {
		assert(realSize != CSize(-1, -1));
		if (suitable) {
			_DrawSuitable(hdc, image);
		} else {
			_DrawZoom(hdc, image);
		}
	}

	if (!isLoaded()) {
		drawLoading(hdc);
	}

#ifndef NDEBUG
	drawParameter(hdc);
#endif
}

void CDisplayParameter::drawLoading (HDC hdc) {
	xl::CLanguage *pLang = xl::CLanguage::getInstance();
	xl::tstring strLoading = pLang->getString(_T("loading..."));

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CResMgr *pResMgr = xl::ui::CResMgr::getInstance();
	HFONT font = pResMgr->getSysFont();
	HFONT oldFont = dc.SelectFont(font);
	int oldMode = dc.SetBkMode(TRANSPARENT);

	UINT fmt = DT_SINGLELINE | DT_CENTER | DT_TOP;//DT_VCENTER;
	dc.DrawShadowText(strLoading, strLoading.length(), rcView, fmt, 
		RGB(80,80,80), RGB(232,232,232), 1, 1);

	dc.SetBkMode(oldMode);
	dc.SelectFont(oldFont);
}

void CDisplayParameter::drawParameter (HDC hdc) {
	xl::tchar info[256];
	_stprintf_s(info, 256, _T("%s; zoom to: %d, zoom now: %d"), 
		suitable ? _T("suit") : _T("zoom"),
		zoomTo, zoomNow
		);
	CRect rc = rcView;
	rc.top = rc.bottom - 20;
	xl::ui::CDCHandle dc(hdc);
	xl::uint fmt = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
	dc.DrawText(info, -1, rc, fmt);
}

//////////////////////////////////////////////////////////////////////////
// static
unsigned __stdcall CImageView::_ZoomThread (void *param) {
	CImageView *pThis = (CImageView *)param;
	assert(pThis != NULL);
	HANDLE hEvent = pThis->m_hZoomEvent;
	assert(hEvent != NULL);
	for (;;) {
		::WaitForSingleObject(hEvent, INFINITE);
		if (pThis->m_exiting) {
			break;
		}

		xl::CScopeLock lock(pThis);
		CSize szZoom = pThis->m_disp.getZoomToSize();
		CImagePtr zoomedImage = pThis->m_imageZoomed;
		if (zoomedImage != NULL && zoomedImage->getImageSize() == szZoom) {
			continue;
		}

		if (pThis->m_imageRealSize == NULL) {
			continue; // no source
		}

		CImagePtr rsImage = pThis->m_imageRealSize;
		zoomedImage.reset();
		lock.unlock();

		zoomedImage = rsImage->resize(szZoom.cx, szZoom.cy, true);

		lock.lock(pThis);
		if (rsImage == pThis->m_imageRealSize) {
			pThis->m_imageZoomed = zoomedImage;
		}
		lock.unlock();
		pThis->invalidate();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// protected
void CImageView::_OnIndexChanged (int index) {
	assert(getLockLevel() > 0); // must be called in lock
	assert(m_pImageManager != NULL);

	m_disp.reset(getClientRect());
	m_imageRealSize.reset();
	m_imageZoomed.reset();
	m_imageThumbnail.reset();

	if (index == m_pImageManager->getCurrIndex()) {
		CCachedImagePtr cachedImage = m_pImageManager->getCurrentCachedImage();
		m_imageZoomed = cachedImage->getCachedImage();
		if (m_imageZoomed != NULL) {
			m_disp.setRealSize(cachedImage->getImageSize());
		}
	}
	invalidate();
}

void CImageView::_OnImageLoaded (CImagePtr image) {
	assert(getLockLevel() > 0); // must be called in lock
	assert(m_pImageManager != NULL);

	m_disp.setRealSize(image->getImageSize());
	m_imageRealSize = image;
	invalidate();
}

void CImageView::_CreateThreads () {
	assert(m_hZoomThread == INVALID_HANDLE_VALUE);
	assert(m_hZoomEvent == NULL);

	xl::tchar name[MAX_PATH];

	xl::CScopeLock lock(this);
	_stprintf_s(name, MAX_PATH, 
		_T("Local\\xlview::ImageView::ZoomEvent [created at %d]"), ::GetTickCount());
	m_hZoomEvent = ::CreateEvent(NULL, FALSE, FALSE, name);
	assert(m_hZoomEvent != NULL);
	m_hZoomThread = (HANDLE)_beginthreadex(NULL, 0, _ZoomThread, this, 0, NULL);
	assert(m_hZoomThread != INVALID_HANDLE_VALUE);
}

void CImageView::_TerminateThreads () {
	assert(m_hZoomEvent != NULL);
	assert(m_hZoomThread != INVALID_HANDLE_VALUE);

	bool exiting = m_exiting;
	m_exiting = true;
	::SetEvent(m_hZoomEvent);
	if (::WaitForSingleObject(m_hZoomThread, 3000) != WAIT_OBJECT_0) {
		::TerminateThread(m_hZoomThread, -1);
		XLTRACE(_T("** Thread [zoom] does not exit normally\n"));
	}

	CloseHandle(m_hZoomEvent);
	CloseHandle(m_hZoomThread);
	m_hZoomEvent = NULL;
	m_hZoomThread = INVALID_HANDLE_VALUE;

	m_exiting = exiting;
}

void CImageView::_BeginZoom () {
	if (m_hZoomEvent != NULL) {
		::SetEvent(m_hZoomEvent);
	} else {
		::SetEvent(m_hZoomEvent);
	}
}


//////////////////////////////////////////////////////////////////////////
// public

CImageView::CImageView (CImageManager *pImageManager)
	: xl::ui::CControl(ID_VIEW)
	, m_pImageManager(pImageManager)
	, m_exiting(false)
	, m_hZoomEvent(NULL)
	, m_hZoomThread(INVALID_HANDLE_VALUE)
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
	if (!m_disp.isLoaded()) {
		// how to handle?
		return;
	}

	if (m_disp.showLarger()) {
		_SetTimer(50, m_id);
		_BeginZoom();
	}
	invalidate();
}

CRect rect;
void CImageView::onSize () {
	assert(m_pImageManager != NULL);
	CRect rc = getClientRect();
	m_pImageManager->onViewSizeChanged(rc);
	lock();
	m_disp.setViewRect(rc);

	if (m_imageRealSize != NULL) {
		_BeginZoom();
	}
	unlock();
	invalidate();
}

void CImageView::drawMe (HDC hdc) {
	int stretchMode = HALFTONE;
	assert(m_pImageManager != NULL);

	xl::CScopeLock lock(this);
	CImagePtr image = m_imageZoomed;
	CRect rc = getClientRect();
	lock.unlock();

	if (image == NULL) {
		stretchMode = COLORONCOLOR;
		lock.lock(m_pImageManager);
		image = m_imageRealSize;
		lock.unlock();

		lock.lock(this);
		if (image == NULL) {
			image = m_imageThumbnail;
		}
		lock.unlock();
	}

	m_disp.draw(hdc, image);

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
	if (m_disp.onTimer()) {
		XLTRACE(_T("--- ON TIMER ---\n"));
		_SetTimer(50, m_id);
	}

	invalidate();
}

void CImageView::onLostCapture () {
}


void CImageView::onEvent (EVT evt, void *param) {
	assert(m_pImageManager->getLockLevel() > 0);

	xl::CScopeLock lock(this);

	switch (evt) {
	case CImageManager::EVT_FILELIST_READY:
		break;
	case CImageManager::EVT_INDEX_CHANGED:
		assert(param);
		_OnIndexChanged(*(int *)param);
		break;
	case CImageManager::EVT_IMAGE_LOADED:
		assert(param);
		_OnImageLoaded(*(CImagePtr *)param);
		break;
	default:
		assert(false);
		break;
	}
}
