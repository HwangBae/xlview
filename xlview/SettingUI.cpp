#include <assert.h>
#include "libxl/include/Language.h"
#include "libxl/include/ui/ResMgr.h"
#include "libxl/include/ui/Gdi.h"
#include "SettingUI.h"
#include "GestureMap.h"

///////////////////////////////////////////////////////////////////////
// static
static TCHAR *tabNames[] = {_T("Gesture"), _T("FileAssociation"), _T("About"), };
static TCHAR *gestureHeaders[] = {_T("GestureCommand"), _T("Gesture"), };
static const int TAB_PADDING_X = 4;
static const int TAB_PADDING_Y = 8;
static const int TAB_MARGIN_Y = 8;
static const int TAB_MARGIN_X = 8;


///////////////////////////////////////////////////////////////////////
// draw gesture
CDrawGestureDialog::CDrawGestureDialog (CGestureMap *pGestureMap) : m_canvas(this, pGestureMap) {
}

CDrawGestureDialog::~CDrawGestureDialog () {
}

LRESULT CDrawGestureDialog::OnCommand (UINT, WPARAM wParam, LPARAM, BOOL &bHandled) {
	//WORD code = HIWORD(wParam);
	WORD id = LOWORD(wParam);
	//HWND hCtrl = (HWND)lParam;

	switch (id) {
	case IDOK:
	case IDCLOSE:
	case IDCANCEL:
		EndDialog(0);
		break;
	default:
		bHandled = false;
		break;
	}

	return 0;
}

LRESULT CDrawGestureDialog::OnInitDialog (UINT, WPARAM, LPARAM, BOOL &) {
	DWORD dwStyle = WS_CHILD | WS_VISIBLE;
	HWND hWnd = m_canvas.Create(m_hWnd, 0, 0, dwStyle);
	if (hWnd != NULL) {
		m_canvas.SetWindowLong(GWL_STYLE, dwStyle);
	}

	CRect rc;
	GetClientRect(&rc);
	m_canvas.MoveWindow(rc.left, rc.top, rc.Width(), rc.Height());

	// set language
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	xl::tchar title[128];
	::GetWindowText(m_hWnd, title, 128);
	xl::tstring lang = pLanguage->getString(title);
	::SetWindowText(m_hWnd, lang.c_str());

	return TRUE;
}

LRESULT CDrawGestureDialog::OnSize (UINT, WPARAM, LPARAM, BOOL &) {
	CRect rc;
	GetClientRect(&rc);
	m_canvas.MoveWindow(rc.left, rc.top, rc.Width(), rc.Height());
	return 0;
}


CDrawGestureDialog::CCanvas::CCanvas (CDrawGestureDialog *pDialog, CGestureMap *pGestureMap)
	: m_pDialog(pDialog)
	, m_pGestureMap(pGestureMap)
{
	assert(m_pDialog != NULL);
}

CDrawGestureDialog::CCanvas::~CCanvas () {
}

void CDrawGestureDialog::CCanvas::onCommand (xl::uint, xl::ui::CControlPtr) {
}

void CDrawGestureDialog::CCanvas::onSlider (xl::uint, int, int, int, bool, xl::ui::CControlPtr) {
}

xl::tstring CDrawGestureDialog::CCanvas::onGesture (const xl::tstring &gesture, CPoint, bool release) {
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	if (gesture == _T("canceled")) {
		return pLanguage->getString(_T("GestureTimeout"));
	}

	if (release) {
		m_pDialog->m_gesture = gesture;
		m_pDialog->EndDialog(1);
	}

	xl::tstring action = m_pGestureMap->onGesture(gesture);
	if (_tcsicmp(action.c_str(), _T("Unknown")) != 0) {
		return pLanguage->getString(_T("Gesture") + action);
	} else {
		return gesture;
	}
}

LRESULT CDrawGestureDialog::CCanvas::OnCreate (UINT, WPARAM, LPARAM, BOOL &) {
	if (m_ctrlMain == NULL) {
		m_ctrlMain.reset(new xl::ui::CCtrlMain(this, this));
	}

	// gesture
	m_ctrlMain->enableGesture(true);
	m_ctrlMain->setStyle(_T("background-color:#00ffff"));
	xl::ui::CControlPtr gestureCtrl = m_ctrlMain->getGestureCtrl();
	gestureCtrl->setStyle(_T("color:#ff0000; gesture-timeout:50000"));

	return TRUE;
}

LRESULT CDrawGestureDialog::CCanvas::OnDestroy (UINT, WPARAM, LPARAM, BOOL &) {
	return 0;
}

LRESULT CDrawGestureDialog::CCanvas::OnSize (UINT, WPARAM wParam, LPARAM, BOOL &) {
	if (m_ctrlMain && wParam != SIZE_MINIMIZED) {
		CRect rc;
		GetClientRect(&rc);
		rc.top += BARHEIGHT;
		m_ctrlMain->layout(rc);
	}
	return 0;
}

LRESULT CDrawGestureDialog::CCanvas::OnPaint (UINT, WPARAM, LPARAM, BOOL &bHandled) {
	bHandled = false;

	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	xl::tstring text = pLanguage->getString(_T("DrawGestureDetail"));

	xl::ui::CResMgr *pResMgr = xl::ui::CResMgr::getInstance();
	HFONT font = pResMgr->getSysFont(16);

	CRect rc;
	GetClientRect(&rc);
	rc.bottom = BARHEIGHT;
	HDC hdc = GetDC();
	xl::ui::CDCHandle dc(hdc);

	HFONT oldFont = dc.SelectFont(font);
	int oldMode = dc.SetBkMode(TRANSPARENT);

	dc.DrawText(text.c_str(), text.length(), &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	dc.SetBkMode(oldMode);
	dc.SelectFont(oldFont);

	ReleaseDC(hdc);
	return 0;
}


///////////////////////////////////////////////////////////////////////
// The gesture dialog
void CGestureDialog::_InitGestureList () {
	assert(m_gestureMap != NULL);
	HWND hWnd = GetDlgItem(IDC_LIST_GESTURE);
	assert(hWnd != NULL);

	// 1. styles
	DWORD dwExStyle = 0;
	dwExStyle = ListView_GetExtendedListViewStyle(hWnd);
	dwExStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
	ListView_SetExtendedListViewStyle(hWnd, dwExStyle);

	// 2. headers
	TCHAR text[MAX_PATH] = {0};
	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	for (size_t i = 0; i < COUNT_OF(gestureHeaders); ++ i) {
		lvc.iSubItem = i;
		lvc.pszText = text;
		lvc.cx = 100;
		_tcsncpy_s(text, gestureHeaders[i], MAX_PATH - 1);

		if (ListView_InsertColumn(hWnd, i, &lvc) == -1) {
			break;
		}
	}

	// 3. insert gestures
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	int index = 0;
	for (auto it = m_gestureMap->begin(); it != m_gestureMap->end(); ++ it) {
		xl::tstring gesture = m_gestureMap->translateGesture(it->gesture);
		xl::tstring command = pLanguage->getString(_T("Gesture") + it->command);
		LVITEM lvi;
		memset(&lvi, 0, sizeof(lvi));
		lvi.mask = LVIF_TEXT;
		lvi.iItem = index ++;
		lvi.iSubItem = 0;
		lvi.pszText = text;
		_tcsncpy_s(text, command.c_str(), MAX_PATH - 1);
		ListView_InsertItem(hWnd, &lvi);

		lvi.iSubItem = 1;
		_tcsncpy_s(text, gesture.c_str(), MAX_PATH - 1);
		ListView_SetItem(hWnd, &lvi);
	}
}

void CGestureDialog::_SetLanguage () {
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	HWND hWnd = GetDlgItem(IDC_LIST_GESTURE);
	assert(hWnd != NULL);
	TCHAR text[MAX_PATH] = {0};
	LVCOLUMN lvc;
	lvc.pszText = text;
	for (size_t i = 0; i < COUNT_OF(gestureHeaders); ++ i) {
		lvc.mask = LVCF_TEXT;
		xl::tstring t = pLanguage->getString(gestureHeaders[i]);
		_tcsncpy_s(text, t.c_str(), MAX_PATH - 1);
		ListView_SetColumn(hWnd, i, &lvc);
	}

	int ids[] = {IDC_BUTTON_DRAW_GESTURE, IDC_BUTTON_EDIT_GESTURE_OK, IDC_STATIC_GESTURE, };
	for (size_t i = 0; i < COUNT_OF(ids); ++ i) {
		hWnd = GetDlgItem(ids[i]);
		assert(hWnd != NULL);
		if (hWnd != NULL) {
			::GetWindowText(hWnd, text, MAX_PATH);
			::SetWindowText(hWnd, pLanguage->getString(text).c_str());
		}
	}
}

void CGestureDialog::_OnListItemChanged () {
	HWND hWnd = GetDlgItem(IDC_LIST_GESTURE);
	HWND hBtnApply = GetDlgItem(IDC_BUTTON_EDIT_GESTURE_OK);
	HWND hBtnDraw = GetDlgItem(IDC_BUTTON_DRAW_GESTURE);
	HWND hEdit = GetDlgItem(IDC_EDIT_GESTURE);
	assert(hWnd && hBtnApply && hBtnDraw && hEdit);
	int sc = ListView_GetSelectedCount(hWnd);
	if (sc == 0 && m_currIndex != -1) {
		m_currIndex = -1;
		::EnableWindow(hBtnApply, false);
		::EnableWindow(hBtnDraw, false);
		::SetWindowText(hEdit, _T(""));
		::EnableWindow(hEdit, false);
	} else {
		int index = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
		if (index != m_currIndex) {
			m_currIndex = index;
			auto it = m_gestureMap->begin();
			it += index;
			::EnableWindow(hBtnApply, false);
			::EnableWindow(hBtnDraw, true);
			::EnableWindow(hEdit, true);
			::SetWindowText(hEdit, it->gesture.c_str());
		}
	}
}

void CGestureDialog::_OnEditChanged () {
	if (m_currIndex == -1) {
		return; // ignore
	}

	HWND hBtnApply = GetDlgItem(IDC_BUTTON_EDIT_GESTURE_OK);
	HWND hEdit = GetDlgItem(IDC_EDIT_GESTURE);
	assert(hBtnApply && hEdit);

	xl::tchar gesture[MAX_PATH];
	::GetWindowText(hEdit, gesture, MAX_PATH);

	auto it = m_gestureMap->begin();
	it += m_currIndex;

	const xl::tchar *p = it->gesture.c_str();
	::EnableWindow(hBtnApply,
		_tcsicmp(gesture, p) != 0 && _tcslen(gesture) > 0);
}

void CGestureDialog::_Apply () {
	assert(m_currIndex != -1);
	xl::tchar gesture[MAX_PATH];
	HWND hEdit = GetDlgItem(IDC_EDIT_GESTURE);
	::GetWindowText(hEdit, gesture, MAX_PATH);

	// test for illegal
	int length = _tcslen(gesture);
	assert(length > 0);
	for (int i = 0; i < length; ++ i) {
		xl::tchar c = gesture[i];
		if (c != _T('L') && c != _T('R') && c != _T('U') && c != _T('D')) {
			xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
			xl::tstring prompt = pLanguage->getString(_T("OnlyLRUDAllowed"));
			MessageBox(prompt.c_str(), _T(""), MB_ICONERROR);
			return;
		}
	}

	// Set the new gesture
	auto it = m_gestureMap->begin();
	it += m_currIndex;
	xl::tstring command = it->command;
	m_gestureMap->setGesture(command, gesture);

	// TODO: save it
	// .......

	// update the list control
	xl::tstring description = m_gestureMap->translateGesture(gesture);
	HWND hWnd = GetDlgItem(IDC_LIST_GESTURE);
	assert(hWnd != NULL);
	if (hWnd) {
		xl::tchar text[MAX_PATH];
		LVITEM lvi;
		memset(&lvi, 0, sizeof(lvi));
		lvi.mask = LVIF_TEXT;
		lvi.iItem = m_currIndex;
		lvi.iSubItem = 1;
		lvi.pszText = text;
		_tcsncpy_s(text, description.c_str(), MAX_PATH - 1);
		ListView_SetItem(hWnd, &lvi);
	}

	// disable the apply button
	hWnd = GetDlgItem(IDC_BUTTON_EDIT_GESTURE_OK);
	assert(hWnd != NULL);
	if (hWnd) {
		::EnableWindow(hWnd, false);
	}
}


CGestureDialog::CGestureDialog (CGestureMap *gestureMap) 
	: m_gestureMap(gestureMap)
	, m_currIndex(-1)
{
}

CGestureDialog::~CGestureDialog () {
}

LRESULT CGestureDialog::OnInitDialog (UINT, WPARAM, LPARAM, BOOL &) {
	_InitGestureList();
	_SetLanguage();

	HWND hEdit = GetDlgItem(IDC_EDIT_GESTURE);
	assert(hEdit != NULL);
	if (hEdit) {
		::SendMessage(hEdit, EM_SETLIMITTEXT, CGestureMap::MAX_LENGTH, 0);
	}
	return TRUE;
}

LRESULT CGestureDialog::OnCommand (UINT, WPARAM wParam, LPARAM, BOOL &bHandled) {
	WORD code = HIWORD(wParam);
	WORD id = LOWORD(wParam);
	//HWND hCtrl = (HWND)lParam;

	switch (id) {
	case IDC_EDIT_GESTURE:
		if (code == EN_CHANGE) {
			_OnEditChanged();
		}
		break;
	case IDC_BUTTON_EDIT_GESTURE_OK:
		_Apply ();
		break;
	case IDC_BUTTON_DRAW_GESTURE:
		{
			CDrawGestureDialog dlg(m_gestureMap);
			if (dlg.DoModal(m_hWnd) == 1) {
				xl::tstring gesture = dlg.m_gesture;
				assert(gesture.length() > 0);
				if (gesture.length() > CGestureMap::MAX_LENGTH) {
					gesture = gesture.substr(0, CGestureMap::MAX_LENGTH);
				}

				HWND hEdit = GetDlgItem(IDC_EDIT_GESTURE);
				assert(hEdit != NULL);
				::SetWindowText(hEdit, gesture.c_str());
			}
		}
		break;
	default:
		bHandled = false;
		break;
	}

	return 0;
}

LRESULT CGestureDialog::OnEraseBkGnd (UINT, WPARAM, LPARAM, BOOL &) {
	return TRUE;
}

LRESULT CGestureDialog::OnSize (UINT, WPARAM, LPARAM, BOOL &) {
	const DWORD dwSWP = SWP_NOACTIVATE | SWP_NOZORDER;
	CRect rc;
	GetClientRect(&rc);
	rc.DeflateRect(TAB_PADDING_X, TAB_PADDING_Y);

	int buttonHeight = 0, editHeight = 0;
	HWND hWnd = GetDlgItem(IDC_BUTTON_EDIT_GESTURE_OK);
	assert(hWnd != NULL);
	if (hWnd != NULL) {
		CRect rcb;
		::GetWindowRect(hWnd, &rcb);
		buttonHeight = rcb.Height();
	}

	hWnd = GetDlgItem(IDC_EDIT_GESTURE);
	assert(hWnd != NULL);
	if (hWnd != NULL) {
		CRect rce;
		::GetWindowRect(hWnd, &rce);
		editHeight = rce.Height();
	}

	hWnd = GetDlgItem(IDC_LIST_GESTURE);
	assert(hWnd != NULL);
	if (hWnd != NULL) {
		CRect rcc = rc;
		rcc.bottom -= TAB_MARGIN_Y * 2 + editHeight + buttonHeight;
		::SetWindowPos(hWnd, HWND_TOP, rcc.left, rcc.top, rcc.Width(), rcc.Height(), dwSWP);

		// list view headers
		LVCOLUMN lvc;
		memset(&lvc, 0, sizeof(lvc));
		int w = rcc.Width() / COUNT_OF(gestureHeaders) - 32;
		for (size_t i = 0; i < COUNT_OF(gestureHeaders); ++ i) {
			lvc.mask = LVCF_WIDTH;
			lvc.cx = w;
			ListView_SetColumn(hWnd, i, &lvc);
		}
	}

	// Place the buttons and edit box
	int buttonY = rc.bottom - editHeight - buttonHeight - TAB_MARGIN_Y;
	hWnd = GetDlgItem(IDC_BUTTON_EDIT_GESTURE_OK);
	assert(hWnd != NULL);
	if (hWnd != NULL) {
		CRect rcb;
		::GetWindowRect(hWnd, &rcb);
		::SetWindowPos(hWnd, HWND_TOP, 
			rc.right - rcb.Width(), buttonY,
			rcb.Width(), rcb.Height(), dwSWP);
	}

	hWnd = GetDlgItem(IDC_BUTTON_DRAW_GESTURE);
	assert(hWnd != NULL);
	if (hWnd != NULL) {
		CRect rcb;
		::GetWindowRect(hWnd, &rcb);
		::SetWindowPos(hWnd, HWND_TOP, 
			rc.right - rcb.Width() * 2 - TAB_MARGIN_X, buttonY,
			rcb.Width(), rcb.Height(), dwSWP);
	}

	int editY = rc.bottom - editHeight;
	int editWidth = (rc.Width() - TAB_MARGIN_X) / 3;
	hWnd = GetDlgItem(IDC_STATIC_GESTURE);
	assert(hWnd != NULL);
	if (hWnd != NULL) {
		CRect rce;
		::GetWindowRect(hWnd, &rce);
		::SetWindowPos(hWnd, HWND_TOP, 
			rc.left, editY + (editHeight - rce.Height()) / 2,
			editWidth * 2, rce.Height(), dwSWP);
	}

	hWnd = GetDlgItem(IDC_EDIT_GESTURE);
	assert(hWnd != NULL);
	if (hWnd != NULL) {
		CRect rce;
		::GetWindowRect(hWnd, &rce);
		::SetWindowPos(hWnd, HWND_TOP, 
			rc.left + editWidth * 2 + TAB_MARGIN_X, editY,
			editWidth, rce.Height(), dwSWP);
	}

	return 0;
}

LRESULT CGestureDialog::OnCtlColorStatic (UINT, WPARAM, LPARAM, BOOL &) {
	return (LRESULT)::GetStockObject(NULL_BRUSH);
}

LRESULT CGestureDialog::OnListGestureNotify (int, LPNMHDR lpNMHDR, BOOL &bHandled) {
	XL_PARAMETER_NOT_USED(lpNMHDR);
	switch (lpNMHDR->code) {
	case LVN_ITEMCHANGED:
		_OnListItemChanged();
		break;
	default:
		bHandled = false;
		break;
	}
	return 0;
}



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
		memset(text, 0, sizeof(text));
		_tcsncpy_s(text, name.c_str(), MAX_PATH - 1);

		TabCtrl_SetItem(hWnd, i, &tie);
	}
}

CSettingDialog::CSettingDialog (CGestureMap *gestureMap)
	: m_dlgGesture(gestureMap)
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