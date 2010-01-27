#include <assert.h>

#include <Windows.h>
#include <GdiPlus.h>

#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/ResMgr.h"

#include "MainWindow.h"
#include "ImageView.h"
#include "Slider.h"

static xl::tchar* s_extensions[] = {
	_T("jpeg"),
	_T("jpg"),
	_T("jif"),
};

bool CMainWindow::_IsFileSupported (const xl::tstring fileName) {
	size_t index = fileName.rfind(_T('.'));
	if (index != fileName.npos) {
		xl::tstring ext = fileName.substr(index + 1);
		bool match = false;
		for (int i = 0; i < COUNT_OF(s_extensions); ++ i) {
			if (_tcsicmp(ext.c_str(), s_extensions[i]) == 0) {
				match = true;
				break;
			}
		}
		return match;
	}
	return false;
}


void CMainWindow::setFile (const xl::tstring &file) {
	assert(m_fileNames.size() == 0);
	xl::tstring dir = xl::file_get_directory(file);
	dir += _T("\\");
	xl::tstring pattern = dir + _T("*.*");
	xl::CTimerLogger logger(_T("searching files cost"));

	// find the files
	WIN32_FIND_DATA wfd;
	memset(&wfd, 0, sizeof(wfd));
	HANDLE hFind = ::FindFirstFile(pattern, &wfd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			xl::tstring name = dir + wfd.cFileName; 
			if (_IsFileSupported(name)) {
				m_fileNames.push_back(name);
			}
		} while (::FindNextFile(hFind, &wfd));
		::FindClose(hFind);
	} else {
		MessageBox(_T("Can't find any file"));
	}
}


void CMainWindow::onCommand (xl::uint id, xl::ui::CControlPtr ctrl) {

}

void CMainWindow::onSlider (xl::uint id, int _min, int _max, int _curr, bool tracking, xl::ui::CControlPtr ctrl) {
}


xl::tstring CMainWindow::onGesture (const xl::tstring &gesture, CPoint ptDown, bool release) {
	if (gesture == _T("canceled")) {
		return _T("timeout");
	}
	return xl::ui::CCtrlTarget::onGesture(gesture, ptDown, release);
}


LRESULT CMainWindow::OnCreate (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	bHandled = false;
	if (m_ctrlMain == NULL) {
		m_ctrlMain.reset(new xl::ui::CCtrlMain(this, this));
	}
	m_ctrlMain->enableGesture(true);
	m_ctrlMain->setStyle(_T("background:none"));
	xl::ui::CControlPtr gestureCtrl = m_ctrlMain->getGestureCtrl();
	gestureCtrl->setStyle(_T("color:#ff0000"));

	CImageView *pView = new CImageView();
	m_ctrlMain->insertChild(xl::ui::CControlPtr(pView));
	
	xl::ui::CControlPtr slider(new CSlider());
	slider->setStyle(_T("margin:0 auto; py:bottom; width:480; float:true;"));
	slider->setStyle(_T("slider:0 100 0;"));
	m_ctrlMain->insertChild(slider);

	xl::ui::CResMgr *pResMgr = xl::ui::CResMgr::getInstance();

	return TRUE;
}


LRESULT CMainWindow::OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	if (m_ctrlMain) {
		CRect rc;
		GetClientRect(rc);
		m_ctrlMain->layout(rc);
	}
	return 0;
}
