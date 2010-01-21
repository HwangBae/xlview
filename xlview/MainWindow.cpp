#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/ResMgr.h"
#include "libxl/include/ui/CtrlButton.h"
#include "libxl/include/ui/CtrlSlider.h"
#include "MainWindow.h"


void CMainWindow::onCommand (xl::uint id, xl::ui::CControlPtr ctrl) {

}

void CMainWindow::onSlider (xl::uint id, int _min, int _max, int _curr, bool tracking, xl::ui::CControlPtr ctrl) {
}


xl::tstring CMainWindow::onGesture (const xl::tstring &gesture, bool release) {
	return xl::ui::CCtrlTarget::onGesture(gesture, release);
}


LRESULT CMainWindow::OnCreate (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	bHandled = false;
	if (m_ctrlMain == NULL) {
		m_ctrlMain.reset(new xl::ui::CCtrlMain(this, this));
	}
	m_ctrlMain->enableGesture(true);

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
