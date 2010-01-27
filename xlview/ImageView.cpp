#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "ImageView.h"


//////////////////////////////////////////////////////////////////////////
// protected

void CImageView::_ResetParameter () {
	m_suitable = true;
	m_zoom = 0;
	m_image.reset();
}

void CImageView::_OnIndexChanged () {
	int newIndex = m_pImageManager->getCurrIndex();
	if (newIndex != m_currIndex) {
		m_currIndex = newIndex;
		_ResetParameter();
		invalidate();
	}
}


//////////////////////////////////////////////////////////////////////////

CImageView::CImageView (CImageManager *pImageManager)
	: xl::ui::CControl(ID_VIEW)
	, m_pImageManager(pImageManager)
	, m_currIndex(-1)
{
	assert(m_pImageManager != NULL);
	m_currIndex = m_pImageManager->getCurrIndex();

	_ResetParameter();

	setStyle(_T("px:left;py:top;width:fill;height:fill;padding:10;"));
	setStyle(_T("background-color:#808080;"));
}

CImageView::~CImageView (void) {
}

void CImageView::onSize () {
}

void CImageView::drawMe (HDC hdc) {
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
	switch (evt) {
	case CImageManager::EVT_INDEX_CHANGED:
		_OnIndexChanged();
		break;
	default:
		break;
	}
}