#include <assert.h>
#include "libxl/include/ui/Gdi.h"
#include "NavView.h"
#include "ImageManager.h"

void CNavView::_CreateDisplayInfo () {

}

CNavView::CNavView (CImageManager *pImageManager)
	: m_pImageManager(pImageManager)
	, m_currIndex(-1)
{
	assert(m_pImageManager != NULL);
}

CNavView::~CNavView () {

}

void CNavView::setInfo (int index, CSize szDisplay, CSize szView, CPoint ptSrc) {
	m_currIndex = index;
	m_szDisplay = szDisplay;
	m_szView = szView;
	m_ptSrc = ptSrc;

	_CreateDisplayInfo();
	invalidate();
}

static void _drawView (HDC hdc, CRect rc, CSize szView) {
	int x = rc.left + (rc.Width() - szView.cx) / 2;
	int y = rc.top + (rc.Height() - szView.cy) / 2;
	xl::ui::CDCHandle dc(hdc);
	CRect rcView(x, y, x + szView.cx, y + szView.cy);
	dc.FillSolidRect(rcView, RGB(255,255,255));
}

void CNavView::drawMe (HDC hdc) {
	if (m_currIndex == -1) {
		return;
	}

	CRect rc = getClientRect();
	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);
	CSize szArea(rc.Width(), rc.Height());

	xl::CScopeLock lock(m_pImageManager);
	CImagePtr image = m_pImageManager->getThumbnail(m_currIndex);
	if (image == NULL) {
		return;
	}
	xl::ui::CDIBSectionPtr dib = image->getImage(0);
	CSize szImage = image->getImageSize();
	image.reset();

	if (m_szView.cx >= m_szDisplay.cx && m_szView.cy >= m_szDisplay.cy) {
		assert(m_ptSrc.x == 0 && m_ptSrc.y == 0);
		CSize szView = CImage::getSuitableSize(szArea, m_szView, false);
		_drawView(hdc, rc, szView);
		double ratio = (double)szView.cx / (double)m_szView.cx;

		int image_x = (int)(ratio * m_szDisplay.cx);
		int image_y = (int)(ratio * m_szDisplay.cy);

		int x = rc.left + (rc.Width() - image_x) / 2;
		int y = rc.top + (rc.Height() - image_y) / 2;
		dib->attachToDCNoLock(mdc);
		dc.StretchBlt(x, y, image_x, image_y, mdc, 0, 0, dib->getWidth(), dib->getHeight(), SRCCOPY);
		dib->detachFromDCNoLock(mdc);
	} else {

	}

	dib.reset();
	lock.unlock();
}