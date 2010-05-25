#include <assert.h>
#include "CommandId.h"
#include "NavButton.h"
#include "resource.h"

CNavButton::CNavButton (bool next)
	: xl::ui::CCtrlButton(next ? ID_NAV_NEXT : ID_NAV_PREV)
{
	setStyle(_T("height:fill; float:true; opacity:0;"));
	if (next) {
		setStyle(_T("px:right;"));
	} else {
		setStyle(_T("px:left"));
	}

	m_cursor = ::LoadCursor(::GetModuleHandle(NULL), 
		next ? MAKEINTRESOURCE(IDC_NEXT) : MAKEINTRESOURCE(IDC_PREV));
	m_curArrow = ::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
}

CNavButton::~CNavButton () {
	::DeleteObject(m_cursor);
	::DeleteObject(m_curArrow);
}

void CNavButton::drawMe (HDC /*hdc*/) {

}

void CNavButton::onMouseMove (CPoint, xl::uint) {
	::SetCursor(m_cursor);
}

void CNavButton::onMouseOut (CPoint) {
	::SetCursor(m_curArrow);
}