#ifndef XL_VIEW_DISPATCH_H
#define XL_VIEW_DISPATCH_H
/**
 *
 */
#include "libxl/include/common.h"
#include "libxl/include/string.h"

class CMainWindow;
class CImageView;

class CDispatch {

	CMainWindow       *m_pMainWindow;
	CImageView        *m_pView;

public:
	CDispatch (CMainWindow *pMainWindow, CImageView *pView);
	void execute (const xl::tstring &command, CPoint ptCur = CPoint(-1, -1));
};

#endif
