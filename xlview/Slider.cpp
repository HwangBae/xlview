#include <assert.h>
#include "Slider.h"

CSlider::CSlider ()
	: xl::ui::CCtrlSlider(0, 50, 0)
	, CFadable(0, 60, 30, 50)
{
	setStyle(_T("padding:0 16; height:40; thumbnail-min-width: 24; opacity:0; background-color:#c0c0c0;"));
}

CSlider::~CSlider () {

}

void CSlider::onMouseIn (CPoint /*pt*/) {
	CFadable<CSlider>::fadeIn();
}

void CSlider::onMouseOut (CPoint /*pt*/) {
	CFadable<CSlider>::fadeOut();
}

void CSlider::onTimer (xl::uint /*id*/) {
	CFadable<CSlider>::process();
}
