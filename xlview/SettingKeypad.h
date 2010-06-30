#ifndef XL_VIEW_SETTING_KEYPAD_H
#define XL_VIEW_SETTING_KEYPAD_H
#include <atlbase.h>
#include <atltypes.h>
#include <atlapp.h>
#include <atlwin.h>
#include "libxl/include/ui/MainWindow.h"
#include "resource.h"

class CGestureMap;
class CKeypadDialog : public CDialogImpl<CKeypadDialog>
{
	CGestureMap       *m_pGestureMap;
public:
	enum {
		IDD = IDD_SETTING_KEYPAD,
	};

	BEGIN_MSG_MAP (CKeypadDialog)
		MESSAGE_HANDLER (WM_CTLCOLORSTATIC, OnCtlColorStatic)
		MESSAGE_HANDLER (WM_ERASEBKGND, OnEraseBkGnd)
		MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER (WM_SIZE, OnSize)
	END_MSG_MAP ()

	CKeypadDialog (CGestureMap *);
	virtual ~CKeypadDialog ();

	LRESULT OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnEraseBkGnd (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCtlColorStatic (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};


#endif
