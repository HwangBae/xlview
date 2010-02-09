#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/Language.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"
#include "MainWindow.h"

//////////////////////////////////////////////////////////////////////////
// DisplayParameter
DisplayParameter::DisplayParameter ()
	: suitable(true)
	, zoomTo(0)
	, zoomNow(0)
	, srcX(0)
	, srcY(0)
	, realSize(-1, -1)
	, zoomSize(-1, -1)
	, rcView(0, 0, 0, 0)
	, frameIndex(0)
{
}

DisplayParameter::~DisplayParameter () {

}

void DisplayParameter::reset (CRect rc) {
	suitable = true;
	zoomTo = zoomNow = 0;
	srcX = srcY = 0;
	realSize.cx = realSize.cy = -1;
	zoomSize.cx = zoomSize.cy = -1;
	rcView = rc;
	frameIndex = 0;
}

void DisplayParameter::draw (HDC hdc, CImagePtr image) {
	if (rcView.Width() <= 0 || rcView.Height() <= 0) {
		return;
	}

	if (realSize.cx == -1 || realSize.cy == -1) {
		assert(image == NULL);
		xl::CLanguage *pLang = xl::CLanguage::getInstance();
		xl::tstring strLoading = pLang->getString(_T("loading..."));

		xl::ui::CDCHandle dc(hdc);
		xl::ui::CResMgr *pResMgr = xl::ui::CResMgr::getInstance();
		HFONT font = pResMgr->getSysFont();
		HFONT oldFont = dc.SelectFont(font);

		UINT fmt = DT_SINGLELINE | DT_VCENTER | DT_CENTER;
		dc.DrawText(strLoading, strLoading.length(), rcView, fmt);

		dc.SelectFont(oldFont);
		return;
	}

	assert(image != NULL);
	if (suitable) {
		_DrawSuitable(hdc, image);
	} else {
		assert(false); // do later
	}
}

void DisplayParameter::_DrawSuitable (HDC hdc, CImagePtr image) {
	CRect rc = rcView;
	CSize szArea(rc.Width(), rc.Height());
	CSize szImage = image->getImageSize();
	CSize sz = CImage::getSuitableSize(szArea, realSize);

	int x = rc.left + (rc.Width() - sz.cx) / 2;
	int y = rc.top + (rc.Height() - sz.cy) / 2;

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(hdc);
	xl::ui::CDIBSectionHelper selector(image->getImage(0), mdc);

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


//////////////////////////////////////////////////////////////////////////
// static


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
		CDisplayImagePtr image = m_pImageManager->getImage(index);
		assert(image != NULL);
		m_imageZoomed = image->getZoomedImage();
		if (m_imageZoomed != NULL) {
			m_imageZoomed = m_imageZoomed->clone();
		}

		m_imageThumbnail = image->getThumbnail();
		if (m_imageThumbnail != NULL) {
			m_imageThumbnail = m_imageThumbnail->clone();
		}

		m_disp.realSize = image->getRealSize();;
	}
	invalidate();
}

void CImageView::_OnImageLoaded (int index) {
	assert(getLockLevel() > 0); // must be called in lock
	assert(m_pImageManager != NULL);

	if (index == m_pImageManager->getCurrIndex()) {
		CDisplayImagePtr image = m_pImageManager->getImage(index);
		assert(image != NULL);
		m_imageRealSize = image->getRealSizeImage();
		assert(m_imageRealSize != NULL);
		m_disp.realSize = image->getRealSize();

		m_imageThumbnail = image->getThumbnail();
		assert(m_imageThumbnail); // when loaded, the thumbnail is also created
		m_imageThumbnail = m_imageThumbnail->clone();
		invalidate();
	}
}


//////////////////////////////////////////////////////////////////////////

CImageView::CImageView (CImageManager *pImageManager)
	: xl::ui::CControl(ID_VIEW)
	, m_pImageManager(pImageManager)
{
	setStyle(_T("px:left;py:top;width:fill;height:fill;padding:10;"));
	setStyle(_T("background-color:#808080;"));

	m_pImageManager->subscribe(this);
}

CImageView::~CImageView (void) {
}

void CImageView::onSize () {
	assert(m_pImageManager != NULL);
	CRect rc = getClientRect();
	m_pImageManager->onViewSizeChanged(rc);
	lock();
	m_disp.rcView = rc;
	unlock();
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
		_OnImageLoaded(*(int *)param);
		break;
	case CImageManager::EVT_IMAGE_ZOOMED:
		{ // in fact, we don't care this event
			assert(param != NULL);
			int index = *(int *)param;
			if (index == m_pImageManager->getCurrIndex()) {
				assert(false); // the loader thread doesn't load the current image
			}
		}
		break;
	default:
		assert(false);
		break;
	}
}
