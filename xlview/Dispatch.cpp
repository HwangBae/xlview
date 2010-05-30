#include <assert.h>
#include "MainWindow.h"
#include "ImageView.h"
#include "Dispatch.h"

///////////////////////////////////////////////////////////////////////
// typedefs
typedef void (CMainWindow::*LPMAINWINDOWFUNC)();
typedef void (CImageView::*LPVIEWFUNC)(CPoint);


///////////////////////////////////////////////////////////////////////
// mapping
template<class T>
struct CmdMappingT {
	xl::tstring    command;
	T              func;
};

///////////////////////////////////////////////////////////////////////
// map
static CmdMappingT<LPMAINWINDOWFUNC> sMainWindowCmdMap[] = {
	{_T("Exit"),                                  &CMainWindow::cmdExit},
	{_T("showPrev"),                              &CMainWindow::cmdPrev},
	{_T("showNext"),                              &CMainWindow::cmdNext},
	{_T("Minimize"),                              &CMainWindow::cmdMinimize},
	{_T("MaximizeOrRestore"),                     &CMainWindow::cmdMaximizeOrRestore},
};

static CmdMappingT<LPVIEWFUNC> sImageViewCmdMap[] = {
	{_T("showSuitable"),                          &CImageView::showSuitable},
	{_T("showRealSize"),                          &CImageView::showRealSize},
	{_T("showSwitch"),                            &CImageView::showSwitch},
	{_T("showLarger"),                            &CImageView::showLarger},
	{_T("showSmaller"),                           &CImageView::showSmaller},
	{_T("showTop"),                               &CImageView::showTop},
	{_T("showBottom"),                            &CImageView::showBottom},
	{_T("showLeft"),                              &CImageView::showLeft},
	{_T("showRight"),                             &CImageView::showRight},
};


///////////////////////////////////////////////////////////////////////

CDispatch::CDispatch (CMainWindow *pMainWindow, CImageView *pView)
	: m_pMainWindow(pMainWindow)
	, m_pView(pView)
{
	assert(m_pMainWindow != NULL && m_pView != NULL);
}

void CDispatch::execute (const xl::tstring &command, CPoint ptCur) {
	const xl::tchar *cmd = command.c_str();

	// 1. try CMainWindow command
	for (int i = 0; i < COUNT_OF(sMainWindowCmdMap); ++ i) {
		if (_tcsicmp(cmd, sMainWindowCmdMap[i].command.c_str()) == 0) {
			(m_pMainWindow->*(sMainWindowCmdMap[i].func))();
			return;
		}
	}

	// 2. try CImageView command
	for (int i = 0; i < COUNT_OF(sImageViewCmdMap); ++ i) {
		if (_tcsicmp(cmd, sImageViewCmdMap[i].command.c_str()) == 0) {
			(m_pView->*(sImageViewCmdMap[i].func))(ptCur);
			return;
		}
	}
}
