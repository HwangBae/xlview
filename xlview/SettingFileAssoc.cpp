#include <assert.h>

#include <algorithm>

#include <Windows.h>
#include <WindowsX.h>
#include <Uxtheme.h>
#include <Vssym32.h>
#include "libxl/include/Language.h"
#include "libxl/include/ui/ResMgr.h"
#include "libxl/include/ui/Gdi.h"
#include "Settings.h"
#include "SettingFileAssoc.h"

#pragma comment (lib, "UxTheme.lib")

///////////////////////////////////////////////////////////////////////////////
// fOR wINDOWS xP
static std::pair<UINT, const xl::tchar *> s_extTable[] = {
	std::make_pair(IDC_CHECKBOX_JPG, _T("jpg;jpeg;jfif")),
	std::make_pair(IDC_CHECKBOX_PNG, _T("png")),
};

void CFileAssociationDialogXp::_Check4Association () {
	for (size_t i = 0; i < COUNT_OF(s_extTable); ++ i) {
		HWND hWnd = GetDlgItem(s_extTable[i].first);
		if (hWnd == NULL) {
			continue;
		}

		xl::ExplodeT<xl::tchar>::ValueT exts = xl::explode(_T(";"), s_extTable[i].second);
		bool isDefault = true;
		for (auto it = exts.begin(); it != exts.end(); ++ it) {
			if (!isDefault4Xp(*it)) {
				isDefault = false;
				break;
			}
		}
		Button_SetCheck(hWnd, isDefault ? BST_CHECKED : BST_UNCHECKED);
	}
}

CFileAssociationDialogXp::CFileAssociationDialogXp ()
	: m_hWndTab(NULL)
{
}

CFileAssociationDialogXp::~CFileAssociationDialogXp () {
}

void CFileAssociationDialogXp::setTabWindow (HWND hWndTab) {
	assert(hWndTab != NULL);
	m_hWndTab = hWndTab;
}

LRESULT CFileAssociationDialogXp::OnInitDialog (UINT, WPARAM, LPARAM, BOOL &) {
	HWND hStatic = GetDlgItem(IDC_STATIC_REINSTALL);
	assert(hStatic != NULL);
	DWORD dwStyle = ::GetWindowLong(hStatic, GWL_STYLE);
	dwStyle |= SS_CENTER;
	::SetWindowLong(hStatic, GWL_STYLE, dwStyle);
	HWND hButton = GetDlgItem(IDC_BUTTON_APPLY);
	assert(hButton);

	// Save the check box HWNDs
	UINT checkbox_ids[] = {
		IDC_CHECKBOX_JPG,
		IDC_CHECKBOX_PNG,
	};
	for (int i = 0; i < COUNT_OF(checkbox_ids); ++ i) {
		HWND hWnd = GetDlgItem(checkbox_ids[i]);
		assert(hWnd != NULL);
		m_checkBoxes.push_back(hWnd);
	}

	bool isRegistered = isAppRegistered();
	::ShowWindow(hStatic, isRegistered ? SW_HIDE : SW_SHOW);
	::ShowWindow(hButton, isRegistered ? SW_SHOW : SW_HIDE);
	std::for_each(m_checkBoxes.begin(), m_checkBoxes.end(), [=](HWND hWnd) {
		::ShowWindow(hWnd, isRegistered ? SW_SHOW : SW_HIDE);
	});

	xl::tchar text[MAX_PATH];
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	HWND hWnds[] = {hStatic, hButton};
	for (int i = 0; i < COUNT_OF(hWnds); ++ i) {
		HWND hWnd = hWnds[i];
		::GetWindowText(hWnd, text, MAX_PATH);
		xl::tstring lang = pLanguage->getString(text);
		::SetWindowText(hWnd, lang.c_str());
	}

	_Check4Association();

	return TRUE;
}

LRESULT CFileAssociationDialogXp::OnSize (UINT, WPARAM, LPARAM, BOOL &) {
	HWND hStatic = GetDlgItem(IDC_STATIC_REINSTALL);
	assert(hStatic != NULL);
	HWND hButton = GetDlgItem(IDC_BUTTON_APPLY);
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
		int x = rc.Width() - rcButton.Width();
		int y = rc.Height() - rcButton.Height();
		::MoveWindow(hButton, x, y, rcButton.Width(), rcButton.Height(), TRUE);

		y = 0;
		std::for_each(m_checkBoxes.begin(), m_checkBoxes.end(), [&rc, &y](HWND hWnd) {
			CRect rect;
			::GetWindowRect(hWnd, &rect);
			rect.MoveToXY(rc.left, rc.top + y);
			y += rect.Height() + 8;
			::MoveWindow(hWnd, rect.left, rect.top, rc.Width(), rect.Height(), TRUE);
		});
	}

	return 0;
}

LRESULT CFileAssociationDialogXp::OnCommand (UINT, WPARAM wParam, LPARAM, BOOL &bHandled) {
	WORD code = HIWORD(wParam);
	code = code;
	WORD id = LOWORD(wParam);

	switch (id) {
	case IDC_BUTTON_APPLY:
		break;
	default:
		bHandled = false;
		break;
	}

	return 0;
}


LRESULT CFileAssociationDialogXp::OnEraseBkGnd (UINT, WPARAM, LPARAM, BOOL &) {
	return TRUE;
}

LRESULT CFileAssociationDialogXp::OnCtlColor (UINT, WPARAM wParam, LPARAM lParam, BOOL &) {
	HWND hCtrl = (HWND)lParam;
	HDC hdc = (HDC)wParam;

	bool isCheckbox = false;
	for (auto it = m_checkBoxes.begin(); it != m_checkBoxes.end(); ++ it) {
		if (hCtrl == *it) {
			isCheckbox = true;
			break;
		}
	}
	if (isCheckbox) {
		if (!IsAppThemed() || !IsThemeActive()) {
			::SetBkMode((HDC)wParam, TRANSPARENT);
		} else {
			HTHEME hTheme = OpenThemeData(hCtrl, L"TAB");
			if (hTheme != NULL) {
				CRect rc, rcTab;
				::GetWindowRect(hCtrl, &rc);
				::ScreenToClient(m_hWndTab, &rc.TopLeft());
				::ScreenToClient(m_hWndTab, &rc.BottomRight());
				int x = -rc.left;
				int y = -rc.top;
				rc.MoveToXY(0, 0);
				::GetWindowRect(m_hWndTab, &rcTab);
				rcTab.MoveToXY(0, 0);
				TabCtrl_AdjustRect(m_hWndTab, FALSE, &rcTab);
				rcTab.MoveToXY(x + rcTab.left, y + rcTab.top);

				DrawThemeBackground(hTheme, hdc, TABP_PANE, 0, &rcTab, &rc);
				CloseThemeData(hTheme);
			}
		}
	} else if (hCtrl == GetDlgItem(IDC_STATIC_REINSTALL)) {
		::SetBkMode(hdc, TRANSPARENT);
	}
	return (LRESULT)::GetStockObject(NULL_BRUSH);
}




///////////////////////////////////////////////////////////////////////////////
// fOR vISTA aND wINDOWS 7
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
	code = code;
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

LRESULT CFileAssociationDialogVista::OnCtlColorStatic (UINT, WPARAM wParam, LPARAM, BOOL &) {
	HDC hdc = (HDC)wParam;
	::SetBkMode(hdc, TRANSPARENT);
	return (LRESULT)::GetStockObject(NULL_BRUSH);
}
