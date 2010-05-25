#ifndef XL_VIEW_NAVBUTTON_H
#define XL_VIEW_NAVBUTTON_H
#include "libxl/include/common.h"
#include "libxl/include/ui/CtrlButton.h"

class CNavButton : public xl::ui::CCtrlButton {
	HCURSOR        m_cursor;
	HCURSOR        m_curArrow;

public:
	CNavButton (bool next);
	virtual ~CNavButton ();

	virtual void drawMe (HDC hdc);
	virtual void onMouseMove (CPoint, xl::uint);
	virtual void onMouseOut (CPoint);
};

#endif
