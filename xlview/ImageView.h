#ifndef XLVIEW_IMAGE_VIEW_H
#define XLVIEW_IMAGE_VIEW_H
#include "libxl/include/ui/Control.h"

#define ID_VIEW  99

class CImageView :
	public xl::ui::CControl
{
public:
	CImageView(void);
	virtual ~CImageView(void);
};

#endif
