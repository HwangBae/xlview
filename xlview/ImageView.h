#ifndef XLVIEW_IMAGE_VIEW_H
#define XLVIEW_IMAGE_VIEW_H
#include "libxl/include/ui/Control.h"
#include "DisplayImage.h"
#include "ImageLoader.h"
#include "ImageManager.h"

#define ID_VIEW  99

class CDisplayParameter {
	bool suitable;
	int zoomTo;
	int zoomNow;
	int srcX;
	int srcY;
	CSize realSize;
	CSize zoomSize;
	CRect rcView;

	int frameIndex;

	void _DrawSuitable (HDC, CImagePtr);

	void _CalacuteParameter ();

public:
	CDisplayParameter ();
	~CDisplayParameter ();
	void reset (CRect);

	void setRealSize (CSize);
	void setViewRect (CRect);

	CSize getRealSize () const { return realSize; }
	CSize getZoomSize () const { return zoomSize; }
	int getZoomTo () const { return zoomTo; }
	int getZoomNow () const { return zoomNow; }

	void draw (HDC, CImagePtr);
	void drawParameter (HDC);
};

//////////////////////////////////////////////////////////////////////////
// CImageView
class CImageView 
	: public xl::ui::CControl
	, public CImageManager::IObserver
	, public xl::CUserLock
{
protected:
	CImageManager     *m_pImageManager;

	CDisplayParameter   m_disp;
	CImagePtr          m_imageThumbnail; // cloned
	CImagePtr          m_imageZoomed; // cloned
	CImagePtr          m_imageRealSize; // point to m_pImageManager->m_image[curr]->getRealSizeImage();

	void _OnIndexChanged (int index);
	void _OnImageLoaded (int index);

	//////////////////////////////////////////////////////////////////////////
	// thread related
	bool m_exiting;
	HANDLE m_hZoomEvent;
	HANDLE m_hZoomThread;
	static unsigned __stdcall _ZoomThread (void *);
	void _CreateThreads ();
	void _TerminateThreads ();
	void _BeginZoom ();
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
