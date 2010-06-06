#include "libxl/include/Language.h"
#include "SettingUI.h"

CSettingDialog::CSettingDialog () {
}

CSettingDialog::~CSettingDialog () {
}


LRESULT CSettingDialog::OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();

	::SetWindowText(m_hWnd, pLanguage->getString(_T("Settings")).c_str());

	HWND hWnd = GetDlgItem(IDOK);
	if (hWnd != NULL) {
		::SetWindowText(hWnd, pLanguage->getString(_T("OK")).c_str());
	}

	hWnd = GetDlgItem(IDCANCEL);
	if (hWnd != NULL) {
		::SetWindowText(hWnd, pLanguage->getString(_T("Cancel")).c_str());
	}

	hWnd = GetDlgItem(IDC_SETTING_TAB);
	if (hWnd != NULL) {
		TCHAR text[MAX_PATH];
		TCITEM tie;
		memset (&tie, 0, sizeof(tie));
		tie.mask = TCIF_TEXT;
		tie.iImage = -1;
		tie.pszText = text;

		TCHAR *names[] = {_T("Gesture"), _T("FileAssociation"), _T("About"),};
		for (size_t i = 0; i < COUNT_OF(names); ++ i) {
			xl::tstring name = pLanguage->getString(names[i]);
			memset(text, 0, sizeof(text));
			_tcsncpy(text, name.c_str(), MAX_PATH - 1);
			TabCtrl_InsertItem(hWnd, i, &tie);
		}
	}

	return TRUE;
}

LRESULT CSettingDialog::OnCommand (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	WORD id = LOWORD(wParam);
	switch (id) {
	case IDOK:
	case IDCANCEL:
		EndDialog(id);
		break;
	default:
		break;
	}
	return 0;
}