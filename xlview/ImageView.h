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
	CSize              m_szReal;
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
#ifdef PROGRESS_ZOOMING
	CPoint             m_ptCurSaved;
	CSize              m_szDisplayBegin;
	int                m_step;
#endif
	HCURSOR            m_hCurNormal;
	HCURSOR            m_hCurMove;

	/**
	 * Display Area is the area which the image will **FILL**, the max size is the view size.
	 * Plz note that it is not the Display Size.
	 */
	CRect _CalcDisplayArea (CRect rcView, CSize szDisplay, CPoint ptSrc);
	/**
	 * Use this function to set m_szDisplay, it will then set m_ptSrc, and so on
	 * DO NOT set m_szDisplay directly!!
	 */
	void _SetDisplaySize (CRect rcView, CSize szDisplay, CPoint ptCur = CPoint(-1, -1));
	void _ResetDisplayInfo ();
	void _CheckPtSrc (CPoint &ptSrc);
	void _NotifyDisplayChanged ();

	void _CalculateZoomedSize (CSize &szDisplay, CSize szReal, bool isZoomin, double factor);

#ifdef PROGRESS_ZOOMING
	bool _CalcStepedDisplaySize (CSize &szDisplay, CSize szZoom);
#endif
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
	void showSmaller (CPoint ptCur);
	void showTop (CPoint ptCur);
	void showBottom (CPoint ptCur);
	void showLeft (CPoint ptCur);
	void showRight (CPoint ptCur);

	//////////////////////////////////////////////////////////////////////////
	// virtual

	// xl::ui::CControl
	virtual void onSize ();
	virtual void drawMe (HDC hdc);

	virtual void onLButtonDown (CPoint pt, xl::uint key);
	virtual void onLButtonUp (CPoint pt, xl::uint key);
	virtual void onMouseMove (CPoint pt, xl::uint key);
	virtual void onMouseWheel (CPoint pt, int delta, xl::uint key);
	virtual void onTimer (xl::uint id);
	virtual void onLostCapture ();

	// CImageManager::IObserver
	virtual void onEvent (EVT evt, void *param);

	// used for zoom callback test
	bool isExiting () const { return m_exiting; }
	CSize getZoomSize () const { return m_szZoom; }
};

#endif
