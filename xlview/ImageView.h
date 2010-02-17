#ifndef XLVIEW_IMAGE_VIEW_H
#define XLVIEW_IMAGE_VIEW_H
#include "libxl/include/ui/Control.h"
#include "ClassWithThreads.h"
#include "ImageConfig.h"
#include "ImageLoader.h"
#include "ImageManager.h"

#define ID_VIEW  99

class DisplayInfo
{
	bool               m_suitable;
	CSize              m_szView;
	CSize              m_szImage;
	CSize              m_szZoomTo;

public:
	DisplayInfo () {
		reset();
	}

	void reset () {
		m_suitable = true;
		m_szImage = CSize(-1, -1);
		m_szZoomTo = CSize(-1, -1);
	}

	void setViewSize (CRect rc) {
		CSize sz = CSize(rc.Width(), rc.Height());
		CHECK_VIEW_SIZE(sz);
		if (m_szView != sz) {
			m_szView = sz;
			if (m_szImage != CSize(-1, -1)) {
				m_szZoomTo = getSuitableSize();
			}
		}
	}

	void setImageSize (CSize szImage) {
		if (m_szImage != szImage) {
			m_szImage = szImage;
			m_szZoomTo = getSuitableSize();
		}
	}

	CSize getSuitableSize () {
		assert(m_szImage != CSize(-1, -1));
		assert(m_szView.cx > 0 && m_szView.cy > 0);

		return CImage::getSuitableSize(m_szView, m_szImage);
	}

	CSize getZoomSize () {
		assert(m_szZoomTo != CSize(-1, -1));
		return m_szZoomTo;
	}

	void drawDebugInfo (HDC hdc, CRect rc) {
		xl::tchar info[128];
		_stprintf_s(info, 128, _T("image size: %dx%d"), m_szImage.cx, m_szImage.cy);
		xl::ui::CDCHandle dc(hdc);
		rc.top = rc.bottom - 32;
		UINT fmt = DT_SINGLELINE | DT_VCENTER;
		dc.DrawText(info, -1, rc, fmt);
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
