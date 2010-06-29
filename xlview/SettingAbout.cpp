#include <assert.h>
#include <utility>
#include "libxl/include/Language.h"

#include "version.h"
#include "resource.h"
#include "SettingAbout.h"

static const int TAB_PADDING_X = 4;
static const int TAB_PADDING_Y = 8;
static const int TAB_MARGIN_X = 8;
static const int TAB_MARGIN_Y = 12;

// static ID and the [link | static] ID
// just like the key and value pair
static std::pair<UINT, UINT> sLines[] = {
	std::make_pair(IDC_STATIC_COPYRIGHT,           IDC_SYSLINK_COPYRIGHT),
	std::make_pair(IDC_STATIC_AUTHOR,              IDC_STATIC_CYBERSCORPIO),
	std::make_pair(IDC_STATIC_TWITTER,             IDC_SYSLINK_TWITTER),
	std::make_pair(IDC_STATIC_PROJECT_HOME,        IDC_SYSLINK_PROJECT_HOME),
	std::make_pair(IDC_STATIC_LICENSE,             IDC_SYSLINK_LICENSE),
	std::make_pair(IDC_STATIC_THANKS,              IDC_SYSLINK_THANKS),
};



CAboutDialog::CAboutDialog () {
}

CAboutDialog::~CAboutDialog () {
}

LRESULT CAboutDialog::OnInitDialog (UINT, WPARAM, LPARAM, BOOL &) {
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	xl::tchar text[MAX_PATH];
	for (size_t i = 0; i < COUNT_OF(sLines); ++ i) {
		HWND hWndKey = GetDlgItem(sLines[i].first);
		assert(hWndKey != NULL);

		::GetWindowText(hWndKey, text, MAX_PATH);
		xl::tstring lang = pLanguage->getString(text);
		::SetWindowText(hWndKey, lang);
	}

	HWND hWnd = GetDlgItem(IDC_SYSLINK_COPYRIGHT);
	m_brush4SysLink = (HBRUSH)::SetClassLongPtr(hWnd, GCL_HBRBACKGROUND, (LONG_PTR)::GetStockObject(NULL_BRUSH));

	hWnd = GetDlgItem(IDC_STATIC_VERSION);
	assert(hWnd);
	_stprintf_s(text, MAX_PATH, _T("xlview 1.0.0.%s (r%s)"), xlview_revision, xlview_revision);
	::SetWindowText(hWnd, text);
	return TRUE;
}

LRESULT CAboutDialog::OnDestroy (UINT, WPARAM, LPARAM, BOOL &bHandled) {
	HWND hWnd = GetDlgItem(IDC_SYSLINK_COPYRIGHT);
	assert(hWnd != NULL);
	::SetClassLongPtr(hWnd, GCL_HBRBACKGROUND, (LONG_PTR)m_brush4SysLink);
	bHandled = false;
	return 0;
}

LRESULT CAboutDialog::OnNotify (UINT, WPARAM, LPARAM lParam, BOOL &bHandled) {
	LPNMHDR lpnmh = (LPNMHDR)lParam;
	assert(lpnmh != NULL);
	switch (lpnmh->code)
	{
	case NM_CLICK:
	case NM_RETURN:
		{
			PNMLINK pNMLink = (PNMLINK)lpnmh;
			LITEM item = pNMLink->item;
			::ShellExecute(NULL, _T("open"), item.szUrl, NULL, NULL, SW_SHOW);
			break;
		}
		break;
	default:
		bHandled = false;
		break;
	}

	return 0;
}

LRESULT CAboutDialog::OnSize (UINT, WPARAM, LPARAM, BOOL &) {
	CRect rc;
	GetClientRect(&rc);
	rc.DeflateRect(TAB_PADDING_X, TAB_PADDING_Y);

	int x = rc.left + TAB_MARGIN_X;
	int y = rc.top + TAB_MARGIN_Y;
	int keyWidth = (rc.Width() - TAB_MARGIN_X * 3) / 3;
	int valueWidth = rc.Width() - TAB_MARGIN_X * 3 - keyWidth;

	for (size_t i = 0; i < COUNT_OF(sLines); ++ i) {
		HWND hWndKey = GetDlgItem(sLines[i].first);
		HWND hWndValue = GetDlgItem(sLines[i].second);
		assert(hWndKey && hWndValue);

		CRect rcKey, rcValue;
		::GetWindowRect(hWndKey, &rcKey);
		::GetWindowRect(hWndValue, &rcValue);

		// make syslink the same height as the static control
		rcValue.bottom = rcValue.top + rcKey.bottom - rcKey.top;

		// has no hurt, but if we don't make the same height above, the below code make sense
		int lineHeight = max(rcKey.Height(), rcValue.Height());
		int offsetKeyY = (lineHeight - rcKey.Height()) / 2;
		int offsetValueY = (lineHeight - rcValue.Height()) / 2;

		::MoveWindow(hWndKey, x, y + offsetKeyY, keyWidth, rcKey.Height(), TRUE);
		::MoveWindow(hWndValue, x + keyWidth + TAB_MARGIN_X, y + offsetValueY, valueWidth, rcValue.Height(), TRUE);

		y += lineHeight + TAB_MARGIN_Y;
	}

	HWND hWndVersion = GetDlgItem(IDC_STATIC_VERSION);
	if (hWndVersion) {
		CRect rcVersion;
		::GetWindowRect(hWndVersion, &rcVersion);
		y = rc.bottom - TAB_MARGIN_Y - rcVersion.Height();
		rcVersion.MoveToXY(x, y);
		::MoveWindow(hWndVersion, x, y, rc.Width() - TAB_MARGIN_X * 2, rcVersion.Height(), true);
		y += TAB_MARGIN_Y + rcVersion.Height();
	}

	return 0;
}

LRESULT CAboutDialog::OnCommand (UINT, WPARAM, LPARAM, BOOL &bHandled) {
	bHandled = false;
	return 0;
}

LRESULT CAboutDialog::OnEraseBkGnd (UINT, WPARAM, LPARAM, BOOL &) {
	return TRUE;
}

LRESULT CAboutDialog::OnCtlColorStatic (UINT, WPARAM wParam, LPARAM, BOOL &) {
	HDC hdc = (HDC)wParam;
	::SetBkMode(hdc, TRANSPARENT);
	return (LRESULT)::GetStockObject(NULL_BRUSH);
}
