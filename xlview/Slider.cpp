#include <assert.h>
#include "Slider.h"

CSlider::CSlider ()
	: xl::ui::CCtrlSlider(0, 50, 0)
{
	setStyle(_T("padding:0 16; thumbnail-min-width:24; background-color:#c0c0c0;"));
}

CSlider::~CSlider () {

}

