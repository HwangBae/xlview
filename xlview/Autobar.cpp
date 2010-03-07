#include <assert.h>
#include "Autobar.h"

CAutobar::CAutobar (int fadeout, int fadein, int step, int fadeoutdelay, int timeinterval)
	: CFadable(fadeout, fadein, step, fadeoutdelay, timeinterval)
{

}

CAutobar::~CAutobar () {

}

void CAutobar::onMouseIn (CPoint /*pt*/) {
	CFadable::fadeIn();
}

void CAutobar::onMouseInChild (CPoint /*pt*/) {
	CFadable::fadeIn();
}

void CAutobar::onMouseOut (CPoint pt) {
	if (!m_rect.PtInRect(pt)) {
		CFadable::fadeOut();
	}
}

void CAutobar::onMouseOutChild (CPoint pt) {
	if (!m_rect.PtInRect(pt)) {
		CFadable::fadeOut();
	}
}

void CAutobar::onTimer (xl::uint id) {
	XL_PARAMETER_NOT_USED(id);
	assert(id == (xl::uint)(CFadable *)this);
	CFadable::process();
}
