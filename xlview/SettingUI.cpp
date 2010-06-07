#include "libxl/include/Language.h"
#include "SettingUI.h"

///////////////////////////////////////////////////////////////////////
// static
static TCHAR *tabNames[] = {_T("Gesture"), _T("FileAssociation"), _T("About"),};


///////////////////////////////////////////////////////////////////////
// The gesture dialog
CGestureDialog::CGestureDialog () {
}

CGestureDialog::~CGestureDialog () {
}

LRESULT CGestureDialog::OnInitDialog (UINT, WPARAM, LPARAM, BOOL &) {
	return TRUE;
}

LRESULT CGestureDialog::OnCommand (UINT, WPARAM, LPARAM, BOOL &) {
	return 0;
}

LRESULT CGestureDialog::OnEraseBkGnd (UINT, WPARAM, LPARAM, BOOL &) {
	return TRUE;
}


///////////////////////////////////////////////////////////////////////
// The whole setting dialog

void CSettingDialog::_CreateTabs () {
	DWORD dwDlgBase = ::GetDialogBaseUnits(); 
	int cxMargin = LOWORD(dwDlgBase) / 4; 
	int cyMargin = HIWORD(dwDlgBase) / 8; 

	HWND hWnd = GetDlgItem(IDC_SETTING_TAB);
	if (hWnd == NULL) {
		return;
	}

	TCHAR text[MAX_PATH];
	TCITEM tie;
	memset (&tie, 0, sizeof(tie));
	tie.mask = TCIF_TEXT;
	tie.iImage = -1;
	tie.pszText = text;
	for (size_t i = 0; i < COUNT_OF(tabNames); ++ i) {
		TabCtrl_InsertItem(hWnd, i, &tie);
	}
	CRect rc;
	::GetWindowRect(hWnd, &rc);
	TabCtrl_AdjustRect(hWnd, FALSE, &rc);
	ScreenToClient(&rc);

	hWnd = m_dlgGesture.Create(m_hWnd, 0);
	::SetWindowPos(hWnd, HWND_TOP, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

void CSettingDialog::_SetLanguage () {
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
	int tabCount = TabCtrl_GetItemCount(hWnd);
	assert(tabCount == COUNT_OF(tabNames));
	TCHAR text[MAX_PATH];
	for (int i = 0; i < tabCount; ++ i) {
		TCITEM tie;
		memset(&tie, 0, sizeof(tie));

		TabCtrl_GetItem(hWnd, i, &tie);
		tie.pszText = text;
		tie.mask |= TCIF_TEXT;

		xl::tstring name = pLanguage->getString(tabNames[i]);
		memset(text, 0, sizeof(text));
		_tcsncpy_s(text, name.c_str(), MAX_PATH - 1);

		TabCtrl_SetItem(hWnd, i, &tie);
	}
}

CSettingDialog::CSettingDialog () {
}

CSettingDialog::~CSettingDialog () {
}


LRESULT CSettingDialog::OnInitDialog (UINT, WPARAM, LPARAM, BOOL &) {

	_CreateTabs();
	_SetLanguage();
	return TRUE;
}

LRESULT CSettingDialog::OnCommand (UINT, WPARAM wParam, LPARAM, BOOL &) {
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