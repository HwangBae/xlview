#include <assert.h>
#include <Windows.h>
#include <GdiPlus.h>
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"

#define CHECK_CONDITION do {\
		if (!m_image || m_image->getImageCount() == 0) { \
			return;\
		} \
	}while (0)

//////////////////////////////////////////////////////////////////////////
void CImageView::_CalacuteDisplayParameter () {
	CHECK_CONDITION;
	if (_GetMainCtrl() == NULL) {
		return;
	}

	SIZE imageSize = m_image->getImageSize();
	int w = imageSize.cx;
	int h = imageSize.cy;
	assert(w * h != 0);
	CRect rc = getClientRect();
	
	if (m_suitable) {
		SIZE szArea, szImage;
		szArea.cx = rc.Width();
		szArea.cy = rc.Height();
		szImage.cx = w;
		szImage.cy = h;
		SIZE sz = CImage::getSuitableSize(szArea, szImage);
		m_srcX = m_srcY = 0;
		m_zoomFactor = (double)sz.cx / (double)w;
	} else {
		int iw = (int)(imageSize.cx * m_zoomFactor);
		int ih = (int)(imageSize.cy * m_zoomFactor);
#if 0
		if ((int)m_srcX + rc.Width() > iw && rc.Width() < iw) {
			m_srcX = iw - rc.Width();
		}

		if ((int)m_srcY + rc.Height() > ih && rc.Height() < ih) {
			m_srcY = ih - rc.Height();
		}
#endif
	}
}

void CImageView::_CreateSourceBitmap () {
	if (m_pMemoryDC) {
		m_pMemoryDC.reset();
	}

	if (_GetMainCtrl() == NULL) {
		return;
	}

	if (!m_image || m_image->getImageCount() == 0) {
		return;
	}

	SIZE sz = m_image->getImageSize();
	int w = (int)(sz.cx * m_zoomFactor);
	int h = (int)(sz.cy * m_zoomFactor);
	if (w <= 0) {
		w = 1;
	}
	if (h <= 0) {
		h = 1;
	}
	CRect rcMem(0, 0, w, h);

	HWND hWnd = _GetMainCtrl()->getHWND();
	assert(hWnd != NULL);
	HDC hdc = ::GetDC(hWnd);
	m_pMemoryDC.reset(new xl::ui::CMemoryDC(hdc, rcMem, false));
	::ReleaseDC(hWnd, hdc);
	hdc = m_pMemoryDC->m_hDC;

	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(hdc);
	HBITMAP oldMBmp = mdc.SelectBitmap(m_image->getImage(0));

	int oldMode = m_pMemoryDC->SetStretchBltMode(COLORONCOLOR);//HALFTONE);
	m_pMemoryDC->StretchBlt(0, 0, w, h, mdc.m_hDC, 0, 0, sz.cx, sz.cy, SRCCOPY);
	m_pMemoryDC->SetStretchBltMode(oldMode);

	mdc.SelectBitmap(oldMBmp);
}


//////////////////////////////////////////////////////////////////////////

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
		showSuitable();
	}
}

void CImageView::showNormalSize () {
	CHECK_CONDITION;

	if (m_suitable == false && abs(m_zoomFactor - 1.0) < 0.001) {
		return;
	}
	m_suitable = false;
	m_zoomFactor = 1.0;
	_CalacuteDisplayParameter();
	_CreateSourceBitmap();
	invalidate();
}

void CImageView::showSuitable () {
	CHECK_CONDITION;

	if (m_suitable == true) {
		return;
	}

	m_suitable = true;
	_CalacuteDisplayParameter();
	_CreateSourceBitmap();
	invalidate();
}

void CImageView::showLarger () {
	CHECK_CONDITION;
	m_suitable = false;
	m_zoomFactor = m_zoomFactor * 1.2;
	_CalacuteDisplayParameter();
	_CreateSourceBitmap();
	invalidate();
}

void CImageView::showSmaller() {
	CHECK_CONDITION;
	m_suitable = false;
	m_zoomFactor = m_zoomFactor / 1.2;
	_CalacuteDisplayParameter();
	_CreateSourceBitmap();
	invalidate();
}

void CImageView::onSize () {
	_CalacuteDisplayParameter();
	if (m_suitable) {
		_CreateSourceBitmap();
	} else {
		invalidate();
	}
}

void CImageView::drawMe (HDC hdc) {
	if (m_pMemoryDC != NULL) {
		CRect rc = getClientRect();
		SIZE imageSize = m_image->getImageSize();
		HBITMAP hBitmap = m_image->getImage(m_index);
		int w = rc.Width();
		int h = rc.Height();

		int iw = (int)(imageSize.cx * m_zoomFactor);
		int ih = (int)(imageSize.cy * m_zoomFactor);

		int x = (rc.Width() - iw) / 2;
		int y = (rc.Height() - ih) / 2;
		if (x < 0) {
			// x = m_srcX;
			x = 0;
		}
		if (y < 0) {
			// y = m_srcY;
			y = 0;
		}

		CRect rect(x, y, iw, ih);
		xl::ui::CDCHandle dc(hdc);
		dc.BitBlt(x, y, iw - m_srcX, ih - m_srcY, m_pMemoryDC->m_hDC, m_srcX, m_srcY, SRCCOPY);
	}
}

void CImageView::onLButtonDown (CPoint pt, xl::uint key) {
	CHECK_CONDITION;
	SIZE sz = m_image->getImageSize();
	int iw = (int)(sz.cx * m_zoomFactor);
	int ih = (int)(sz.cy * m_zoomFactor);
	CRect rc = getClientRect();
	if (iw <= rc.Width() && ih <= rc.Height()) {
		return;
	}

	_Capture(true);
	m_ptGrab = pt;
}

void CImageView::onLButtonUp (CPoint pt, xl::uint key) {
	CHECK_CONDITION;

	xl::ui::CCtrlMain *pCtrlMain = _GetMainCtrl();
	assert(pCtrlMain);
	if (pCtrlMain->getCaptureCtrl() == shared_from_this()) {
		_Capture(false);
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	}
}

void CImageView::onMouseMove (CPoint pt, xl::uint key) {
	CHECK_CONDITION;
	xl::ui::CCtrlMain *pCtrlMain = _GetMainCtrl();
	assert(pCtrlMain);
	if (pCtrlMain->getCaptureCtrl() == shared_from_this()) {
		::SetCursor(::LoadCursor(NULL, IDC_HAND));

		SIZE sz = m_image->getImageSize();
		int iw = (int)(sz.cx * m_zoomFactor);
		int ih = (int)(sz.cy * m_zoomFactor);

		CRect rc = getClientRect();
		int offX = pt.x - m_ptGrab.x;
		int offY = pt.y - m_ptGrab.y;

		int newX = m_srcX - offX;
		int newY = m_srcY - offY;
		if (newX < 0) {
			newX = 0;
		} else if (newX + rc.Width() > iw && iw > rc.Width()) {
			newX = iw - rc.Width();
		}

		if (newY < 0) {
			newY = 0;
		} else if (newY + rc.Height() > ih && ih > rc.Height()) {
			newY = ih - rc.Height();
		}

		if (newX != m_srcX || newY != m_srcY) {
			m_srcX = newX;
			m_srcY = newY;
			m_ptGrab = pt;
			invalidate();
		}
	}
}

void CImageView::onLostCapture () {
	CHECK_CONDITION;
	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
}
