#ifndef XLVIEW_IMAGE_VIEW_H
#define XLVIEW_IMAGE_VIEW_H
#include "libxl/include/ui/Control.h"
#include "Image.h"
#include "ImageManager.h"

#define ID_VIEW  99

class CImageView 
	: public xl::ui::CControl
	, public CImageManager::IObserver
{
protected:
	CImageManager     *m_pImageManager;
	int                m_currIndex;

	CImagePtr          m_image;
	bool               m_suitable;
	int                m_zoom;

	void _ResetParameter ();
	void _OnIndexChanged ();

public:
	CImageView(CImageManager *pImageManager);
	virtual ~CImageView(void);

	//////////////////////////////////////////////////////////////////////////
	// virtual

	// xl::ui::CControl
	virtual void onSize ();
	virtual void drawMe (HDC hdc);

	virtual void onLButtonDown (CPoint pt, xl::uint key);
	virtual void onLButtonUp (CPoint pt, xl::uint key);
	virtual void onMouseMove (CPoint pt, xl::uint key);
	virtual void onLostCapture ();

	// CImageManager::IObserver
	virtual void onEvent (EVT evt, void *param);
};

#endif
