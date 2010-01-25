#include <assert.h>

#include <Windows.h>
#include <GdiPlus.h>

#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/ResMgr.h"
#include "MainWindow.h"
#include "ImageView.h"
#include "Slider.h"


void CMainWindow::onCommand (xl::uint id, xl::ui::CControlPtr ctrl) {

}

void CMainWindow::onSlider (xl::uint id, int _min, int _max, int _curr, bool tracking, xl::ui::CControlPtr ctrl) {
}


xl::tstring CMainWindow::onGesture (const xl::tstring &gesture, CPoint ptDown, bool release) {
	if (gesture == _T("canceled")) {
		return _T("timeout");
	} else if (gesture == _T("UR")) {
		if (release) {
			xl::ui::CControlPtr ctrl = m_ctrlMain->getControlByID(ID_VIEW);
			assert(ctrl);
			CImageView *view = (CImageView *)ctrl.get();
			assert(view);
			view->showNormalSize(ptDown);
		}
		return _T("show normal size");
	} else if (gesture == _T("LD")) {
		if (release) {
			xl::ui::CControlPtr ctrl = m_ctrlMain->getControlByID(ID_VIEW);
			assert(ctrl);
			CImageView *view = (CImageView *)ctrl.get();
			assert(view);
			view->showSuitable(ptDown);
		}
		return _T("show suitable size");
	} else if (gesture == _T("U")) {
		if (release) {
			xl::ui::CControlPtr ctrl = m_ctrlMain->getControlByID(ID_VIEW);
			assert(ctrl);
			CImageView *view = (CImageView *)ctrl.get();
			assert(view);
			view->showLarger(ptDown);
		}
		return _T("show larger");
	} else if (gesture == _T("D")) {
		if (release) {
			xl::ui::CControlPtr ctrl = m_ctrlMain->getControlByID(ID_VIEW);
			assert(ctrl);
			CImageView *view = (CImageView *)ctrl.get();
			assert(view);
			view->showSmaller(ptDown);
		}
		return _T("show smaller");
	}
	return xl::ui::CCtrlTarget::onGesture(gesture, ptDown, release);
}


LRESULT CMainWindow::OnCreate (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	bHandled = false;
	if (m_ctrlMain == NULL) {
		m_ctrlMain.reset(new xl::ui::CCtrlMain(this, this));
	}
	m_ctrlMain->enableGesture(true);
	m_ctrlMain->setStyle(_T("background:none"));
	xl::ui::CControlPtr gestureCtrl = m_ctrlMain->getGestureCtrl();
	gestureCtrl->setStyle(_T("color:#ff0000"));

	CImageView *pView = new CImageView();
	m_ctrlMain->insertChild(xl::ui::CControlPtr(pView));
	
	xl::ui::CControlPtr slider(new CSlider());
	slider->setStyle(_T("margin:0 auto; py:bottom; width:480; float:true;"));
	slider->setStyle(_T("slider:0 100 0;"));
	m_ctrlMain->insertChild(slider);

	xl::ui::CResMgr *pResMgr = xl::ui::CResMgr::getInstance(); // start gdiplus

	// test: load image
	xl::tchar *p = _tgetenv(_T("xlview_test_image"));
	if (!p) {
		bHandled = false;
		MessageBox(_T("Please set env: xlview_test_image to an image"));
		return FALSE;
	}
	xl::tstring file(p);
	DWORD tick = ::GetTickCount();
	CImagePtr image = CImage::loadFromFile(file);
	tick = ::GetTickCount() - tick;
	// ATLTRACE(_T("Load image cost: %dms\n"), tick);
	xl::tchar buf[65];
	_stprintf_s(buf, 64, _T("load image use: %dms"), tick);
	// MessageBox(buf, _T("time:"));
	if (image != NULL) {
		pView->setImage(image);
	}


	return TRUE;
}


LRESULT CMainWindow::OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	if (m_ctrlMain) {
		CRect rc;
		GetClientRect(rc);
		m_ctrlMain->layout(rc);
	}
	return 0;
}
