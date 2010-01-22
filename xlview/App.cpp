#define _WTL_NO_CSTRING
#include <stdlib.h>
#include "libxl/include/ui/Application.h"
#include "MainWindow.h"

#pragma warning (disable:4996)

class CXLViewApp : public xl::ui::CApplicationT<CXLViewApp>
{
	CMainWindow m_wndMain;
public:

	virtual HWND createMainWindow (LPCTSTR, int) {
		xl::tstring title = _T("xl / view");
		return m_wndMain.Create(NULL, 0, title);
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
