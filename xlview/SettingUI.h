#ifndef XL_VIEW_SETTING_UI_H
#define XL_VIEW_SETTING_UI_H
#include <atlbase.h>
#include <atltypes.h>
#include <atlapp.h>
#include <atlwin.h>
#include "libxl/include/ui/MainWindow.h"
#include "resource.h"

class CGestureMap;

/////////////////////////////////////////////////////////////////////
// draw gesture dialog
class CDrawGestureDialog : public CDialogImpl<CDrawGestureDialog>
{
	class CCanvas : public xl::ui::CMainWindowT<CCanvas>
	{
		CDrawGestureDialog                    *m_pDialog;
		CGestureMap                           *m_pGestureMap;
		static const int BARHEIGHT = 32;
	public:
		CCanvas (CDrawGestureDialog *, CGestureMap *);
		virtual ~CCanvas ();

		virtual void onCommand (xl::uint id, xl::ui::CControlPtr ctrl);
		virtual void onSlider (xl::uint id, int _min, int _max, int _curr, bool tracking, xl::ui::CControlPtr ctrl);
		virtual xl::tstring onGesture (const xl::tstring &gesture, CPoint ptDown, bool release);

		BEGIN_MSG_MAP (CCanvas)
			MESSAGE_HANDLER (WM_CREATE, OnCreate)
			MESSAGE_HANDLER (WM_DESTROY, OnDestroy)
			MESSAGE_HANDLER (WM_SIZE, OnSize)
			MESSAGE_HANDLER (WM_PAINT, OnPaint)
			CHAIN_MSG_MAP(CMainWindowT)
		END_MSG_MAP ()

		LRESULT OnCreate (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
		LRESULT OnDestroy (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
		LRESULT OnSize (UINT, WPARAM, LPARAM, BOOL &);
		LRESULT OnPaint (UINT, WPARAM, LPARAM, BOOL &);
	};

	CCanvas            m_canvas;
public:
	xl::tstring        m_gesture;
	enum {
		IDD = IDD_SETTING_DRAW_GESTURE,
	};

	BEGIN_MSG_MAP (CGestureDialog)
		MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER (WM_COMMAND, OnCommand)
		MESSAGE_HANDLER (WM_SIZE, OnSize)
	END_MSG_MAP ()

	CDrawGestureDialog (CGestureMap *);
	virtual ~CDrawGestureDialog ();

	LRESULT OnCommand (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};

/////////////////////////////////////////////////////////////////////
// gesture dialog (window)
class CGestureDialog : public CDialogImpl<CGestureDialog>
{
	CGestureMap       *m_gestureMap;
	int                m_currIndex;
	void _InitGestureList ();
	void _SetLanguage ();
	void _OnListItemChanged ();
	void _OnEditChanged ();
	void _Apply ();


public:
	enum {
		IDD = IDD_SETTING_GESTURE,
	};

	BEGIN_MSG_MAP (CGestureDialog)
		MESSAGE_HANDLER (WM_CTLCOLORSTATIC, OnCtlColorStatic)
		MESSAGE_HANDLER (WM_ERASEBKGND, OnEraseBkGnd)
		MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER (WM_COMMAND, OnCommand)
		MESSAGE_HANDLER (WM_SIZE, OnSize)
		NOTIFY_ID_HANDLER (IDC_LIST_GESTURE, OnListGestureNotify)
	END_MSG_MAP ()

	CGestureDialog (CGestureMap *);
	virtual ~CGestureDialog ();

	LRESULT OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCommand (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnEraseBkGnd (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCtlColorStatic (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnListGestureNotify (int wParam, LPNMHDR lParam, BOOL &bHandled);
};


/////////////////////////////////////////////////////////////////////
// the whole setting dialog
class CSettingDialog : public CDialogImpl<CSettingDialog>
{
	CGestureDialog                                 m_dlgGesture;

	void _CreateTabs ();
	void _SetLanguage ();

public:
	enum {
		IDD = IDD_SETTING,
	};

	BEGIN_MSG_MAP (CSettingDialog)
		MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER (WM_COMMAND, OnCommand)
	END_MSG_MAP ()


	CSettingDialog (CGestureMap *);
	virtual ~CSettingDialog ();

	LRESULT OnInitDialog (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCommand (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};

#endif
