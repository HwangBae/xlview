#ifndef XL_VIEW_SETTING_UI_H
#define XL_VIEW_SETTING_UI_H
#include <atlbase.h>
#include <atltypes.h>
#include <atlapp.h>
#include <atlwin.h>

#include "resource.h"


class CSettingDialog : public CDialogImpl<CSettingDialog>
{
public:
	enum {
		IDD = IDD_SETTING,
	};

	BEGIN_MSG_MAP (CSettingDialog)
		MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER (WM_COMMAND, OnCommand)
	END_MSG_MAP ()


	CSettingDialog ();
	virtual ~CSettingDialog ();

	LRESULT OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCommand (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};

#endif
