#ifndef XLVIEW_IMAGE_VIEW_H
#define XLVIEW_IMAGE_VIEW_H
#include "libxl/include/ui/Control.h"
#include "ClassWithThreads.h"
#include "ImageLoader.h"
#include "ImageManager.h"

#define ID_VIEW  99

struct DisplayInfo
{
	CSize szImageSize;

	DisplayInfo () {
		reset();
	}

	void reset () {
		szImageSize = CSize(-1, -1);
	}
};


//////////////////////////////////////////////////////////////////////////
// CImageView
class CImageView 
	: public xl::ui::CControl
	, public CImageManager::IObserver
	, public xl::CUserLock
	, public ClassWithThreadT<CImageView, 1>
{
protected:
	CImageManager     *m_pImageManager;
	DisplayInfo        m_di;
	CImagePtr          m_imageZoomed; // cloned
	CImagePtr          m_imageRealSize;

	void _OnIndexChanged (int index);
	void _OnImageLoaded (CImagePtr);

	//////////////////////////////////////////////////////////////////////////
	// thread related
	bool m_exiting;
	static unsigned __stdcall _ZoomThread (void *);
	void _BeginZoom ();
public:
	CImageView(CImageManager *pImageManager);
	virtual ~CImageView(void);

	void showLarger ();

	//////////////////////////////////////////////////////////////////////////
	// virtual

	// xl::ui::CControl
	virtual void onSize ();
	virtual void drawMe (HDC hdc);

	virtual void onLButtonDown (CPoint pt, xl::uint key);
	virtual void onLButtonUp (CPoint pt, xl::uint key);
	virtual void onMouseMove (CPoint pt, xl::uint key);
	virtual void onTimer (xl::uint id);
	virtual void onLostCapture ();

	// CImageManager::IObserver
	virtual void onEvent (EVT evt, void *param);

	// used by ClassWithThreads
	const xl::tchar* getThreadName();
	void assignThreadProc();
	void markThreadExit();	
};

#endif
