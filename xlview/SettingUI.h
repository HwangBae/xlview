#ifndef XL_VIEW_SETTING_UI_H
#define XL_VIEW_SETTING_UI_H
// #include <memory>
#include <atlbase.h>
#include <atltypes.h>
#include <atlapp.h>
#include <atlwin.h>
#include "libxl/include/ui/MainWindow.h"
#include "resource.h"
#include "SettingGesture.h"
#include "SettingKeypad.h"
#include "SettingFileAssoc.h"
#include "SettingAbout.h"

class CGestureMap;


/////////////////////////////////////////////////////////////////////
// the whole setting dialog
class CSettingDialog : public CDialogImpl<CSettingDialog>
{
	CGestureDialog                                 m_dlgGesture;
	CKeypadDialog                                  m_dlgKeypad;
	// std::auto_ptr<CDialogImpl>                     m_pDlgFileAssoc;
	CFileAssociationDialogXp                       m_dlgFileAssocXp;
	CFileAssociationDialogVista                    m_dlgFileAssocVista;
	CAboutDialog                                   m_dlgAbout;

	void _CreateTabs ();
	void _SetLanguage ();
	void _OnTabSelChanged ();

public:
	enum {
		IDD = IDD_SETTING,
	};

	BEGIN_MSG_MAP (CSettingDialog)
		MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER (WM_COMMAND, OnCommand)
		NOTIFY_ID_HANDLER (IDC_SETTING_TAB, OnTabNotify)
	END_MSG_MAP ()


	CSettingDialog (CGestureMap *);
	virtual ~CSettingDialog ();

	LRESULT OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCommand (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnTabNotify (int wParam, LPNMHDR lParam, BOOL &bHandled);
};

#endif
