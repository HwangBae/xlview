#include <assert.h>
#include <Windows.h>
#include <GdiPlus.h>
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"

//////////////////////////////////////////////////////////////////////////
// 
// static double zoomLevels[] = {
// };

//////////////////////////////////////////////////////////////////////////
bool CImageView::_CheckCondition () {
	if (!m_image || m_image->getImageCount() == 0) {
		return false;
	}
	return true;
}

void CImageView::_CalacuteDisplayParameter () {
	if (!_CheckCondition()) {
		return;
	}
	
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

		m_imgW = sz.cx;
		m_imgH = sz.cy;
		m_dstX = (rc.Width() - sz.cx) / 2;
		m_dstY = (rc.Height() - sz.cy) / 2;
	} else {
		m_imgW = (int)(imageSize.cx * m_zoomFactor);
		m_imgH = (int)(imageSize.cy * m_zoomFactor);

		int x = (rc.Width() - m_imgW) / 2;
		int y = (rc.Height() - m_imgH) / 2;
		if (x < 0) {
			x = 0;
		}
		if (y < 0) {
			y = 0;
		}
		m_dstX = x;
		m_dstY = y;

		if (m_dstX == 0 && m_srcX + rc.Width() > m_imgW) {
			m_srcX = m_imgW - rc.Width();
		}

		if (m_dstY == 0 && m_srcY + rc.Height() > m_imgH) {
			m_srcY = m_imgH - rc.Height();
		}

		if (m_srcX < 0) {
			m_srcX = 0;
		}

		if (m_srcY < 0) {
			m_srcY = 0;
		}
	}

}

void CImageView::_CreateSourceBitmap () {
	DWORD tick = ::GetTickCount();
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

	if (w == sz.cx && h == sz.cy) {
		m_pMemoryDC->BitBlt(0, 0, w, h, mdc.m_hDC, 0, 0, SRCCOPY);
	} else {
		int oldMode = m_pMemoryDC->SetStretchBltMode(COLORONCOLOR);//HALFTONE);
		m_pMemoryDC->StretchBlt(0, 0, w, h, mdc.m_hDC, 0, 0, sz.cx, sz.cy, SRCCOPY);
		m_pMemoryDC->SetStretchBltMode(oldMode);
	}

	mdc.SelectBitmap(oldMBmp);
	tick = ::GetTickCount() - tick;
	ATLTRACE(_T("_CreateSourceBitmap cost %dms\n"), tick);
}

SIZE CImageView::_BeforeZoom (CPoint pt) {
	CRect rc = getClientRect();
	int x = pt.x - rc.left - m_dstX + m_srcX;
	x = (int)(x / m_zoomFactor);
	int y = pt.y - rc.top - m_dstY + m_srcY;
	y = (int)(y / m_zoomFactor);
	SIZE sz;
	sz.cx = x;
	sz.cy = y;
	return sz;
}

void CImageView::_AfterZoom (CPoint pt, SIZE sz) {
	int x = sz.cx;
	int y = sz.cy;
	CRect rc = getClientRect();
	if (rc.Width() < m_imgW) {
		x = (int)(x * m_zoomFactor);
		m_srcX = x - pt.x + rc.left + m_dstX;
	}

	if (rc.Height() < m_imgH) {
		y = (int)(y * m_zoomFactor);
		m_srcY = y - pt.y + rc.top + m_dstY;
	}
	_CalacuteDisplayParameter();
}


//////////////////////////////////////////////////////////////////////////

CImageView::CImageView(void)
	: xl::ui::CControl(ID_VIEW)
	, m_index(0)
{
	setStyle(_T("px:left;py:top;width:fill;height:fill;padding:10;"));
	setStyle(_T("background-color:#808080;"));
}

CImageView::~CImageView(void)
{
}

void CImageView::setImage (CImagePtr image) {
	if (m_image != image) {
		m_image = image;
		m_index = 0;
		showSuitable(CPoint());
	}
}

void CImageView::showNormalSize (CPoint pt) {
	if (!_CheckCondition()) {
		return;
	}

	SIZE sz = m_image->getImageSize();
	if (m_suitable == false && m_imgW == sz.cx && m_imgH == sz.cy) {
		return;
	}

	SIZE szAdjust = _BeforeZoom(pt);

	m_suitable = false;
	m_zoomFactor = 1.0;
	_CalacuteDisplayParameter();
	_CreateSourceBitmap();
	invalidate();

	_AfterZoom(pt, szAdjust);
}

void CImageView::showSuitable (CPoint) {
	if (!_CheckCondition()) {
		return;
	}

	if (m_suitable == true) {
		return;
	}

	m_suitable = true;
	_CalacuteDisplayParameter();
	_CreateSourceBitmap();
	invalidate();
}

void CImageView::showLarger (CPoint pt) {
	if (!_CheckCondition()) {
		return;
	}
	SIZE szAdjust = _BeforeZoom(pt);

	m_suitable = false;
	m_zoomFactor = m_zoomFactor * 1.1;
	_CalacuteDisplayParameter();
	_CreateSourceBitmap();
	invalidate();

	_AfterZoom(pt, szAdjust);
}

void CImageView::showSmaller(CPoint pt) {
	if (!_CheckCondition()) {
		return;
	}

	SIZE szAdjust = _BeforeZoom(pt);

	m_suitable = false;
	m_zoomFactor = m_zoomFactor / 1.1;
	_CalacuteDisplayParameter();
	_CreateSourceBitmap();
	invalidate();

	_AfterZoom(pt, szAdjust);
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

		xl::ui::CDCHandle dc(hdc);
		int drawW = iw - m_srcX;
		if (drawW > rc.Width()) {
			drawW = rc.Width();
		}
		int drawH = iw - m_srcY;
		if (drawH > rc.Height()) {
			drawH = rc.Height();
		}
		dc.BitBlt(
		          rc.left + m_dstX, rc.top + m_dstY, 
		          drawW, drawH,
		          m_pMemoryDC->m_hDC, 
		          m_srcX, m_srcY, 
		          SRCCOPY
		         );
	}
}

void CImageView::onLButtonDown (CPoint pt, xl::uint key) {
	if (!_CheckCondition()) {
		return;
	}
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
	if (!_CheckCondition()) {
		return;
	}

	xl::ui::CCtrlMain *pCtrlMain = _GetMainCtrl();
	assert(pCtrlMain);
	if (pCtrlMain->getCaptureCtrl() == shared_from_this()) {
		_Capture(false);
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	}
}

void CImageView::onMouseMove (CPoint pt, xl::uint key) {
	if (!_CheckCondition()) {
		return;
	}
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
		} else if (rc.Width() >= iw) {
			newX = 0;
		} else if (newX + rc.Width() > iw) {
			newX = iw - rc.Width();
		}

		if (newY < 0) {
			newY = 0;
		} else if (rc.Height() >= ih) {
			newY = 0;
		} else if (newY + rc.Height() > ih) {
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
	if (!_CheckCondition()) {
		return;
	}
	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
}
