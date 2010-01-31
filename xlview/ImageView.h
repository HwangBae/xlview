#ifndef XLVIEW_IMAGE_VIEW_H
#define XLVIEW_IMAGE_VIEW_H
#include "libxl/include/ui/Control.h"
#include "Image.h"
#include "ImageManager.h"

#define ID_VIEW  99

class CImageView 
	: public xl::ui::CControl
	, public CImageManager::IObserver
	, public CImage::ICancel
{
protected:
	CImageManager     *m_pImageManager;
	int                m_currIndex;
	int                m_currLoading;
	CDisplayImagePtr   m_image;
	CImagePtr          m_imageWhenResizing;

	bool               m_suitable;
	int                m_zoomTo;
	int                m_zoomNow;

	void _CreateThreads ();
	void _TerminateThreads ();

	void _ResetParameter ();
	void _PrepareDisplay ();
	void _BeginLoad ();
	void _BeginResize ();
	void _OnIndexChanged ();
	void _OnImageLoaded (bool success);
	void _OnImageResized ();

	//////////////////////////////////////////////////////////////////////////
	// thread
	bool               m_exit;
	bool               m_loading;
	bool               m_resizing;
	HANDLE             m_semaphoreLoad;
	HANDLE             m_threadLoad;
	HANDLE             m_semaphoreResize;
	HANDLE             m_threadResize;
	CRITICAL_SECTION   m_cs;
	static unsigned int __stdcall _LoadThread (void *);
	static unsigned int __stdcall _ResizeThread (void *);

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

	// CImage::ICancel
	virtual bool cancelLoading ();
};

#endif
