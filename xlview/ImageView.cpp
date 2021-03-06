#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/Language.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"
#include "MainWindow.h"
#include "NavView.h"
#include "InfoView.h"

//////////////////////////////////////////////////////////////////////////
// callback when zooming
namespace {
	class CZoomingCallback : public xl::ILongTimeRunCallback {
		CSize m_szZoom;
		int m_index;
		CImageView *m_pView;
		CImageManager *m_pManager;
	public:
		CZoomingCallback (CSize szZoom, int index, CImageView *pView, CImageManager *pManager) 
			: m_szZoom(szZoom), m_index(index)
			, m_pView(pView), m_pManager(pManager) 
		{
			assert(m_pView != NULL && m_pManager != NULL);
		}

		virtual bool shouldStop () const {
			return m_szZoom != m_pView->getZoomSize()
				|| m_index != m_pManager->getCurrIndex()
				|| m_pView->isExiting();
		}
	};
}

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

		CScopeMultiLock lock(pThis, true);
		if (pThis->m_imageRealSize == NULL) {
			continue; // no source
		}
		bool suitable = pThis->m_suitable;
		CSize szRS = pThis->m_imageRealSize->getImageSize();
		CSize szZoomTo = pThis->m_szZoom;
		// 1. if ratio > 1, use original
		// 2. if image size > 2500 * 2000, and ratio > 0.7, use the original
		double ratio = (double)szZoomTo.cx / (double)szRS.cx;
		__int64 pixel_count = szRS.cx * szRS.cy;
		if (ratio >= 0.99 || (pixel_count > 2500 * 2000 && ratio > 0.7))
		{
			pThis->m_imageZoomed = pThis->m_imageRealSize;
			pThis->invalidate();
			continue; // zoom to a too large size, so we use the real size image instead
		}
		if (pThis->m_imageZoomed && pThis->m_imageZoomed->getImageSize() == szZoomTo) {
			continue; // zoom not needed
		}

		int index = pThis->m_pImageManager->getCurrIndex();
		CImagePtr imageRS = pThis->m_imageRealSize;
		pThis->m_zooming = true;
		lock.unlock();

		xl::CTimerLogger logger(_T("** Resize image (%d-%d) to (%d-%d) cost"), 
			szRS.cx, szRS.cy, szZoomTo.cx, szZoomTo.cy);
		CZoomingCallback callback(szZoomTo, index, pThis, pThis->m_pImageManager);
		CImagePtr imageZoomed = imageRS->resize(szZoomTo.cx, szZoomTo.cy, true, &callback);
		imageRS.reset(); // no use now, save memory
		logger.log();

		lock.lock(pThis, true);
		pThis->m_zooming = false;
		if (imageZoomed != NULL && index == pThis->m_pImageManager->getCurrIndex()) {
			pThis->m_imageZoomed = imageZoomed;
			pThis->invalidate();
			lock.unlock();

			if (suitable) {
				pThis->m_pImageManager->setSuitableImage(imageZoomed, szRS, index);
			}
		} else {
			lock.unlock();
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// protected
void CImageView::_OnIndexChanged (int index) {
	assert(m_pImageManager != NULL && m_pImageManager->getLockLevel() > 0);
	assert(getLockLevel() > 0); // must be called in lock

	m_currIndex = index;
	_ResetDisplayInfo();

	if (index == m_pImageManager->getCurrIndex()) {
		CCachedImagePtr cachedImage = m_pImageManager->getCurrentCachedImage();
		m_imageZoomed = cachedImage->getCachedImage();
		if (m_imageZoomed != NULL) {
			assert(cachedImage->getImageSize() != CSize(-1, -1));
			m_szReal = cachedImage->getImageSize();
			CRect rc = getClientRect();
			if (rc.Width() > 0 && rc.Height() > 0) {
				CSize szArea(rc.Width(), rc.Height());
				CSize szDisplay = CImage::getSuitableSize(szArea, m_szReal);
				// _SetDisplaySize(rc, szDisplay);
				m_szDisplay = szDisplay; // _SetDisplaySize need m_szDisplay not (-1, -1)
				_NotifyDisplayChanged();
			}
		}
	}
	invalidate();
}

void CImageView::_OnImageLoaded (CImagePtr image) {
	assert(m_pImageManager != NULL && m_pImageManager->getLockLevel() > 0);
	assert(getLockLevel() > 0); // must be called in lock

	m_imageRealSize = image;
	m_szReal = image->getImageSize();
	if (m_szDisplay == CSize(-1, -1)) {
		CRect rc = getClientRect();
		// xl::trace(_T("rc: %d %d"), rc.Width(), rc.Height());
		if (rc.Width() <= 0 || rc.Height() <= 0) {
			invalidate();
			return;
		}

		assert(m_suitable);
		m_szDisplay = CImage::getSuitableSize(CSize(rc.Width(), rc.Height()), m_szReal);
		_NotifyDisplayChanged();
		invalidate();
	}
	_SetZoomSize(m_szDisplay);
}

void CImageView::_OnThumbnailLoaded (int index) {
	assert(m_pImageManager != NULL && m_pImageManager->getLockLevel() > 0);
	assert(getLockLevel() > 0); // must be called in lock

	if (index != m_pImageManager->getCurrIndex()) {
		return;
	} else {
		if (m_imageRealSize == NULL && m_imageZoomed == NULL) {
			_OnIndexChanged(index);
		}
	}
}

CRect CImageView::_CalcDisplayArea (CRect rcView, CSize szDisplay, CPoint ptSrc) {
	int dx = (rcView.Width() - szDisplay.cx) / 2;
	int dy = (rcView.Height() - szDisplay.cy) / 2;
	int dw = szDisplay.cx - ptSrc.x;
	int dh = szDisplay.cy - ptSrc.y;
	if (dx < 0) {
		dx = 0;
		dw = rcView.Width();
	} else {
		assert(ptSrc.x == 0);
	}
	if (dy < 0) {
		dy = 0;
		dh = rcView.Height();
	} else {
		assert(ptSrc.y == 0);
	}
	dx += rcView.left;
	dy += rcView.top;

	return CRect(dx, dy, dx + dw, dy + dh);
}

void CImageView::_SetDisplaySize (CRect rcView, CSize szDisplay, CPoint ptCur) {
	assert(m_pImageManager != NULL && m_pImageManager->getLockLevel() > 0);
	assert(getLockLevel() > 0);
	CRect rcDisplayAreaBefore = _CalcDisplayArea(rcView, m_szDisplay, m_ptSrc);
	if (!rcDisplayAreaBefore.PtInRect(ptCur)) {
		ptCur = CPoint(rcView.left + rcView.Width() / 2, rcView.top + rcView.Height() / 2);
	}
	CSize szDisplayBefore = m_szDisplay;
	assert(szDisplayBefore.cx > 0 && szDisplayBefore.cy > 0);
	m_szDisplay = szDisplay;

	// adjust m_ptSrc
	if (rcView.Width() >= m_szDisplay.cx) {
		m_ptSrc.x = 0;
	} else {
		int x = (int)(0.5 + m_szDisplay.cx * ((double)ptCur.x - rcDisplayAreaBefore.left + m_ptSrc.x) / szDisplayBefore.cx);
		m_ptSrc.x = x + rcView.left - ptCur.x;
	}
	if (rcView.Height() >= m_szDisplay.cy) {
		m_ptSrc.y = 0;
	} else {
		int y = (int)(0.5 + m_szDisplay.cy * ((double)ptCur.y - rcDisplayAreaBefore.top + m_ptSrc.y) / szDisplayBefore.cy);
		m_ptSrc.y = y + rcView.top - ptCur.y;
	}
	_CheckPtSrc(m_ptSrc);

	// use real size image or zoomed image as source?
	// Here maybe a problem, if image manager delete the cachedImage could cause **race condition**
	if (m_imageRealSize != NULL) {
		CCachedImagePtr cachedImage = m_pImageManager->getCurrentCachedImage();
		CImagePtr image = cachedImage->getCachedImage();
		if (image != NULL) {
			CSize szCached = image->getImageSize();
			if (m_imageZoomed == m_imageRealSize) {
				if (szCached.cx >= szDisplay.cx && szCached.cy >= szDisplay.cy) {
					m_imageZoomed = image;
				}
			}
			image.reset();
		}
		cachedImage.reset();
	}

	_NotifyDisplayChanged();

	invalidate();
}

void CImageView::_SetZoomSize (CSize szZoom) {
	assert(getLockLevel() > 0);
	CHECK_ZOOM_SIZE(szZoom);
	m_szZoom = szZoom;
	_BeginZoom();
}

void CImageView::_ResetDisplayInfo () {
	assert(m_pImageManager && m_pImageManager->getLockLevel() > 0);
	assert(getLockLevel() > 0); // must be locked
	m_szDisplay = CSize(-1, -1);
	m_szReal = CSize(-1, -1);
	m_szZoom = CSize(-1, -1);
	m_suitable = true;
	m_ptSrc = CPoint(0, 0);
	m_imageRealSize.reset();
	m_imageZoomed.reset();
#ifdef PROGRESS_ZOOMING
	m_ptCurSaved = CPoint(-1, -1);
#endif

	_NotifyDisplayChanged();
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

void CImageView::_NotifyDisplayChanged () {
	if (m_pNavView || m_pInfoView) {
		CRect rc = getClientRect();
		CSize szImageReal = m_szReal;
		CSize szDisplay = m_szDisplay;
		CSize szView(rc.Width(), rc.Height());
		CPoint ptSrc = m_ptSrc;

		assert(m_currIndex != -1);
		if (m_pNavView) {
			m_pNavView->setInfo(m_currIndex, szImageReal, szDisplay, szView, ptSrc);
		}

		if (m_pInfoView) {
			m_pInfoView->onDisplayInfoChanged(szDisplay);
		}
	}
}

void CImageView::_CalculateZoomedSize (CSize &szDisplay, CSize szReal, bool isZoomin, double factor) {
	double x, y;
	if (szReal.cx > szReal.cy) {
		double cx = szReal.cx / 100.0;
		if (cx < (double)szDisplay.cx * factor) {
			cx = (double)szDisplay.cx * factor;
		}
		
		if (isZoomin) {
			x = szDisplay.cx + (int)cx;
		} else {
			x = szDisplay.cx - (int)cx;
			if (x < 1) {
				x = 1;
			}
		}
		double ratio = x / szReal.cx;
		int ratio_i = (int)(ratio + 0.5);
		if ((abs(ratio - (double)ratio_i) <= factor / 2) && ratio_i != 0) {
			x = szReal.cx * ratio_i;
			y = szReal.cy * ratio_i;
		} else {
			y = (int)(x * szReal.cy / szReal.cx);
			if (y < 1) {
				y = 1;
			}
		}
	} else {
		double cy = szReal.cy / 100.0;
		if (cy < (double)szDisplay.cy * factor) {
			cy = (double)szDisplay.cy * factor;
		}

		if (isZoomin) {
			y = szDisplay.cy + cy;
		} else {
			y = szDisplay.cy - cy;
			if (y < 1) {
				y = 1;
			}
		}
		double ratio = y / szReal.cy;
		int ratio_i = (int)(ratio + 0.5);
		if ((abs(ratio - (double)ratio_i) <= factor / 2) && ratio_i != 0) {
			x = szReal.cx * ratio_i;
			y = szReal.cy * ratio_i;
		} else {
			x = (int)(y * szReal.cx / szReal.cy);
			if (x < 1) {
				x = 1;
			}
		}
	}
	szDisplay = CSize((int)x, (int)y);
}

void CImageView::_CreateCachedBitmap () {
	CRect rc = m_rect;
	if (rc.Width() <= 0 || rc.Height() <= 0) {
		return;
	}

	int w = ::GetSystemMetrics(SM_CXSCREEN);
	int h = ::GetSystemMetrics(SM_CYSCREEN);
	w = w > rc.Width() ? w : rc.Width();
	h = h > rc.Height() ? h : rc.Height();

	if (m_cachedBitmap && m_cachedBitmap->getWidth() >= w && m_cachedBitmap->getHeight() >= h) {
		return;
	}

	m_cachedBitmap = xl::ui::CDIBSection::createDIBSection(w, h, 24, false);
}

// not used, bitblt into the cached bitmap in drawMe()
void CImageView::_SetCachedBitmap (HDC hdc) {
	CRect rc = getClientRect();
	assert(m_cachedBitmap != NULL && m_cachedBitmap->getWidth() == rc.Width() && m_cachedBitmap->getHeight() == rc.Height());

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);
	m_cachedBitmap->attachToDC(mdc);
	mdc.BitBlt(0, 0, rc.Width(), rc.Height(), mdc, rc.left, rc.top, SRCCOPY);
	m_cachedBitmap->detachFromDC(mdc);
	m_dirty = false;
}

void CImageView::_GetCachedBitmap (HDC hdc) {
	CRect rc = m_rect;
	assert(m_cachedBitmap != NULL && m_cachedBitmap->getWidth() >= rc.Width() && m_cachedBitmap->getHeight() >= rc.Height());

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);
	m_cachedBitmap->attachToDC(mdc);
	dc.BitBlt(rc.left, rc.top, rc.Width(), rc.Height(), mdc, 0, 0, SRCCOPY);
	m_cachedBitmap->detachFromDC(mdc);
}

#ifdef PROGRESS_ZOOMING
bool CImageView::_CalcStepedDisplaySize (CSize &szDisplay, CSize szZoom) {
	assert(getLockLevel() > 0);
	if (szDisplay == szZoom) {
		return false; // not needed
	}

	double deltaX = (double)szZoom.cx - m_szDisplayBegin.cx;
	double deltaY = (double)szZoom.cy - m_szDisplayBegin.cy;
	m_step ++;
	deltaX = deltaX * m_step / 20;
	deltaY = deltaY * m_step / 20;
	szDisplay.cx += deltaX;
	szDisplay.cy += deltaY;

	assert(szZoom.cx > 0);
	double ratio = (double)szDisplay.cx / (double)szZoom.cx;
	if (ratio >= 0.95 && ratio <= 1.05) {
		szDisplay = szZoom;
	}

	return true;
}
#endif

void CImageView::_BeginZoom () {
	assert(getLockLevel() > 0);
	CHECK_ZOOM_SIZE(m_szZoom);
	_RunThread(0);
}


//////////////////////////////////////////////////////////////////////////
// public

CImageView::CImageView (CImageManager *pImageManager)
	: xl::ui::CControl(ID_VIEW)
	, CMultiLock(pImageManager)
	, m_pImageManager(pImageManager)
	, m_pNavView(NULL)
	, m_pInfoView(NULL)
	, m_currIndex(-1)
	, m_dirty(false)
	, m_szReal(-1, -1)
	, m_szDisplay(-1, -1)
	, m_szZoom(-1, -1)
	, m_ptSrc(0, 0)
	, m_suitable(true)
	, m_zooming(false)
	, m_ptCapture(-1, -1)
#ifdef PROGRESS_ZOOMING
	, m_ptCurSaved(-1, -1)
#endif
	, m_hCurNormal(::LoadCursor(NULL, IDC_ARROW))
	, m_hCurMove(::LoadCursor(NULL, IDC_SIZEALL))
	, m_exiting(false)
{
	setStyle(_T("px:left;py:top;width:fill;height:fill;padding:2;"));
	// setStyle(_T("background-color:#202020;"));

	m_pImageManager->subscribe(this);

	_CreateThreads();
}

CImageView::~CImageView (void) {
	_TerminateThreads();
}

void CImageView::showSuitable (CPoint /*ptCur*/) {
	CScopeMultiLock lock(this, true);
	if (m_szReal == CSize(-1, -1) || m_suitable) {
		return;
	}

	m_suitable = true;

	CRect rc = getClientRect();
	CSize szArea(rc.Width(), rc.Height());
	CSize szDisplay = CImage::getSuitableSize(szArea, m_szReal, true);
	_SetDisplaySize(rc, szDisplay);
	_SetZoomSize(szDisplay);
	// get an image for fast display if ...
	if (m_imageZoomed == m_imageRealSize) {
		assert(m_imageZoomed != NULL);
		m_imageZoomed = m_pImageManager->getCurrentCachedImage()->getCachedImage();
	}
}

void CImageView::showRealSize (CPoint ptCur) {
	CScopeMultiLock lock(this, true);
	if (m_szReal == CSize(-1, -1) || m_szDisplay == m_szReal) {
		return;
	}
	m_suitable = false;

	CRect rc = getClientRect();
	_SetZoomSize(m_szReal);

#ifdef PROGRESS_ZOOMING
	m_szDisplayBegin = m_szDisplay;
	m_step = 0;
	CSize szDisplay = m_szDisplay;
	CSize szZoom = m_szZoom;
	
	_CalcStepedDisplaySize(szDisplay, szZoom);
	_SetDisplaySize(rc, szDisplay, ptCur);
	m_ptCurSaved = ptCur;
	_SetTimer(100, ID_VIEW);
#else
	_SetDisplaySize(rc, m_szReal, ptCur);
#endif
}

void CImageView::showSwitch (CPoint ptCur) {
	if (m_suitable) {
		showRealSize(ptCur);
	} else {
		showSuitable(ptCur);
	}
}

void CImageView::showLarger (CPoint ptCur) {
	CScopeMultiLock lock(this, true);
	if (m_szReal == CSize(-1, -1)) {
		return;
	}
	m_suitable = false;

	CSize szDisplay = m_szDisplay;
	_CalculateZoomedSize(szDisplay, m_szReal, true, 0.15);
	_SetZoomSize(szDisplay);

	CRect rc = getClientRect();
	_SetDisplaySize(rc, szDisplay, ptCur);
}

void CImageView::showSmaller (CPoint ptCur) {
	CScopeMultiLock lock(this, true);
	if (m_szReal == CSize(-1, -1)) {
		return;
	}
	m_suitable = false;

	CSize szDisplay = m_szDisplay;
	_CalculateZoomedSize(szDisplay, m_szReal, false, 0.15);
	_SetZoomSize(szDisplay);

	CRect rc = getClientRect();
	_SetDisplaySize(rc, szDisplay, ptCur);
}

void CImageView::showTop (CPoint ptCur) {
	XL_PARAMETER_NOT_USED(ptCur);
	CScopeMultiLock lock(this, false);
	if (m_szReal == CSize(-1, -1) || m_ptSrc.y == 0) {
		return;
	}

	m_ptSrc.y = 0;
	_NotifyDisplayChanged();
	invalidate();
}

void CImageView::showBottom (CPoint ptCur) {
	XL_PARAMETER_NOT_USED(ptCur);
	CScopeMultiLock lock(this, false);
	CRect rc = getClientRect();
	if (m_szReal == CSize(-1, -1) || m_szDisplay.cy <= rc.Height()) {
		return;
	}

	CPoint ptSrc = m_ptSrc;
	ptSrc.y = m_szDisplay.cy;
	_CheckPtSrc(ptSrc);
	if (ptSrc != m_ptSrc) {
		m_ptSrc = ptSrc;
		_NotifyDisplayChanged();
		invalidate();
	}
}

void CImageView::showLeft (CPoint ptCur) {
	XL_PARAMETER_NOT_USED(ptCur);
	CScopeMultiLock lock(this, false);
	if (m_szReal == CSize(-1, -1) || m_ptSrc.x == 0) {
		return;
	}

	m_ptSrc.x = 0;
	_NotifyDisplayChanged();
	invalidate();
}

void CImageView::showRight (CPoint ptCur) {
	XL_PARAMETER_NOT_USED(ptCur);
	CScopeMultiLock lock(this, false);
	CRect rc = getClientRect();
	if (m_szReal == CSize(-1, -1) || m_szDisplay.cx <= rc.Width()) {
		return;
	}

	CPoint ptSrc = m_ptSrc;
	ptSrc.x = m_szDisplay.cx;
	_CheckPtSrc(ptSrc);
	if (ptSrc != m_ptSrc) {
		m_ptSrc = ptSrc;
		_NotifyDisplayChanged();
		invalidate();
	}
}

void CImageView::setPtSrc (int x, int y) {
	CPoint ptSrc = CPoint(x, y);
	_CheckPtSrc(ptSrc);

	if (m_ptSrc != ptSrc) {
		m_ptSrc = ptSrc;

		_NotifyDisplayChanged();

		invalidate();
	}
}

void CImageView::setNavView (CNavView *pNavView) {
	assert(m_pNavView == NULL);
	assert(pNavView != NULL);
	m_pNavView = pNavView;
	if (m_currIndex != -1) {
		_NotifyDisplayChanged();
	}
}

void CImageView::setInfoView (CInfoView *pInfoView) {
	assert(m_pInfoView == NULL);
	assert(pInfoView != NULL);
	m_pInfoView = pInfoView;
	if (m_currIndex != -1) {
		_NotifyDisplayChanged();
	}
}

void CImageView::onSize () {
	assert(m_pImageManager != NULL);
	CRect rc = getClientRect();
	m_pImageManager->onViewSizeChanged(rc);

	CScopeMultiLock lock(this, false);
	if (m_imageRealSize != NULL) {
		if (m_suitable) {
			assert(m_szReal != CSize(-1, -1));
			CSize szArea(rc.Width(), rc.Height());
			m_szDisplay = CImage::getSuitableSize(szArea, m_szReal, true);
			_SetZoomSize(m_szDisplay);

			_NotifyDisplayChanged();
		} else {
			_CheckPtSrc(m_ptSrc);
			_NotifyDisplayChanged();
		}
	}
	lock.unlock();
	_CreateCachedBitmap ();
	invalidate();
}

void CImageView::drawMe (HDC hdc) {
	assert(m_pImageManager != NULL);

	CRect rc = getClientRect();
	CSize szDisplay = m_szDisplay;
	if (rc.Width() <= 0 || rc.Height() <= 0 || m_szReal == CSize(-1, -1) || szDisplay.cx <= 0 || szDisplay.cy <= 0) {
		return;
	}
	assert(m_cachedBitmap != NULL && m_cachedBitmap->getWidth() >= m_rect.Width() && m_cachedBitmap->getHeight() >= m_rect.Height());
	if (!m_dirty) {
		_GetCachedBitmap(hdc);
		return;
	}

	m_dirty = false;
	CScopeMultiLock lock(this, false);
	CImagePtr image = m_imageZoomed;
	if (image == NULL) {
		return;
	}
	xl::ui::CDIBSectionPtr dib = image->getImage(0);
	assert(dib != NULL);
	CSize szImage = image->getImageSize();
	CPoint ptSrc = m_ptSrc;
	lock.unlock();

	CRect rcDisplayArea = _CalcDisplayArea(rc, szDisplay, ptSrc);

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC cdc;
	cdc.CreateCompatibleDC(dc);
	m_cachedBitmap->attachToDC(cdc);
	cdc.FillSolidRect(m_rect, RGB(0x20, 0x20, 0x20));

	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);

	// xl::ui::CDIBSectionHelper dibHelper(dib, mdc);
	// I don't use ::StretchBlt() in the back thread for zooming, 
	// so BitBlt or StretchBlt without lock is safe.
	dib->attachToDC(mdc);

	if (szImage == szDisplay) {
		cdc.BitBlt(rcDisplayArea.left, rcDisplayArea.top, rcDisplayArea.Width(), rcDisplayArea.Height(), 
			mdc, ptSrc.x, ptSrc.y, SRCCOPY);
	} else {
		// use StretchBlt
		int sx = (int)(0.5 + (double)szImage.cx * (double)ptSrc.x / (double)szDisplay.cx);
		int sy = (int)(0.5 + (double)szImage.cy * (double)ptSrc.y / (double)szDisplay.cy);
		int sw = (int)(0.5 + (double)szImage.cx * (double)rcDisplayArea.Width() / (double)szDisplay.cx);
		int sh = (int)(0.5 + (double)szImage.cy * (double)rcDisplayArea.Height() / (double)szDisplay.cy);
		CScopeMultiLock lock(this, false);
		int oldMode = cdc.SetStretchBltMode(HALFTONE);
		cdc.StretchBlt(rcDisplayArea.left, rcDisplayArea.top, rcDisplayArea.Width(), rcDisplayArea.Height(),
			mdc, sx, sy, sw, sh, SRCCOPY);
		cdc.SetStretchBltMode(oldMode);
		lock.unlock();
	}
	dib->detachFromDC(mdc);
	m_cachedBitmap->detachFromDC(cdc);

//	_SetCachedBitmap(hdc);
	_GetCachedBitmap(hdc);

#ifndef NDEBUG
	// draw debug information
	{
		xl::tchar buf[256];
		double ratio = (double)m_szDisplay.cx / (double)m_szReal.cx;
		_stprintf_s(buf, 256, _T("%.4fx (%d - %d) => (%d - %d)"), ratio, 
			m_szReal.cx, m_szReal.cy, m_szDisplay.cx, m_szDisplay.cy);
		CRect rcInfo = rc;
		rcInfo.bottom = rcInfo.top + 16;
		dc.DrawText(buf, -1, rcInfo, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	}
#endif

	// free the image
	m_pImageManager->lock();
	dib.reset();
	image.reset();
	m_pImageManager->unlock();
}

void CImageView::onLButtonDown (CPoint pt, xl::uint /*key*/) {
	CRect rc = getClientRect();
	if (m_szDisplay.cx > rc.Width() || m_szDisplay.cy > rc.Height()) {
		::SetCursor(m_hCurMove);
		_Capture(true);
		m_ptCapture = pt;
		m_ptCaptureSrc = m_ptSrc;
	}
}

void CImageView::onLButtonUp (CPoint /*pt*/, xl::uint /*key*/) {
	if (m_ptCapture != CPoint(-1, -1)) {
		::SetCursor(m_hCurNormal);
		_Capture(false);
		m_ptCapture = CPoint(-1, -1);
	}
}

void CImageView::onMouseMove (CPoint pt, xl::uint /*key*/) {
	if (m_ptCapture != CPoint(-1, -1)) {
		::SetCursor(m_hCurMove);
		int x = m_ptCaptureSrc.x - (pt.x - m_ptCapture.x);
		int y = m_ptCaptureSrc.y - (pt.y - m_ptCapture.y);
		CPoint ptSrc = CPoint(x, y);
		_CheckPtSrc(ptSrc);

		if (m_ptSrc != ptSrc) {
			m_ptSrc = ptSrc;

			_NotifyDisplayChanged();
			
			invalidate();
		}
	} else {
		::SetCursor(m_hCurNormal);
	}
}

void CImageView::onMouseWheel (CPoint pt, int delta, xl::uint /*key*/) {
	if (delta > 0) {
		showLarger(pt);
	} else {
		showSmaller(pt);
	}
}

void CImageView::onTimer (xl::uint /*id*/) {
	CScopeMultiLock lock(this, false);
#ifdef PROGRESS_ZOOMING
	if (m_ptCurSaved == CPoint(-1, -1) || m_szDisplay == m_szZoom) {
		m_ptCurSaved = CSize(-1, -1);
		return;
	}

	CSize szDisplay = m_szDisplay;
	CSize szZoom = m_szZoom;
	if (_CalcStepedDisplaySize(szDisplay, szZoom)) {
		_SetTimer(100, ID_VIEW);
	}
	_SetDisplaySize(getClientRect(), szDisplay, m_ptCurSaved);

	invalidate();
#endif
}

void CImageView::onLostCapture () {
	m_ptCapture = CPoint(-1, -1);
}

void CImageView::invalidate () {
	m_dirty = true;
	xl::ui::CControl::invalidate();
}

void CImageView::onEvent (EVT evt, void *param) {
	assert(m_pImageManager->getLockLevel() > 0);

	CScopeMultiLock lock(this, false);

	switch (evt) {
	case CImageManager::EVT_INDEX_CHANGED:
		assert(param);
		_OnIndexChanged(*(int *)param);
		break;
	case CImageManager::EVT_IMAGE_LOADED:
		assert(param);
		_OnImageLoaded(*(CImagePtr *)param);
		break;
	case CImageManager::EVT_THUMBNAIL_LOADED:
		assert(param);
		_OnThumbnailLoaded(*(int *)param);
		break;
	case CImageManager::EVT_FILELIST_READY:
		break;
	case CImageManager::EVT_I_AM_DEAD:
		clearExternalLock();
		break;
	default:
		assert(false);
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
// used by ClassWithThreads
const xl::tchar* CImageView::_GetThreadName () {
	return _T("xlview::CImageView");
}

void CImageView::_AssignThreadProc () {
	m_procThreads[THREAD_ZOOM] = &_ZoomThread;
}

void CImageView::_MarkThreadExit () {
	m_exiting = true;
}

void CImageView::_Lock () {
	lockMe();
}

void CImageView::_Unlock () {
	unlockMe();
}
