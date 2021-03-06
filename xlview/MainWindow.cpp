#include <assert.h>

#include <Windows.h>
#include <GdiPlus.h>

#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "libxl/include/Language.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/ResMgr.h"

#include "CommandId.h"
#include "resource.h"
#include "Settings.h"
#include "SettingUI.h"
#include "MainWindow.h"
#include "ImageView.h"
#include "Autobar.h"
#include "ThumbnailView.h"
#include "Slider.h"
#include "NavView.h"
#include "InfoView.h"
#include "NavButton.h"
#include "ToolbarButton.h"
#include "Dispatch.h"

static const xl::tchar *MAIN_TITLE = _T("xl / view");


void CMainWindow::onCommand (xl::uint id, xl::ui::CControlPtr /*ctrl*/) {
	switch (id) {
	case ID_NAV_NEXT:
		cmdNext();
		break;
	case ID_NAV_PREV:
		cmdPrev();
		break;
	case ID_NAV_ZOOMIN:
	case ID_NAV_ZOOMOUT:
	case ID_NAV_SWITCH:
		{
			CImageView *pView = (CImageView *)m_view.get();
			assert(pView);
			if (id == ID_NAV_ZOOMIN) {
				pView->showLarger(CPoint(-1, -1));
			} else if (id == ID_NAV_ZOOMOUT) {
				pView->showSmaller(CPoint(-1, -1));
			} else {
				pView->showSwitch(CPoint(-1, -1));
			}
		}
		break;
	case ID_SETTING:
		// launchAssociation();
		{
			m_toolbar->setOpacity(0);
			CSettingDialog dlg(&m_gestureMap);
			dlg.DoModal(m_hWnd);
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
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	if (gesture == _T("canceled")) {
		return pLanguage->getString(_T("GestureTimeout"));
	}

	xl::tstring action = m_gestureMap.onGesture(gesture);
	if (_tcsicmp(action.c_str(), _T("Unknown")) != 0) {
		if (release) {
			m_pDispatch->execute(action, ptDown);
		}
		return pLanguage->getString(_T("Gesture") + action);
	}

	// return xl::ui::CCtrlTarget::onGesture(gesture, ptDown, release);
	return pLanguage->getString(_T("GestureUnknown"));
}


LRESULT CMainWindow::OnCreate (UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL &bHandled) {
	bHandled = false;
	subscribe(this);

	SetWindowText(MAIN_TITLE);

	if (m_ctrlMain == NULL) {
		m_ctrlMain.reset(new xl::ui::CCtrlMain(this, this));
	}

#if 0 // move to file list ready
	// gesture
	m_ctrlMain->enableGesture(true);
	m_ctrlMain->setStyle(_T("background:none"));
	xl::ui::CControlPtr gestureCtrl = m_ctrlMain->getGestureCtrl();
	gestureCtrl->setStyle(_T("color:#ff0000; font-size:16;"));
#endif

	// the view
	CImageView *pView = new CImageView(this);
	m_view = xl::ui::CControlPtr(pView);
	m_ctrlMain->insertChild(m_view);

	// nav button (left and right)
	xl::ui::CControlPtr navbtn(new CNavButton(true));
	navbtn->setStyle(_T("width:200; margin:0 0; disable:true;"));
	m_ctrlMain->insertChild(navbtn);
	m_navbtn[0] = navbtn;

	navbtn.reset(new CNavButton(false));
	navbtn->setStyle(_T("width:200; margin:0 0; disable:true;"));
	m_ctrlMain->insertChild(navbtn);
	m_navbtn[1] = navbtn;

	// toolbar
	xl::ui::CControlPtr toolbar(new CAutobar(0, 75, 25, 300, 25));
	m_toolbar = toolbar;
	toolbar->setStyle(_T("border-top:0 #d0d0d0; py:top; height:48; float:true; disable:true"));
	toolbar->setStyle(_T("background-color:#cccccc"));
	m_ctrlMain->insertChild(toolbar);

	// toobar -> layoutor
	toolbar.reset(new xl::ui::CControl());
	toolbar->setStyle(_T("margin:0 auto; padding:0 2; float:true; py:top; width:208;"));
	m_toolbar->insertChild(toolbar);

	// toolbar -> layoutor -> buttons
	xl::ui::CControlPtr button(new CToolbarButton(ID_NAV_ZOOMIN, _T(""), IDB_ZOOMIN, true));
	button->setStyle(_T("margin:8 16; width:32; height:32;"));
	toolbar->insertChild(button);

	button.reset(new CToolbarButton(ID_NAV_ZOOMOUT, _T(""), IDB_ZOOMOUT, true));
	button->setStyle(_T("margin:8 16 8 0; width:32; height:32;"));
	toolbar->insertChild(button);

	button.reset(new CToolbarButton(ID_NAV_SWITCH, _T(""), IDB_SWITCH, true));
	button->setStyle(_T("margin:8 16 8 0; width:32; height:32;"));
	toolbar->insertChild(button);

	button.reset(new CToolbarButton(ID_SETTING, _T(""), IDB_SETTING, true));
	button->setStyle(_T("margin:8 16 8 0; width:32; height:32;"));
	toolbar->insertChild(button);

	// navbar
	xl::ui::CControlPtr navbar(new CAutobar(0, 75, 25, 300, 25));
	m_navbar = navbar;
	navbar->setStyle(_T("margin:0; padding:0; border-top:0 #d0d0d0; py:bottom; width:fill; height:120; float:true; disable:true"));
	m_ctrlMain->insertChild(navbar);

	// navbar -> infoview
	CInfoView *pInfoView = new CInfoView(this);
	xl::ui::CControlPtr infoview(pInfoView);
	infoview->setStyle(_T("margin:0; padding:2 0; width:fill; height:20; background-color:#000000"));
	m_navbar->insertChild(infoview);
	pView->setInfoView(pInfoView);

	// navbar -> thumbnail
	xl::ui::CControlPtr thumbview(new CThumbnailView(this));
	thumbview->setStyle(_T("margin:0; padding:0; width:fill; height:70; background-color:#000000"));
	m_navbar->insertChild(thumbview);

	// navbar -> slider
	xl::ui::CControlPtr slider(new CSlider());
	m_slider = slider;
	slider->setStyle(_T("margin:0 0; width:fill; height:30;"));
	slider->setStyle(_T("slider:0 0 0;"));
	m_navbar->insertChild(slider);

	// naviewbar
	navbar.reset(new CAutobar(0, 75, 25, 300, 25));
	navbar->setStyle(_T("margin:0 210 100 0; padding:0; px:right; py:bottom; width:140; height:200; float:true; background-color:#000000; disable:true"));
	m_ctrlMain->insertChild(navbar);
	m_naview = navbar;

	// naviewbar -> naview
	CNavView *pNavView = new CNavView(this, pView);
	xl::ui::CControlPtr naview(pNavView);
	naview->setStyle(_T("border:1 #ffffff; padding:20 4 4 4;"));
	navbar->insertChild(naview);
	pView->setNavView(pNavView);


	// Dispatch
	m_pDispatch = new CDispatch(this, pView);

	return TRUE;
}

LRESULT CMainWindow::OnDestroy (UINT, WPARAM, LPARAM, BOOL &bHandled) {
	bHandled = FALSE;
	unsubscribe(this);

	delete m_pDispatch;
	m_pDispatch = NULL;
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

LRESULT CMainWindow::OnKeyDown (UINT, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	XL_PARAMETER_NOT_USED(lParam);
	assert(m_pDispatch != NULL);
	switch (wParam) {
	case 'A':
	case 'a':
	case VK_LEFT:
		m_pDispatch->execute(_T("showPrev"));
		break;
	case 'D':
	case 'd':
	case VK_RIGHT:
	case VK_SPACE:
		m_pDispatch->execute(_T("showNext"));
		break;
	case VK_UP:
		m_pDispatch->execute(_T("showLarger"));
		break;
	case VK_DOWN:
		m_pDispatch->execute(_T("showSmaller"));
		break;
	case 'S':
	case 's':
	case VK_OEM_5: // '\' and '|'
		m_pDispatch->execute(_T("showSwitch"));
		break;
	default:
		bHandled = false;
		break;
	}
	return 0;
}

LRESULT CMainWindow::OnXLViewExit (UINT, WPARAM, LPARAM, BOOL &) {
	DestroyWindow();
	return 0;
}

void CMainWindow::onEvent (CImageManager::IObserver::EVT evt, void *param) {
	xl::ui::CCtrlSlider *pSlider = (xl::ui::CCtrlSlider *)m_slider.get();
	assert(pSlider != NULL);
	assert(m_navbar != NULL);
	switch (evt) {
	case CImageManager::EVT_FILELIST_READY:
		m_navbar->setStyle(_T("disable:false"));
		m_toolbar->setStyle(_T("disable:false"));
		m_navbtn[0]->setStyle(_T("disable:false"));
		m_navbtn[1]->setStyle(_T("disable:false"));
		m_naview->setStyle(_T("disable:false"));
		// gestory
		m_ctrlMain->enableGesture(true);
		m_ctrlMain->setStyle(_T("background:none"));
		m_ctrlMain->getGestureCtrl()->setStyle(_T("padding:4 0; color:#ff0000; font-size:16;"));// font-weight:bold;"));
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
			// title += _T(" - ") + m_cachedImages[_curr]->getFileName();
			title += _T(" - ") + file_get_name(m_cachedImages[_curr]->getFileName());
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

// command functions
void CMainWindow::cmdExit () {
	PostMessage(WM_XLVIEW_EXIT, 0, 0);
}

void CMainWindow::cmdPrev () {
	int new_index = m_currIndex;
	if (new_index == 0) {
		new_index = m_cachedImages.size() - 1;
	} else {
		new_index --;
	}
	setIndex(new_index);
}

void CMainWindow::cmdNext () {
	xl::uint new_index = m_currIndex + 1;
	if (new_index == m_cachedImages.size()) {
		new_index = 0;
	}
	setIndex((int)new_index);
}

void CMainWindow::cmdMinimize () {
	ShowWindow(SW_MINIMIZE);
}

void CMainWindow::cmdMaximizeOrRestore () {
	if (IsZoomed()) {
		ShowWindow(SW_RESTORE);
	} else {
		ShowWindow(SW_MAXIMIZE);
	}
}