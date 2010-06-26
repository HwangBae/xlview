#ifndef XL_VIEW_FILE_ASSOCIATION_H
#define XL_VIEW_FILE_ASSOCIATION_H
#include <vector>
#include <atlbase.h>
#include <atltypes.h>
#include <atlapp.h>
#include <atlwin.h>
#include "libxl/include/ui/MainWindow.h"
#include "resource.h"


/////////////////////////////////////////////////////////////////////
// File association

// for windows XP
class CFileAssociationDialogXp : public CDialogImpl<CFileAssociationDialogXp>
{
	HWND               m_hWndTab;
	std::vector<HWND>  m_checkBoxes;

	void _Check4Association ();
	void _OnCheckboxChange ();
	void _OnApply ();
public:
	enum {
		IDD = IDD_SETTING_FILEASSOC_XP,
	};

	void setTabWindow (HWND hWndTab);

	BEGIN_MSG_MAP (CFileAssociationDialogXp)
		MESSAGE_HANDLER (WM_ERASEBKGND, OnEraseBkGnd)
		// MESSAGE_RANGE_HANDLER (WM_CTLCOLORMSGBOX, WM_CTLCOLORSTATIC, OnCtlColor)
		MESSAGE_HANDLER (WM_CTLCOLORSTATIC, OnCtlColor)
		MESSAGE_HANDLER (WM_COMMAND, OnCommand)
		MESSAGE_HANDLER (WM_SIZE, OnSize)
		MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
	END_MSG_MAP ()

	CFileAssociationDialogXp ();
	virtual ~CFileAssociationDialogXp ();

	LRESULT OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCommand (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnEraseBkGnd (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCtlColor (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};



// for vista and windows 7
class CFileAssociationDialogVista : public CDialogImpl<CFileAssociationDialogVista>
{
	void _LaunchSysFileAssociationDialog ();

public:
	enum {
		IDD = IDD_SETTING_FILEASSOC_VISTA,
	};


	BEGIN_MSG_MAP (CFileAssociationDialogVista)
		MESSAGE_HANDLER (WM_ERASEBKGND, OnEraseBkGnd)
		MESSAGE_HANDLER (WM_CTLCOLORSTATIC, OnCtlColorStatic)
		MESSAGE_HANDLER (WM_COMMAND, OnCommand)
		MESSAGE_HANDLER (WM_SIZE, OnSize)
		MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
	END_MSG_MAP ()

	CFileAssociationDialogVista ();
	virtual ~CFileAssociationDialogVista ();

	LRESULT OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCommand (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnEraseBkGnd (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCtlColorStatic (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};


#endif
