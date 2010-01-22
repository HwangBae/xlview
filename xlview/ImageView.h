#ifndef XLVIEW_IMAGE_VIEW_H
#define XLVIEW_IMAGE_VIEW_H
#include "libxl/include/ui/Control.h"
#include "Image.h"

#define ID_VIEW  99

class CImageView :
	public xl::ui::CControl
{
	CImagePtr    m_image;
	xl::uint     m_index;

public:
	CImageView(void);
	virtual ~CImageView(void);

	void setImage (CImagePtr image = CImagePtr());

	//////////////////////////////////////////////////////////////////////////
	// virtual
	virtual void drawMe (HDC hdc);
};

#endif
