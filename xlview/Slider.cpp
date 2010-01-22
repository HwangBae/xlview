#include <assert.h>
#include "Slider.h"

CSlider::CSlider ()
	: xl::ui::CCtrlSlider(0, 50, 0)
{
	setStyle(_T("padding:0 32; height:32; opacity:0;"));
}

CSlider::~CSlider () {

}

void CSlider::onMouseIn (CPoint pt) {
	if (!disable) {
		setStyle (_T("opacity:50"));
	}
}

void CSlider::onMouseOut (CPoint pt) {
	if (!disable) {
		setStyle (_T("opacity:0"));
	}
}
