#ifndef XL_VIEW_NAVVIEW_H
#define XL_VIEW_NAVVIEW_H
#include <Windows.h>
#include <atltypes.h>
#include "libxl/include/common.h"
#include "libxl/include/ui/Control.h"

class CImageManager;
class CImageView;
class CNavView : public xl::ui::CControl {
	CImageManager      *m_pImageManager;
	CImageView         *m_pImageView;
	CSize               m_szImageReal;
	CSize               m_szDisplay;
	CSize               m_szView;
	CPoint              m_ptSrc;
	int                 m_currIndex;

	CRect               m_rcView;
	CRect               m_rcImage;
	bool                m_dragable;

	CPoint              m_ptCapture;
	CPoint              m_ptSrcCapture;
	HCURSOR             m_curArrow;
	HCURSOR             m_curMove;
	xl::ui::CDIBSectionPtr                         m_dibView;
	xl::tchar           m_ratio[32];

	void _CreateDisplayInfo ();

public:
	CNavView (CImageManager *, CImageView *);
	virtual ~CNavView ();

	void setInfo (int index, CSize szImageReal, CSize szDisplay, CSize szView, CPoint ptSrc);

	virtual void drawMe (HDC hdc);
	virtual void onMouseMove (CPoint, xl::uint);
	virtual void onLButtonDown (CPoint, xl::uint);
	virtual void onLButtonUp (CPoint, xl::uint);
	virtual void onLostCapture ();
	virtual void onMouseWheel (CPoint, int, xl::uint);
};

#endif
