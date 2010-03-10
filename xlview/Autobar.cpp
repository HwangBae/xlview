#include <assert.h>
#include "libxl/include/ui/CtrlMain.h"
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
	xl::ui::CCtrlMain *pCtrlMain = _GetMainCtrl();
	assert(pCtrlMain != NULL);
	xl::ui::CControlPtr ctrlCapture = pCtrlMain->getCaptureCtrl();
	if (!m_rect.PtInRect(pt) || !isChild(ctrlCapture)) {
		CFadable::fadeOut();
	}
}

void CAutobar::onMouseOutChild (CPoint pt) {
	xl::ui::CCtrlMain *pCtrlMain = _GetMainCtrl();
	assert(pCtrlMain != NULL);
	xl::ui::CControlPtr ctrlCapture = pCtrlMain->getCaptureCtrl();
	if (!m_rect.PtInRect(pt) || !isChild(ctrlCapture)) {
		CFadable::fadeOut();
	}
}

void CAutobar::onTimer (xl::uint id) {
	XL_PARAMETER_NOT_USED(id);
	assert(id == (xl::uint)(CFadable *)this);
	CFadable::process();
}
