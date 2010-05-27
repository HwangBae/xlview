#ifndef XL_VIEW_TOOLBAR_BUTTON_H
#define XL_VIEW_TOOLBAR_BUTTON_H
#include "libxl/include/ui/CtrlButton.h"

class CToolbarButton : public xl::ui::CCtrlButton
{
public:
	CToolbarButton (xl::uint id, const xl::tstring &text = _T(""), xl::uint idImg = 0, bool imgTrans = false, COLORREF clrKey = RGB(255, 0, 255));

	virtual void drawMe (HDC hdc);
};


#endif
