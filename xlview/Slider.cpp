#include <assert.h>
#include "Slider.h"

CSlider::CSlider ()
	: xl::ui::CCtrlSlider(0, 50, 0)
	, CFadable(0, 50, 50)
{
	setStyle(_T("padding:0 16; height:40; thumbnail-min-width: 24; opacity:0; background-color:#c0c0c0;"));
}

CSlider::~CSlider () {

}

void CSlider::onMouseIn (CPoint /*pt*/) {
	if (!disable) {
		CFadable<CSlider>::onMouseIn();
	}
}

void CSlider::onMouseOut (CPoint /*pt*/) {
	if (!disable) {
		CFadable<CSlider>::onMouseOut();
	}
}

void CSlider::onTimer (xl::uint /*id*/) {
	CFadable<CSlider>::process();
}
