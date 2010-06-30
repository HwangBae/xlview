#include <assert.h>
#include "libxl/include/Language.h"
#include "libxl/include/ui/ResMgr.h"
#include "libxl/include/ui/Gdi.h"
#include "GestureMap.h"
#include "SettingKeypad.h"

static const int TAB_PADDING_X = 4;
static const int TAB_PADDING_Y = 8;
static const int TAB_MARGIN_Y = 8;
static const int TAB_MARGIN_X = 8;

CKeypadDialog::CKeypadDialog (CGestureMap *pGestureMap)
	: m_pGestureMap(pGestureMap)
{
	assert(m_pGestureMap != NULL);
}

CKeypadDialog::~CKeypadDialog () {
}

LRESULT CKeypadDialog::OnInitDialog (UINT, WPARAM, LPARAM, BOOL &) {
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	HWND hWnd = GetDlgItem(IDC_STATIC_KEYPAD_DESC);
	xl::tchar text[MAX_PATH];

	assert(hWnd != NULL);
	if (hWnd != NULL) {
		::GetWindowText(hWnd, text, MAX_PATH);
		xl::tstring lang = pLanguage->getString(text);
		::SetWindowText(hWnd, lang.c_str());
	}

	hWnd = GetDlgItem(IDC_STATIC_KEYPAD);
	assert(hWnd != NULL);
	if (hWnd != NULL) {
		std::pair<const xl::tchar *, const xl::tchar *> maps[] = {
			std::make_pair(_T("A_key"), _T("showPrev")),
			std::make_pair(_T("Left_key"), _T("showPrev")),
			std::make_pair(_T(""), _T("")),
			std::make_pair(_T("D_key"), _T("showNext")),
			std::make_pair(_T("Right_key"), _T("showNext")),
			std::make_pair(_T("Space_key"), _T("showNext")),
			std::make_pair(_T(""), _T("")),
			std::make_pair(_T("Up_key"), _T("showLarger")),
			std::make_pair(_T(""), _T("")),
			std::make_pair(_T("Down_key"), _T("showSmaller")),
			std::make_pair(_T(""), _T("")),
			std::make_pair(_T("S_key"), _T("showSwitch")),
			std::make_pair(_T("\\_key"), _T("showSwitch")),
		};

		xl::tstring keypad;
		for (size_t i = 0; i < COUNT_OF(maps); ++ i) {
			const xl::tchar *key = maps[i].first;
			const xl::tchar *command = maps[i].second;
			if (*key == 0) {
				// seperator
				keypad += _T("\n");
				continue;
			}

			xl::tstring k = pLanguage->getString(key);
			xl::tstring cmd = m_pGestureMap->translateCommand(command);
			keypad += k;
			keypad += _T("\t\t=>  ");
			keypad += cmd;
			keypad += _T("\n");
		}
		::SetWindowText(hWnd, keypad.c_str());
	}

	return TRUE;
}

LRESULT CKeypadDialog::OnEraseBkGnd (UINT, WPARAM, LPARAM, BOOL &) {
	return TRUE;
}

LRESULT CKeypadDialog::OnSize (UINT, WPARAM, LPARAM, BOOL &) {
	CRect rc;
	GetClientRect(&rc);
	rc.DeflateRect(TAB_PADDING_X, TAB_PADDING_Y);
	int x = rc.left + TAB_MARGIN_X;
	int y = rc.top + TAB_MARGIN_Y;
	int width = rc.Width() - TAB_MARGIN_X * 2;
	HWND hWnd = GetDlgItem(IDC_STATIC_KEYPAD_DESC);
	if (hWnd != NULL) {
		CRect rect;
		::GetWindowRect(hWnd, &rect);
		::MoveWindow(hWnd, x, y, width, rect.Height(), true);

		y += rect.Height() + TAB_MARGIN_Y;
	}

	hWnd = GetDlgItem(IDC_STATIC_KEYPAD);
	if (hWnd != NULL) {
		int height = rc.bottom - TAB_MARGIN_Y - y;
		::MoveWindow(hWnd, x, y, width, height, true);
	}

	return 0;
}

LRESULT CKeypadDialog::OnCtlColorStatic (UINT, WPARAM wParam, LPARAM, BOOL &) {
	HDC hdc = (HDC)wParam;
	::SetBkMode(hdc, TRANSPARENT);
	return (LRESULT)::GetStockObject(NULL_BRUSH);
}

