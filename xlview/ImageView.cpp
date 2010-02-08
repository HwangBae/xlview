#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"
#include "MainWindow.h"

//////////////////////////////////////////////////////////////////////////
// static


//////////////////////////////////////////////////////////////////////////
// protected


//////////////////////////////////////////////////////////////////////////

CImageView::CImageView (CImageManager *pImageManager)
	: xl::ui::CControl(ID_VIEW)
	, m_pImageManager(pImageManager)
	, m_szImage(0, 0)
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

	if (image != NULL) {
		CSize szArea(rc.Width(), rc.Height());
		CSize szImage = image->getImageSize();
		CSize sz = CImage::getSuitableSize(szArea, m_szImage);

		int x = rc.left + (rc.Width() - sz.cx) / 2;
		int y = rc.top + (rc.Height() - sz.cy) / 2;
		
		xl::ui::CDCHandle dc(hdc);
		xl::ui::CDC mdc;
		mdc.CreateCompatibleDC(hdc);
		xl::ui::CDIBSectionHelper selector(image->getImage(0), mdc);

		int oldMode = dc.SetStretchBltMode(stretchMode);
		dc.StretchBlt(x, y, sz.cx, sz.cy, mdc, 0, 0, szImage.cx, szImage.cy, SRCCOPY);
		dc.SetStretchBltMode(oldMode);

		selector.detach();

		dc.TextOut(10, 10, stretchMode == COLORONCOLOR ? _T("ColorOnColor") : _T("HalfTone"));
	}

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
		{
			m_imageRealSize.reset();
			m_imageZoomed.reset();
			m_imageThumbnail.reset();

			assert(param != NULL);
			int index = *(int *)param;
			if (index == m_pImageManager->getCurrIndex()) {
				CDisplayImagePtr image = m_pImageManager->getImage(index);
				assert(image != NULL);
				m_imageZoomed = image->getZoomedImage();
				if (m_imageZoomed != NULL) {
					m_imageZoomed = m_imageZoomed->clone();
					m_szImage = image->getRealSize();
				}
				m_imageThumbnail = image->getThumbnail();
				if (m_imageThumbnail != NULL) {
					m_imageThumbnail = m_imageThumbnail->clone();
				}
			}
		}
		m_imageRealSize.reset();
		invalidate();
		break;
	case CImageManager::EVT_IMAGE_LOADED:
		{
			assert(param != NULL);
			int index = *(int *)param;
			if (index == m_pImageManager->getCurrIndex()) {
				CDisplayImagePtr image = m_pImageManager->getImage(index);
				assert(image != NULL);
				m_imageRealSize = image->getRealSizeImage();
				assert(m_imageRealSize != NULL);
				m_szImage = image->getRealSize();

				m_imageThumbnail = image->getThumbnail();
				if (m_imageThumbnail != NULL) {
					m_imageThumbnail = m_imageThumbnail->clone();
				}
				invalidate();
			}
		}
		break;
	case CImageManager::EVT_IMAGE_ZOOMED:
		{
			assert(param != NULL);
			int index = *(int *)param;
			if (index == m_pImageManager->getCurrIndex()) {
				CDisplayImagePtr image = m_pImageManager->getImage(index);
				assert(image != NULL);
				m_imageZoomed = image->getZoomedImage();
				assert(m_imageRealSize != NULL);
				m_imageZoomed = m_imageZoomed->clone();
				invalidate();
			}
		}
		break;
	default:
		assert(false);
		break;
	}
}
