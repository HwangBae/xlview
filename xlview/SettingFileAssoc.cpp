#include <assert.h>
#include "libxl/include/Language.h"
#include "libxl/include/ui/ResMgr.h"
#include "libxl/include/ui/Gdi.h"
#include "Settings.h"
#include "SettingFileAssoc.h"

void CFileAssociationDialogVista::_LaunchSysFileAssociationDialog () {
	if (!launchAssociationOnVista()) {
		HWND hStatic = GetDlgItem(IDC_STATIC_REINSTALL);
		assert(hStatic != NULL);
		xl::tchar prompt[MAX_PATH];
		::GetWindowText(hStatic, prompt, MAX_PATH);

		MessageBox(prompt, NULL, MB_OK | MB_ICONERROR);
	}
}

CFileAssociationDialogVista::CFileAssociationDialogVista () {
}

CFileAssociationDialogVista::~CFileAssociationDialogVista () {
}

LRESULT CFileAssociationDialogVista::OnInitDialog (UINT, WPARAM, LPARAM, BOOL &) {
	HWND hStatic = GetDlgItem(IDC_STATIC_REINSTALL);
	assert(hStatic != NULL);
	DWORD dwStyle = ::GetWindowLong(hStatic, GWL_STYLE);
	dwStyle |= SS_CENTER;
	::SetWindowLong(hStatic, GWL_STYLE, dwStyle);
	HWND hButton = GetDlgItem(IDC_BUTTON_FILEASSOC);
	assert(hButton);

	if (!isAppRegistered()) {
		::ShowWindow(hStatic, SW_SHOW);
		::ShowWindow(hButton, SW_HIDE);
	} else {
		::ShowWindow(hStatic, SW_HIDE);
		::ShowWindow(hButton, SW_SHOW);
	}

	xl::tchar text[MAX_PATH];
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	HWND hWnds[] = {hStatic, hButton};
	for (int i = 0; i < COUNT_OF(hWnds); ++ i) {
		HWND hWnd = hWnds[i];
		::GetWindowText(hWnd, text, MAX_PATH);
		xl::tstring lang = pLanguage->getString(text);
		::SetWindowText(hWnd, lang.c_str());
	}

	return TRUE;
}

LRESULT CFileAssociationDialogVista::OnSize (UINT, WPARAM, LPARAM, BOOL &) {
	HWND hStatic = GetDlgItem(IDC_STATIC_REINSTALL);
	assert(hStatic != NULL);
	HWND hButton = GetDlgItem(IDC_BUTTON_FILEASSOC);
	assert(hButton);

	CRect rc;
	GetClientRect(&rc);
	rc.DeflateRect(8, 20);

	CRect rcStatic;
	::GetClientRect(hStatic, &rcStatic);

	CRect rcButton;
	::GetClientRect(hButton, &rcButton);

	if (!isAppRegistered()) {
		::MoveWindow(hStatic, rc.left, rc.top, rc.Width(), rc.Height(), TRUE);
	} else {
		int x = (rc.Width() - rcButton.Width()) / 2;
		int y = (rc.Height() - rcButton.Height()) / 2;
		::MoveWindow(hButton, x, y, rcButton.Width(), rcButton.Height(), TRUE);
	}

	return 0;
}

LRESULT CFileAssociationDialogVista::OnCommand (UINT, WPARAM wParam, LPARAM, BOOL &bHandled) {
	WORD code = HIWORD(wParam);
	WORD id = LOWORD(wParam);

	switch (id) {
	case IDC_BUTTON_FILEASSOC:
		_LaunchSysFileAssociationDialog();
		break;
	default:
		bHandled = false;
		break;
	}

	return 0;
}


LRESULT CFileAssociationDialogVista::OnEraseBkGnd (UINT, WPARAM, LPARAM, BOOL &) {
	return TRUE;
}

LRESULT CFileAssociationDialogVista::OnCtlColorStatic (UINT, WPARAM, LPARAM, BOOL &) {
	return (LRESULT)::GetStockObject(NULL_BRUSH);
}
