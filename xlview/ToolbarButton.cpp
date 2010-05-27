#include "libxl/include/ui/CtrlMain.h"
#include "ToolbarButton.h"

CToolbarButton::CToolbarButton (xl::uint id, const xl::tstring &text, xl::uint idImg, bool imgTrans, COLORREF clrKey)
	: CCtrlButton (id, text, idImg, imgTrans, clrKey)
{
}

void CToolbarButton::drawMe (HDC hdc) {
	xl::ui::CCtrlMain *pCtrlMain = _GetMainCtrl();
	bool hovered = pCtrlMain->getHoverCtrl() == shared_from_this();
	if (hovered && !m_pushAndCapture) {
		m_rect.OffsetRect(-1, -1);
	}

	xl::ui::CCtrlButton::drawMe(hdc);

	if (hovered && !m_pushAndCapture) {
		m_rect.OffsetRect(1, 1);
	}
}