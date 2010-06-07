#ifndef XL_VIEW_MAIN_WINDOW_H
#define XL_VIEW_MAIN_WINDOW_H

#include "libxl/include/string.h"
#include "libxl/include/ui/MainWindow.h"

#include "ImageManager.h"
#include "GestureMap.h"

#define WM_XLVIEW_IMAGE_LOADED                         (WM_XL_END + 1)
#define WM_XLVIEW_INVALIDE                             (WM_XL_END + 2)

// forward declaration
class CDispatch;


class CMainWindow 
	: public xl::ui::CMainWindowT<CMainWindow>
	, public CImageManager
	, public CImageManager::IObserver
{

	xl::ui::CControlPtr                            m_navbar;
	xl::ui::CControlPtr                            m_toolbar;
	xl::ui::CControlPtr                            m_slider;
	xl::ui::CControlPtr                            m_view;

	CDispatch                                     *m_pDispatch;
	CGestureMap                                    m_gestureMap;

public:
	virtual void onCommand (xl::uint id, xl::ui::CControlPtr ctrl);
	virtual void onSlider (xl::uint id, int _min, int _max, int _curr, bool tracking, xl::ui::CControlPtr ctrl);
	virtual xl::tstring onGesture (const xl::tstring &gesture, CPoint ptDown, bool release);

public:
	DECLARE_WND_CLASS_EX (_T("xlview / MainWindow"), 0, COLOR_WINDOW)
	BEGIN_MSG_MAP (CMainWindow)
		MESSAGE_HANDLER (WM_CREATE, OnCreate)
		MESSAGE_HANDLER (WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER (WM_SIZE, OnSize)
		MESSAGE_HANDLER (WM_KEYDOWN, OnKeyDown)
		CHAIN_MSG_MAP(CMainWindowT)
	END_MSG_MAP ()

	LRESULT OnCreate (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnDestroy (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnKeyDown (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

	// CImageManager::IObserver
	virtual void onEvent (CImageManager::IObserver::EVT evt, void *param);

	// command functions
	void cmdExit ();
	void cmdPrev ();
	void cmdNext ();
	void cmdMinimize ();
	void cmdMaximizeOrRestore ();
};

#endif
