#ifndef XL_VIEW_AUTOBAR_H
#define XL_VIEW_AUTOBAR_H
/**
 * it is a simple wrapper, which is a control like a toolbar, 
 * or tool panel, which fade in when the mouse in, and fade out
 * when the mouse out.
 */
#include "libxl/include/ui/Control.h"
#include "Fadable.h"

class CAutobar
	: public xl::ui::CControl
	, private CFadableT<CAutobar>
{
	typedef CFadableT<CAutobar>                    CFadable;
	friend class CFadable;
public:
	CAutobar (int fadeout = 0, int fadein = 50, int step = 20, int fadeoutdelaya = 1000, int timeinterval = 150);
	virtual ~CAutobar();

	virtual void onMouseIn (CPoint);
	virtual void onMouseInChild (CPoint);

	virtual void onMouseOut (CPoint);
	virtual void onMouseOutChild (CPoint);

	virtual void onTimer (xl::uint);
};

#endif