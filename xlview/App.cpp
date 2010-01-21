#define _WTL_NO_CSTRING
#include "libxl/include/ui/Application.h"
#include "MainWindow.h"

class CXLViewApp : public xl::ui::CApplicationT<CXLViewApp>
{
	CMainWindow m_wndMain;
public:
	virtual HWND createMainWindow (LPCTSTR, int) {
		return m_wndMain.Create(NULL, 0, _T("xl / view"));
	}

	virtual void preRun () {

	}

	virtual void postRun () {

	}
};


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpstrCmdLine, int nCmdShow) {

	CXLViewApp *pApp = CXLViewApp::getInstance();
	pApp->initialize(hInstance);

	int nRet = pApp->run(lpstrCmdLine, nCmdShow);

	pApp->cleanup();

	return nRet;
}
