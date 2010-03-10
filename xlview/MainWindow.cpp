#include <assert.h>

#include <Windows.h>
#include <GdiPlus.h>

#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/ResMgr.h"

#include "CommandId.h"
#include "MainWindow.h"
#include "ImageView.h"
#include "Autobar.h"
#include "ThumbnailView.h"
#include "Slider.h"
#include "NavView.h"
#include "NavButton.h"

static const xl::tchar *MAIN_TITLE = _T("xl / view");


void CMainWindow::onCommand (xl::uint id, xl::ui::CControlPtr /*ctrl*/) {
	switch (id) {
	case ID_NAV_NEXT:
		{
			xl::uint new_index = m_currIndex + 1;
			if (new_index == m_cachedImages.size()) {
				new_index = 0;
			}
			setIndex((int)new_index);
		}
		break;
	case ID_NAV_PREV:
		{
			int new_index = m_currIndex;
			if (new_index == 0) {
				new_index = m_cachedImages.size() - 1;
			} else {
				new_index --;
			}
			setIndex(new_index);
		}
		break;
	default:
		break;
	}
}

void CMainWindow::onSlider (xl::uint id, int _min, int _max, int _curr, bool tracking, xl::ui::CControlPtr ctrl) {
	XL_PARAMETER_NOT_USED(id);
	XL_PARAMETER_NOT_USED(_min);
	XL_PARAMETER_NOT_USED(_max);
	XL_PARAMETER_NOT_USED(tracking);
	assert(id == m_slider->getID());

	setIndex(_curr);
}


xl::tstring CMainWindow::onGesture (const xl::tstring &gesture, CPoint ptDown, bool release) {
	if (gesture == _T("canceled")) {
		return _T("timeout");
	}

	CImageView *pView = (CImageView *)m_view.get();
	// test gesture and prev & next
	if (gesture == _T("R")) {
		if (release) {
			xl::uint new_index = m_currIndex + 1;
			if (new_index == m_cachedImages.size()) {
				new_index = 0;
			}
			setIndex((int)new_index);
		}
		return _T("Next");
	}

	if (gesture == _T("L")) {
		if (release) {
			int new_index = m_currIndex;
			if (new_index == 0) {
				new_index = m_cachedImages.size() - 1;
			} else {
				new_index --;
			}
			setIndex(new_index);
		}
		return _T("Prev");
	}

	if (gesture == _T("U")) {
		if (release) {
			pView->showLarger(ptDown);
		}
		return _T("Larger");
	}

	if (gesture == _T("D")) {
		if (release) {
			pView->showSmaller(ptDown);
		}
		return _T("Smaller");
	}

	if (gesture == _T("RDR")) {
		if (release) {
			pView->showRealSize(ptDown);
		}
		return _T("Show original size");
	}

	if (gesture == _T("LDL")) {
		if (release) {
			pView->showSuitable(ptDown);
		}
		return _T("Show suitable size");
	}

	if (gesture == _T("ULR")) {
		if (release) {
			pView->showTop(ptDown);
		}
		return _T("Scroll to top");
	}

	if (gesture == _T("DRL")) {
		if (release) {
			pView->showBottom(ptDown);
		}
		return _T("Scroll to bottom");
	}

	if (gesture == _T("LUD")) {
		if (release) {
			pView->showLeft(ptDown);
		}
		return _T("Scroll to left");
	}

	if (gesture == _T("RUD")) {
		if (release) {
			pView->showRight(ptDown);
		}
		return _T("Scroll to right");
	}

	return xl::ui::CCtrlTarget::onGesture(gesture, ptDown, release);
}


LRESULT CMainWindow::OnCreate (UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL &bHandled) {
	subscribe(this);

	bHandled = false;

	SetWindowText(MAIN_TITLE);

	if (m_ctrlMain == NULL) {
		m_ctrlMain.reset(new xl::ui::CCtrlMain(this, this));
	}

	// gesture
	m_ctrlMain->enableGesture(true);
	m_ctrlMain->setStyle(_T("background:none"));
	xl::ui::CControlPtr gestureCtrl = m_ctrlMain->getGestureCtrl();
	gestureCtrl->setStyle(_T("color:#ff0000"));

	// the view
	CImageView *pView = new CImageView(this);
	m_view = xl::ui::CControlPtr(pView);
	m_ctrlMain->insertChild(m_view);

	// autobar
	xl::ui::CControlPtr navbar(new CAutobar(0, 75, 25, 300, 25));
	m_navbar = navbar;
	navbar->setStyle(_T("margin:0; padding:0; border-top:0 #d0d0d0; py:bottom; width:fill; height:100; float:true; disable:true"));
	m_ctrlMain->insertChild(navbar);

	// thumbnail
	xl::ui::CControlPtr thumbview(new CThumbnailView(this));
	thumbview->setStyle(_T("margin:0; padding:0; width:fill; height:70; background-color:#000000"));
	m_navbar->insertChild(thumbview);

	// slider
	xl::ui::CControlPtr slider(new CSlider());
	m_slider = slider;
	slider->setStyle(_T("margin:0 0; width:fill; height:30;"));
	slider->setStyle(_T("slider:0 0 0;"));
	m_navbar->insertChild(slider);

	// nav view bar
	navbar.reset(new CAutobar(0, 75, 25, 300, 25));
	navbar->setStyle(_T("margin:0 100 110 0; padding:0; px:right; py:bottom; width:140; height:200; float:true; background-color:#000000"));
	// navbar->setStyle(_T("margin:20 0 0 20; px:left; py:top;"));
	m_ctrlMain->insertChild(navbar);

	// nav view
	CNavView *pNavView = new CNavView(this, pView);
	xl::ui::CControlPtr naview(pNavView);
	naview->setStyle(_T("border:1 #ffffff; padding:20 4 4 4;"));
	navbar->insertChild(naview);
	pView->setNavView(pNavView);

	// nav button
	CNavButton *pNavButton = new CNavButton(true);
	pNavButton->setStyle(_T("width:100; margin:100 0"));
	m_ctrlMain->insertChild(xl::ui::CControlPtr(pNavButton));

	pNavButton = new CNavButton(false);
	pNavButton->setStyle(_T("width:100; margin:100 0"));
	m_ctrlMain->insertChild(xl::ui::CControlPtr(pNavButton));

	return TRUE;
}


LRESULT CMainWindow::OnSize (UINT /*msg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL &/*bHandled*/) {
	if (m_ctrlMain && wParam != SIZE_MINIMIZED) {
		CRect rc;
		GetClientRect(rc);
		m_ctrlMain->layout(rc);
	}
	return 0;
}

void CMainWindow::onEvent (CImageManager::IObserver::EVT evt, void *param) {
	xl::ui::CCtrlSlider *pSlider = (xl::ui::CCtrlSlider *)m_slider.get();
	assert(pSlider != NULL);
	assert(m_navbar != NULL);
	switch (evt) {
	case CImageManager::EVT_FILELIST_READY:
		m_navbar->setStyle(_T("disable:false"));
		break;
	case CImageManager::EVT_INDEX_CHANGED:
		{
			assert(param != NULL);
			int _min = 0, _max = m_cachedImages.size() - 1, _curr = *(int *)param;
			assert(_curr == (int)m_currIndex);
			TCHAR buf[128];
			_stprintf_s(buf, 128, _T("slider: %d %d %d; disable:false;"), _min, _max, _curr);
			pSlider->setStyle(buf);

			xl::tstring title = MAIN_TITLE;
			_stprintf_s(buf, 128, _T(" (%d/%d)"), _curr + 1, _max + 1);
			title += _T(" - ") + m_cachedImages[_curr]->getFileName();
			title += buf;
			SetWindowText(title);
		}
		break;
	case CImageManager::EVT_IMAGE_LOADED:
		break;
	case CImageManager::EVT_THUMBNAIL_LOADED:
		break;
	case CImageManager::EVT_I_AM_DEAD:
		break;
	default:
		assert(false);
		break;
	}
}
