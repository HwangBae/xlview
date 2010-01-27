#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"


//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////

CImageView::CImageView(void)
	: xl::ui::CControl(ID_VIEW)
{
	setStyle(_T("px:left;py:top;width:fill;height:fill;padding:10;"));
	setStyle(_T("background-color:#808080;"));
}

CImageView::~CImageView(void) {
}

void CImageView::setImage (CImagePtr image) {
	if (m_image != image) {
		m_image = image;
		invalidate();
	}
}


void CImageView::onSize () {
}

void CImageView::drawMe (HDC hdc) {
	if (m_image != NULL && m_image->getImageCount() > 0) {
		CRect rc = getClientRect();
#if 1
		xl::ui::CDIBSectionPtr thumbnail = m_image->getThumbnail();
		assert(thumbnail != NULL);
		SIZE sz = m_image->getThumbnailSize();
		xl::ui::CDCHandle dc(hdc);
		xl::ui::CDC mdc;
		mdc.CreateCompatibleDC(hdc);
		HBITMAP oldBmp = mdc.SelectBitmap(*thumbnail);

		int x = rc.left + (rc.Width() - sz.cx) / 2;
		int y = rc.top + (rc.Height() - sz.cy) / 2;
		dc.BitBlt(x, y, sz.cx, sz.cy, mdc, 0, 0, SRCCOPY);

		mdc.SelectBitmap(oldBmp);
#else
		xl::CTimerLogger log(_T("CImageView::drawMe() use"));

		xl::ui::CDIBSectionPtr dib = m_image->getImage(0);
		CSize szArea(rc.Width(), rc.Height());
		CSize szImage(dib->getWidth(), dib->getHeight());
		SIZE sz = m_image->getSuitableSize(szArea, szImage);

		xl::ui::CDIBSectionPtr dibDraw = dib->resize(sz.cx, sz.cy);
		
		xl::ui::CDCHandle dc(hdc);
		xl::ui::CDC mdc;
		mdc.CreateCompatibleDC(hdc);
		HBITMAP oldBmp = mdc.SelectBitmap(*dibDraw);

		int x = rc.left + (rc.Width() - sz.cx) / 2;
		int y = rc.top + (rc.Height() - sz.cy) / 2;
		dc.BitBlt(x, y, sz.cx, sz.cy, mdc, 0, 0, SRCCOPY);

		mdc.SelectBitmap(oldBmp);
#endif
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
