#ifndef XL_VIEW_INFOVIEW_h
#define XL_VIEW_INFOVIEW_h
#include <Windows.h>
#include <atltypes.h>
#include "libxl/include/common.h"
#include "libxl/include/string.h"
#include "libxl/include/ui/Control.h"
#include "ImageManager.h"


class CInfoView 
	: public xl::ui::CControl
	, public CImageManager::IObserver
{
	CImageManager     *m_pImageManager;
	int                m_index;
	xl::tstring        m_fileName;
	CSize              m_szImage;
	CSize              m_szDisplay;

public:
	CInfoView (CImageManager *);
	virtual ~CInfoView ();

	virtual void drawMe (HDC hdc);
	virtual void onDetach ();

	// CImageManager::IObserver
	virtual void onEvent (EVT evt, void *param);

	void onDisplayInfoChanged (CSize);
};


#endif
