#define _WTL_NO_CSTRING
#include <stdlib.h>
#include "libxl/include/ui/Application.h"
#include "libxl/include/ui/ResMgr.h"
#include "libxl/include/Language.h"
#include "libxl/include/utilities.h"
#include "Registry.h"
#include "MainWindow.h"
#include "Settings.h"
#include "resource.h"

#pragma warning (disable:4996)
#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


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
				return hWnd;
			}
			xl::tstring name(p);
			name.trim(_T("\""));
			if (!m_wndMain.setFile(name)) {
				m_wndMain.DestroyWindow();
			}
		}
		return hWnd;
	}

	virtual void preRun () {
#if 0
		auto pLanguage = xl::CLanguage::getInstance();
		xl::tstring shellName = pLanguage->getString(_T("Open with xlview"));
		xl::tstring app = _T("xilou.viewer.1");
		registerApp(app, shellName);
		registerExtAsDefault(_T("jpg"), app);
		// registerExtAsDefault(_T("png"), app);
#endif
	}

	virtual void postRun () {

	}
};

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpstrCmdLine, int nCmdShow) {

	CXLViewApp *pApp = CXLViewApp::getInstance();
	pApp->initialize(hInstance);

	// 1. test for set default ?
	if (_tcsicmp(lpstrCmdLine, _T("/setdefault")) == 0) {
		assert(xl::os_is_vista_or_later());
		if (!xl::os_is_vista_or_later()) {
			return -1;
		}
		if (!launchAssociationOnVista()) {
			::MessageBox(NULL, _T("xlview hasn't been installed in your system...\nPlease run xlview.setup.exe first:)"), NULL, MB_OK | MB_ICONERROR);
		}
		return 0;
	}

	// 2. test for multi-instance
	HANDLE hMutex = ::CreateMutex(NULL, true, _T("xlview::MultiLock - by cyberscorpio@gmail.com"));
	if (hMutex && ::GetLastError() == ERROR_ALREADY_EXISTS) {
		::CloseHandle(hMutex);
		return 0;
	}

	// 3. the normal way

	nCmdShow = SW_MAXIMIZE;// -- maximize the windows seems a good idea
	int nRet = pApp->run(lpstrCmdLine, nCmdShow);

	pApp->cleanup();

	::CloseHandle(hMutex);
	return nRet;
}
