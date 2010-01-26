#ifndef XLVIEW_IMAGE_VIEW_H
#define XLVIEW_IMAGE_VIEW_H
#include "libxl/include/ui/Control.h"
#include "Image.h"

#define ID_VIEW  99

class CImageView :
	public xl::ui::CControl
{
protected:

	CImagePtr m_image;

public:
	CImageView(void);
	virtual ~CImageView(void);

	void setImage (CImagePtr image);

	//////////////////////////////////////////////////////////////////////////
	// virtual
	virtual void onSize ();
	virtual void drawMe (HDC hdc);

	virtual void onLButtonDown (CPoint pt, xl::uint key);
	virtual void onLButtonUp (CPoint pt, xl::uint key);
	virtual void onMouseMove (CPoint pt, xl::uint key);
	virtual void onLostCapture ();
};

#endif
