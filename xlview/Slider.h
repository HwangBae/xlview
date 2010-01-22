#ifndef XL_VIEW_SLIDER_H
#define XL_VIEW_SLIDER_H
#include "libxl/include/ui/CtrlSlider.h"

class CSlider : public xl::ui::CCtrlSlider
{
public:
	CSlider ();
	virtual ~CSlider ();

	virtual void onMouseIn (CPoint pt);
	virtual void onMouseOut (CPoint pt);
};

#endif
