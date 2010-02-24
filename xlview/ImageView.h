#ifndef XLVIEW_IMAGE_VIEW_H
#define XLVIEW_IMAGE_VIEW_H
#include "libxl/include/ui/Control.h"
#include "ClassWithThreads.h"
#include "ImageConfig.h"
#include "ImageLoader.h"
#include "ImageManager.h"

#define ID_VIEW  99


//////////////////////////////////////////////////////////////////////////
// CImageView
class CImageView 
	: public xl::ui::CControl
	, public CImageManager::IObserver
	, public xl::CUserLock
	, public ClassWithThreadT<CImageView, 1>
{
	friend class ClassWithThreadT<CImageView, 1>;
protected:
	CImageManager     *m_pImageManager;

	// for display
	// 
	CSize              m_szRealSize;
	CSize              m_szDisplay;
	CSize              m_szZoom;
	CPoint             m_ptSrc; // in zoomed area
	bool               m_suitable;
	bool               m_zooming;
	CImagePtr          m_imageZoomed; // cloned
	CImagePtr          m_imageRealSize;

	void _OnIndexChanged (int index);
	void _OnImageLoaded (CImagePtr);

	CPoint             m_ptCapture;
	CPoint             m_ptCaptureSrc;
	HCURSOR            m_hCurNormal;
	HCURSOR            m_hCurMove;

	/**
	 * Display Area is the area which the image will **FILL**, the max size is the view size.
	 * Plz note that it is not the Display Size.
	 */
	CRect _CalcDisplayArea (CRect rcView, CSize szDisplay, CPoint ptSrc);
	void _SetDisplaySize (CRect rcView, CSize szDisplaySize, CPoint ptCur = CPoint(-1, -1));
	void _ResetDisplayInfo ();
	void _CheckPtSrc (CPoint &ptSrc);
	void _NotifyDisplayChanged ();

	//////////////////////////////////////////////////////////////////////////
	// thread related
	bool m_exiting;
	static unsigned __stdcall _ZoomThread (void *);
	void _BeginZoom ();

	// used by ClassWithThreads
	enum {
		THREAD_ZOOM,
		THREAD_COUNT
	};
	const xl::tchar* getThreadName();
	void assignThreadProc();
	void markThreadExit();	

public:
	CImageView(CImageManager *pImageManager);
	virtual ~CImageView(void);

	void showSuitable (CPoint ptCur);
	void showRealSize (CPoint ptCur);
	void showLarger (CPoint ptCur);

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

};

#endif
