#include <assert.h>
#include <libxl/include/utilities.h>
#include "libxl/include/Language.h"
#include "libxl/include/ui/ResMgr.h"
#include "libxl/include/ui/Gdi.h"
#include "SettingUI.h"
#include "GestureMap.h"

///////////////////////////////////////////////////////////////////////
// static
static TCHAR *tabNames[] = {_T("Gesture"), _T("Keypad"), _T("FileAssociation"), _T("About"), };
static HWND tabWindow[] = {0, 0, 0, 0,};



///////////////////////////////////////////////////////////////////////
// The whole setting dialog

void CSettingDialog::_CreateTabs () {
/*	DWORD dwDlgBase = ::GetDialogBaseUnits(); 
	int cxMargin = LOWORD(dwDlgBase) / 4; 
	int cyMargin = HIWORD(dwDlgBase) / 8; 
*/
	HWND hWnd = GetDlgItem(IDC_SETTING_TAB);
	if (hWnd == NULL) {
		return;
	}
	HWND hTab = hWnd;

	TCHAR text[MAX_PATH];
	TCITEM tie;
	memset (&tie, 0, sizeof(tie));
	tie.mask = TCIF_TEXT;
	tie.iImage = -1;
	tie.pszText = text;
	for (size_t i = 0; i < COUNT_OF(tabNames); ++ i) {
		TabCtrl_InsertItem(hTab, i, &tie);
	}
	CRect rc;
	::GetWindowRect(hTab, &rc);
	TabCtrl_AdjustRect(hTab, FALSE, &rc);
	::ScreenToClient(m_hWnd, &rc.TopLeft());
	::ScreenToClient(m_hWnd, &rc.BottomRight());

	int idx = 0;
	hWnd = m_dlgGesture.Create(m_hWnd, 0);
	::SetWindowPos(hWnd, HWND_TOP, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
	tabWindow[idx ++] = hWnd;

	hWnd = m_dlgKeypad.Create(m_hWnd);
	::SetWindowPos(hWnd, HWND_TOP, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
	tabWindow[idx ++] = hWnd;

	if (xl::os_is_vista_or_later()) {
		hWnd = m_dlgFileAssocVista.Create(m_hWnd);
	} else {
		assert(xl::os_is_xp());
		m_dlgFileAssocXp.setTabWindow(hTab);
		hWnd = m_dlgFileAssocXp.Create(m_hWnd);
	}
	::SetWindowPos(hWnd, HWND_TOP, rc.left, rc.top, rc.Width(), rc.Height(), SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	tabWindow[idx ++] = hWnd;

	hWnd = m_dlgAbout.Create(m_hWnd);
	::SetWindowPos(hWnd, HWND_TOP, rc.left, rc.top, rc.Width(), rc.Height(), SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	tabWindow[idx ++] = hWnd;
}

void CSettingDialog::_SetLanguage () {
	TCHAR text[MAX_PATH];
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	::SetWindowText(m_hWnd, pLanguage->getString(_T("Settings")).c_str());

	HWND hWnd = GetDlgItem(IDOK);
	if (hWnd != NULL) {
		::GetWindowText(hWnd, text, MAX_PATH);
		::SetWindowText(hWnd, pLanguage->getString(text).c_str());
	}


	hWnd = GetDlgItem(IDC_SETTING_TAB);
	int tabCount = TabCtrl_GetItemCount(hWnd);
	assert(tabCount == COUNT_OF(tabNames));
	for (int i = 0; i < tabCount; ++ i) {
		TCITEM tie;
		memset(&tie, 0, sizeof(tie));

		TabCtrl_GetItem(hWnd, i, &tie);
		tie.pszText = text;
		tie.mask |= TCIF_TEXT;

		xl::tstring name = pLanguage->getString(tabNames[i]);
		name += _T("  ");
		memset(text, 0, sizeof(text));
		_tcsncpy_s(text, name.c_str(), MAX_PATH - 1);

		TabCtrl_SetItem(hWnd, i, &tie);
	}
}

void CSettingDialog::_OnTabSelChanged () {
	HWND hWnd = GetDlgItem(IDC_SETTING_TAB);
	assert(hWnd != NULL);
	int index = TabCtrl_GetCurSel(hWnd);
	for (int i = 0; i < COUNT_OF(tabWindow); ++ i) {
		HWND hWnd = tabWindow[i];
		if (hWnd == NULL) {
			continue;
		}

		if (i == index) {
			::ShowWindow(hWnd, SW_SHOW);
		} else {
			::ShowWindow(hWnd, SW_HIDE);
		}
	}
}

CSettingDialog::CSettingDialog (CGestureMap *gestureMap)
	: m_dlgGesture(gestureMap)
	, m_dlgKeypad(gestureMap)
{
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

LRESULT CSettingDialog::OnTabNotify (int, LPNMHDR lpNMHDR, BOOL &bHandled) {
	switch (lpNMHDR->code) {
	case TCN_SELCHANGE:
		_OnTabSelChanged();
		break;
	default:
		bHandled = false;
		break;
	}
	return 0;
}
