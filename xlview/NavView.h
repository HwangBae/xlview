#ifndef XL_VIEW_NAVVIEW_H
#define XL_VIEW_NAVVIEW_H
#include <Windows.h>
#include <atltypes.h>
#include "libxl/include/common.h"
#include "libxl/include/ui/Control.h"

class CImageManager;
class CNavView : public xl::ui::CControl {
	CImageManager      *m_pImageManager;
	CSize               m_szDisplay;
	CSize               m_szView;
	CPoint              m_ptSrc;
	int                 m_currIndex;

	void _CreateDisplayInfo ();

public:
	CNavView (CImageManager *);
	virtual ~CNavView ();

	void setInfo (int index, CSize szDisplay, CSize szView, CPoint ptSrc);

	virtual void drawMe (HDC hdc);
};

#endif
