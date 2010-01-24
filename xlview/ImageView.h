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

	bool m_suitable;
	double m_zoomFactor;
	xl::uint m_srcX;
	xl::uint m_srcY;
	std::tr1::shared_ptr<xl::ui::CMemoryDC> m_pMemoryDC;

	CPoint m_ptGrab;

protected:
	void _CalacuteDisplayParameter ();
	void _CreateSourceBitmap ();

public:
	CImageView(void);
	virtual ~CImageView(void);

	void setImage (CImagePtr image = CImagePtr());

	void showNormalSize ();
	void showSuitable ();
	void showLarger ();
	void showSmaller ();

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
