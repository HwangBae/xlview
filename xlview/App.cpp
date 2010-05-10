#define _WTL_NO_CSTRING
#include <stdlib.h>
#include "libxl/include/ui/Application.h"
#include "libxl/include/ui/ResMgr.h"
#include "MainWindow.h"
#include "resource.h"

#pragma warning (disable:4996)

class CXLViewApp : public xl::ui::CApplicationT<CXLViewApp>
{
	CMainWindow m_wndMain;
public:

	virtual HWND createMainWindow (LPCTSTR lpctCmdLine, int) {
		HWND hWnd = m_wndMain.Create(NULL, 0, _T(""));
		if (hWnd != NULL) {
			xl::ui::CResMgr *rm = xl::ui::CResMgr::getInstance();
			HICON icon = rm->getIcon(IDI_XLVIEW);
			if (icon != NULL) {
				::SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
				::SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
			}

			const xl::tchar *p = lpctCmdLine;
			if (_tcslen(p) == 0) {
				p = _tgetenv(_T("xlview_test_image"));
			}
			if (!p) {
				::MessageBox(hWnd, _T("Please set env: xlview_test_image to an image"), 0, MB_OK);
				return hWnd;
			}
			// p = _T("C:\\Users\\ddh\\Pictures\\wp\\4.jpg");
			// p = _T("D:\\test_images\\1\\1.jpg");
			// p = _T("I:\\test_images\\1266987653527.png");
			xl::tstring name(p);
			name.trim(_T("\""));
			if (!m_wndMain.setFile(name)) {
				m_wndMain.DestroyWindow();
			}
		}
		return hWnd;
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
