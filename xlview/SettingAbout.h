#ifndef XL_VIEW_SETTING_ABOUT_H
#define XL_VIEW_SETTING_ABOUT_H
#include <atlbase.h>
#include <atltypes.h>
#include <atlapp.h>
#include <atlwin.h>
#include "libxl/include/ui/MainWindow.h"
#include "resource.h"

class CAboutDialog : public CDialogImpl<CAboutDialog>
{
	HBRUSH             m_brush4SysLink;
public:
	enum {
		IDD = IDD_SETTING_ABOUT,
	};


	BEGIN_MSG_MAP (CAboutDialog)
		MESSAGE_HANDLER (WM_ERASEBKGND, OnEraseBkGnd)
		MESSAGE_HANDLER (WM_CTLCOLORSTATIC, OnCtlColorStatic)
		MESSAGE_HANDLER (WM_COMMAND, OnCommand)
		MESSAGE_HANDLER (WM_SIZE, OnSize)
		MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER (WM_DESTROY, OnDestroy)
	END_MSG_MAP ()

	CAboutDialog ();
	virtual ~CAboutDialog ();

	LRESULT OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCommand (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnEraseBkGnd (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCtlColorStatic (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnDestroy (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};




#endif
