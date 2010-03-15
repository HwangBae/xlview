#include <assert.h>
#include "libxl/include/ui/Gdi.h"
#include "NavView.h"
#include "ImageManager.h"
#include "ImageView.h"

void CNavView::_CreateDisplayInfo () {
	if (m_currIndex == -1) {
		return;
	}
	if (m_szDisplay == CSize(-1, -1)) {
		return;
	}

	CRect rc = getClientRect();
	CSize szArea(rc.Width(), rc.Height());
	if (m_szDisplay.cx > m_szView.cx || m_szDisplay.cy > m_szView.cy) {
		// image (partial) > view
		m_dragable = true;
		CSize szImage = CImage::getSuitableSize(szArea, m_szDisplay);
		int x = (rc.Width() - szImage.cx) / 2;
		int y = (rc.Height() - szImage.cy) / 2;
		m_rcImage = CRect(x, y, x + szImage.cx, y + szImage.cy);

		double ratio = (double)szImage.cx / (double)m_szDisplay.cx;
		CSize szView = CSize((int)(m_szView.cx * ratio + 0.5), (int)(m_szView.cy * ratio + 0.5));
		if (szView.cx > szImage.cx) {
			szView.cx = szImage.cx;
		}
		if (szView.cy > szImage.cy) {
			szView.cy = szImage.cy;
		}
		x = m_rcImage.left + (int)(m_ptSrc.x * ratio + 0.5);
		y = m_rcImage.top + (int)(m_ptSrc.y * ratio + 0.5);
		m_rcView = CRect(x, y, x + szView.cx, y + szView.cy);
	} else {
		// view > image
		m_dragable = false;
		CSize szView = CImage::getSuitableSize(szArea, m_szView);
		int x = (rc.Width() - szView.cx) / 2;
		int y = (rc.Height() - szView.cy) / 2;
		m_rcView = CRect(x, y, x + szView.cx, y + szView.cy);

		double ratio = (double)szView.cx / (double)m_szView.cx;
		CSize szImage = CSize((int)(m_szDisplay.cx * ratio + 0.5), (int)(m_szDisplay.cy * ratio + 0.5));
		x = m_rcView.left + (m_rcView.Width() - szImage.cx) / 2;
		y = m_rcView.top + (m_rcView.Height() - szImage.cy) / 2;
		m_rcImage = CRect(x, y, x + szImage.cx, y + szImage.cy);
	}

	double ratio = (double)m_szDisplay.cx / (double)m_szImageReal.cx;
	int r = (int)(ratio * 100 + 0.5);
	if (r == 0) {
		r = 1;
	}
	_stprintf_s(m_ratio, COUNT_OF(m_ratio), _T("%d%%"), r);
}

CNavView::CNavView (CImageManager *pImageManager, CImageView *pImageView)
	: m_pImageManager(pImageManager)
	, m_pImageView(pImageView)
	, m_currIndex(-1)
	, m_dragable(false)
	, m_ptCapture(-1, -1)
	, m_ptSrcCapture(-1, -1)
	, m_curArrow(::LoadCursor(NULL, IDC_ARROW))
	, m_curMove(::LoadCursor(NULL, IDC_SIZEALL))
{
	assert(m_pImageManager != NULL);
	assert(m_pImageView != NULL);
	m_dibView = xl::ui::CDIBSection::createDIBSection(1, 1, 24, false);
	xl::uint8 *data = m_dibView->getLine(0);
	*data ++ = 0xff;
	*data ++ = 0xff;
	*data ++ = 0xff;
}

CNavView::~CNavView () {

}

void CNavView::setInfo (int index, CSize szImageReal, CSize szDisplay, CSize szView, CPoint ptSrc) {
	m_currIndex = index;
	m_szImageReal = szImageReal;
	m_szDisplay = szDisplay;
	m_szView = szView;
	m_ptSrc = ptSrc;

	_CreateDisplayInfo();
	invalidate();
}

void CNavView::drawMe (HDC hdc) {
	if (m_currIndex == -1) {
		return;
	}

	CRect rc = getClientRect();
	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);

	xl::CScopeLock lock(m_pImageManager);
	CCachedImagePtr cachedImage = m_pImageManager->getCachedImage(m_currIndex);
	CImagePtr image = cachedImage->getThumbnailImage();
	if (image == NULL) {
		cachedImage.reset();
		return;
	}
	xl::ui::CDIBSectionPtr dib = image->getImage(0);
	lock.unlock();

	CSize szImage = image->getImageSize();

	// draw text
	CRect rcText = m_rect;
	rcText.top += border.top.width + 2;
	rcText.left += border.left.width + 2;
	COLORREF oldColor = dc.SetTextColor(RGB(255,255,255));
	dc.drawTransparentTextWithDefaultFont(m_ratio, -1, rcText, DT_LEFT);
	dc.SetTextColor(oldColor);


	int oldMode = dc.SetStretchBltMode(HALFTONE);
	if (!m_dragable) {
		CRect rcView = m_rcView;
		rcView.OffsetRect(rc.left, rc.top);
		m_dibView->attachToDC(mdc);
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 75, 0};
		dc.AlphaBlend(rcView.left, rcView.top, rcView.Width(), rcView.Height(), mdc, 0, 0, 1, 1, bf);
		m_dibView->detachFromDC(mdc);
		

		CRect rcImage = m_rcImage;
		rcImage.OffsetRect(rc.left, rc.top);

		dib->attachToDC(mdc);
		dc.StretchBlt(rcImage.left, rcImage.top, rcImage.Width(), rcImage.Height(), mdc, 0, 0, dib->getWidth(), dib->getHeight(), SRCCOPY);
		dib->detachFromDC(mdc);
	} else {
		CRect rcImage = m_rcImage;
		rcImage.OffsetRect(rc.left, rc.top);

		dib->attachToDC(mdc);
		int oldMode = dc.SetStretchBltMode(HALFTONE);
		dc.StretchBlt(rcImage.left, rcImage.top, rcImage.Width(), rcImage.Height(), mdc, 0, 0, dib->getWidth(), dib->getHeight(), SRCCOPY);
		dc.SetStretchBltMode(oldMode);
		dib->detachFromDC(mdc);

		CRect rcView = m_rcView;
		rcView.OffsetRect(rc.left, rc.top);
		m_dibView->attachToDC(mdc);
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 75, 0};
		dc.AlphaBlend(rcView.left, rcView.top, rcView.Width(), rcView.Height(), mdc, 0, 0, 1, 1, bf);
		m_dibView->detachFromDC(mdc);
		dc.drawRectangle(rcView, 1, RGB(32,32,32), PS_SOLID);
	}
	dc.SetStretchBltMode(oldMode);

	cachedImage->lock();
	dib.reset();
	image.reset();
	cachedImage->unlock();

	lock.lock(m_pImageManager);
	cachedImage.reset();
	lock.unlock();
}

void CNavView::onMouseMove (CPoint pt, xl::uint) {
	if (!m_dragable) {
		::SetCursor(m_curArrow);
	} else {
		CRect rcView = m_rcView;
		CRect rc = getClientRect();
		rcView.OffsetRect(rc.left, rc.top);
		if (rcView.PtInRect(pt)) {
			::SetCursor(m_curMove);
		} else {
			::SetCursor(m_curArrow);
		}
	}

	if (m_ptCapture != CPoint(-1, -1)) {
		int x = pt.x - m_ptCapture.x;
		int y = pt.y - m_ptCapture.y;

		double ratio = (double)m_rcImage.Width() / (double)m_szDisplay.cx;
		x = (int)((double)x / (double)ratio + 0.5);
		y = (int)((double)y / (double)ratio + 0.5);
		x += m_ptSrcCapture.x;
		y += m_ptSrcCapture.y;
		assert(m_pImageView);
		m_pImageView->setPtSrc(x, y);
	}
}

void CNavView::onLButtonDown (CPoint pt, xl::uint) {
	assert(m_ptCapture == CPoint(-1, -1));
	CRect rcView = m_rcView;
	CRect rc = getClientRect();
	rcView.OffsetRect(rc.left, rc.top);
	if (rcView.PtInRect(pt)) {
		_Capture(true);
		m_ptCapture = pt;
		m_ptSrcCapture = m_ptSrc;
	}
}

void CNavView::onLButtonUp (CPoint, xl::uint) {
	_Capture(false);
	m_ptCapture = CPoint(-1, -1);
	m_ptSrcCapture = CPoint(-1, -1);
}

void CNavView::onLostCapture () {
	m_ptCapture = CPoint(-1, -1);
	m_ptSrcCapture = CPoint(-1, -1);
}

void CNavView::onMouseWheel (CPoint pt, int delta, xl::uint) {
	assert(m_pImageView != NULL);
	CRect rcImage = m_rcImage;
	CRect rcView = m_rcView;
	CRect rc = getClientRect();
	rcImage.OffsetRect(rc.left, rc.top);
	rcView.OffsetRect(rc.left, rc.top);
	if (rcImage.PtInRect(pt) || rcView.PtInRect(pt)) {
		CPoint ptCenter(-1, -1);
		if (delta > 0) {
			m_pImageView->showLarger(ptCenter);
		} else {
			m_pImageView->showSmaller(ptCenter);
		}
	}
}