#ifndef XL_VIEW_SLIDER_H
#define XL_VIEW_SLIDER_H
#include "libxl/include/ui/CtrlSlider.h"
#include "Fadable.h"

class CSlider
	: public xl::ui::CCtrlSlider
	, CFadable<CSlider>
{
	friend class CFadable<CSlider>;
public:
	CSlider ();
	virtual ~CSlider ();

	virtual void onMouseIn (CPoint pt);
	virtual void onMouseOut (CPoint pt);
	virtual void onTimer (xl::uint id);
};

#endif
