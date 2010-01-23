#include <assert.h>
#include <Windows.h>
#include <GdiPlus.h>
#include "libxl/include/ui/Gdi.h"
#include "ImageView.h"

CImageView::CImageView(void)
	: xl::ui::CControl(ID_VIEW)
	, m_index(0)
{
	setStyle(_T("px:left;py:top;width:fill;height:fill;"));
	setStyle(_T("background-color:#808080;"));
}

CImageView::~CImageView(void)
{
}

void CImageView::setImage (CImagePtr image) {
	if (m_image != image) {
		m_image = image;
		m_index = 0;
		invalidate();
	}
}

void CImageView::drawMe (HDC hdc) {
	if (m_image != NULL) {
		CRect rc = getClientRect();
		HBITMAP hBitmap = m_image->getImage(m_index);

		// Gdiplus::Bitmap bitmap(hBitmap, NULL);
		// assert(bitmap.GetLastStatus() == Gdiplus::Ok);
		// Gdiplus::Graphics g(hdc);

		BITMAP bmp;
		::GetObject(hBitmap, sizeof(bmp), &bmp);
		int w = rc.Width();
		int h = rc.Height();
		int iw = bmp.bmWidth;
		int ih = bmp.bmHeight;
		if (w >= iw && h >= ih) {
			// Gdiplus::RectF rf((rc.Width() - iw) / 2, (rc.Height() - ih) / 2, iw, ih);
			// g.DrawImage(&bitmap, rf, 0, 0, bitmap.GetWidth(), bitmap.GetHeight(), Gdiplus::UnitPixel);
		} else {
			SIZE szArea, szImage;
			szArea.cx = rc.Width();
			szArea.cy = rc.Height();
			szImage.cx = bmp.bmWidth;
			szImage.cy = bmp.bmHeight;
			SIZE sz = CImage::getProperSize(szArea, szImage);

			DWORD tick = ::GetTickCount();
			CRect rcImage(0, 0, bmp.bmWidth, bmp.bmHeight);
			xl::ui::CMemoryDC mdc(hdc, rcImage);
			HBITMAP oldBmp = mdc.SelectBitmap(hBitmap);
			xl::ui::CDCHandle dc(hdc);
			int oldMode = dc.SetStretchBltMode(COLORONCOLOR);//HALFTONE);
			dc.StretchBlt((rc.Width() - sz.cx) / 2, (rc.Height() - sz.cy) / 2, 
				sz.cx, sz.cy, mdc.m_hDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
			dc.SetStretchBltMode(oldMode);
			mdc.SelectBitmap(oldBmp);
			mdc.m_paintWhenDestroy = false;

			tick = ::GetTickCount() - tick;
			ATLTRACE(_T("DRAW COST %dms\n"), tick);
		}
	}
}