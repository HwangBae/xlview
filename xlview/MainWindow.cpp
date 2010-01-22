#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/ResMgr.h"
#include "MainWindow.h"
#include "ImageView.h"
#include "Slider.h"


void CMainWindow::onCommand (xl::uint id, xl::ui::CControlPtr ctrl) {

}

void CMainWindow::onSlider (xl::uint id, int _min, int _max, int _curr, bool tracking, xl::ui::CControlPtr ctrl) {
}


xl::tstring CMainWindow::onGesture (const xl::tstring &gesture, bool release) {
	if (gesture == _T("canceled")) {
		return _T("timeout");
	}
	return xl::ui::CCtrlTarget::onGesture(gesture, release);
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

	m_ctrlMain->insertChild(xl::ui::CControlPtr(new CImageView()));
	
	xl::ui::CControlPtr slider(new CSlider());
	slider->setStyle(_T("margin:0 auto; py:bottom; width:480; float:true;"));
	slider->setStyle(_T("slider:0 100 0;"));
	m_ctrlMain->insertChild(slider);

	xl::ui::CResMgr *pResMgr = xl::ui::CResMgr::getInstance(); // start gdiplus

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
