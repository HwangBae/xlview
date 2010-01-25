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
	int m_imgW; // zoomed size
	int m_imgH; // zoomed size
	int m_srcX;
	int m_srcY;
	int m_dstX;
	int m_dstY;
	std::tr1::shared_ptr<xl::ui::CMemoryDC> m_pMemoryDC;

	CPoint m_ptGrab;

protected:
	bool _CheckCondition ();
	void _CalacuteDisplayParameter ();
	void _CreateSourceBitmap ();
	SIZE _BeforeZoom (CPoint);
	void _AfterZoom (CPoint, SIZE);

public:
	CImageView(void);
	virtual ~CImageView(void);

	void setImage (CImagePtr image = CImagePtr());

	void showNormalSize (CPoint pt);
	void showSuitable (CPoint);
	void showLarger (CPoint pt);
	void showSmaller (CPoint pt);

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
