#include <assert.h>
#include <Windows.h>
#include "libxl/include/utilities.h"
#include "libxl/include/fs.h"
#include "libxl/include/Language.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/ResMgr.h"
#include "MainWindow.h"
#include "InfoView.h"

CInfoView::CInfoView (CImageManager *pImageManager)
	: m_pImageManager(pImageManager)
	, m_index(-1)
	, m_szImage(-1, -1)
	, m_szDisplay(-1, -1)
{
	assert(m_pImageManager != NULL);
	m_pImageManager->subscribe(this);
}

CInfoView::~CInfoView () {
}

void CInfoView::onDetach () {
}

void CInfoView::drawMe (HDC hdc) {
	xl::ui::CResMgr *pResMgr = xl::ui::CResMgr::getInstance();
	HFONT font = pResMgr->getSysFont(0, xl::ui::CResMgr::FS_BOLD);

	xl::ui::CDCHandle dc(hdc);
	COLORREF oldColor = dc.SetTextColor(RGB(255,255,255));
	int oldMode = dc.SetBkMode(TRANSPARENT);
	HFONT oldFont = dc.SelectFont(font);

	CRect rc = getClientRect();
	DWORD flags = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
	if (m_szImage.cx == -1 || m_szImage.cy == -1) {
		dc.DrawText(m_fileName.c_str(), m_fileName.length(), rc, flags);
	} else {
		xl::tchar text[MAX_PATH + 32];
		if (m_szDisplay.cx == -1 || m_szDisplay.cy == -1) {
			_stprintf_s(text, MAX_PATH + 32, _T("%s %dx%d"), m_fileName.c_str(), m_szImage.cx, m_szImage.cy);
		} else {
			assert(m_szImage.cx != 0);
			double x = m_szImage.cx != 0 ? (double)m_szImage.cx : 0.000001;
			double ratio_f = (double)m_szDisplay.cx / x;
			int ratio = (int)(ratio_f * 100);
			if (ratio == 0) {
				ratio = 1;
			}
			_stprintf_s(text, MAX_PATH + 32, _T("%s %dx%d (%d%%)"), m_fileName.c_str(),
				m_szImage.cx, m_szImage.cy, ratio);
		}
		dc.DrawText(text, -1, rc, flags);
	}

	dc.SelectFont(oldFont);
	dc.SetBkMode(oldMode);
	dc.SetTextColor(oldColor);
}

void CInfoView::onEvent (EVT evt, void *param) {
	assert(m_pImageManager->getLockLevel() > 0);

	switch (evt) {
	case CImageManager::EVT_INDEX_CHANGED:
		assert(param);
		{
			int index = *(int *)param;
			if (index != m_index && index == m_pImageManager->getCurrIndex()) {
				m_index = index;
				m_fileName = xl::file_get_name(m_pImageManager->getCurrentFileName());
				m_szImage = CSize(-1, -1);
				m_szDisplay = CSize(-1, -1);
				invalidate();
			}
		}
		break;
	case CImageManager::EVT_IMAGE_LOADED:
		assert(param);
		if (m_index == m_pImageManager->getCurrIndex()) {
			CImagePtr image = *(CImagePtr *)param;
			m_szImage = image->getImageSize();
			invalidate();
		}
		break;
	case CImageManager::EVT_THUMBNAIL_LOADED:
		assert(param);
		break;
	case CImageManager::EVT_FILELIST_READY:
		break;
	case CImageManager::EVT_I_AM_DEAD:
		break;
	default:
		assert(false);
		break;
	}
}

void CInfoView::onDisplayInfoChanged (CSize szDisplay) {
	if (m_index != m_pImageManager->getCurrIndex()) {
		m_index = m_pImageManager->getCurrIndex();
		m_fileName = xl::file_get_name(m_pImageManager->getCurrentFileName());
		CCachedImagePtr img = m_pImageManager->getCurrentCachedImage();
		assert(img != NULL);
		if (img != NULL) {
			m_szImage = img->getImageSize();
		}
	}

	m_szDisplay = szDisplay;

	invalidate();
}