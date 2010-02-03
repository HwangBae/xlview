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


void CMainWindow::onCommand (xl::uint id, xl::ui::CControlPtr ctrl) {

}

void CMainWindow::onSlider (xl::uint id, int _min, int _max, int _curr, bool tracking, xl::ui::CControlPtr ctrl) {
	assert(id == m_slider->getID());

	setIndex(_curr);
}


xl::tstring CMainWindow::onGesture (const xl::tstring &gesture, CPoint ptDown, bool release) {
	if (gesture == _T("canceled")) {
		return _T("timeout");
	}

	// test gesture and prev & next
	if (gesture == _T("R")) {
		if (release) {
			int new_index = m_currIndex + 1;
			if (new_index == m_images.size()) {
				new_index = 0;
			}
			setIndex(new_index);
		}
		return _T("Next");
	}

	if (gesture == _T("L")) {
		if (release) {
			int new_index = m_currIndex;
			if (new_index == 0) {
				new_index = m_images.size() - 1;
			} else {
				new_index --;
			}
			setIndex(new_index);
		}
		return _T("Prev");
	}

	return xl::ui::CCtrlTarget::onGesture(gesture, ptDown, release);
}


LRESULT CMainWindow::OnCreate (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	subscribe(this);

	bHandled = false;
	if (m_ctrlMain == NULL) {
		m_ctrlMain.reset(new xl::ui::CCtrlMain(this, this));
	}
	m_ctrlMain->enableGesture(true);
	m_ctrlMain->setStyle(_T("background:none"));
	xl::ui::CControlPtr gestureCtrl = m_ctrlMain->getGestureCtrl();
	gestureCtrl->setStyle(_T("color:#ff0000"));

	CImageView *pView = new CImageView(this);
	m_ctrlMain->insertChild(xl::ui::CControlPtr(pView));
	
	xl::ui::CControlPtr slider(new CSlider());
	m_slider = slider;
	slider->setStyle(_T("margin:0 auto; py:bottom; width:480; float:true; disable:true"));
	slider->setStyle(_T("slider:0 0 0;"));
	m_ctrlMain->insertChild(slider);

	xl::ui::CResMgr *pResMgr = xl::ui::CResMgr::getInstance();

	return TRUE;
}


LRESULT CMainWindow::OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	if (m_ctrlMain && wParam != SIZE_MINIMIZED) {
		CRect rc;
		GetClientRect(rc);
		m_ctrlMain->layout(rc);
	}
	return 0;
}

void CMainWindow::onEvent (CImageManager::IObserver::EVT evt, void *param) {
	xl::ui::CCtrlSlider *pSlider = (xl::ui::CCtrlSlider *)m_slider.get();
	assert(pSlider != NULL);
	switch (evt) {
	case CImageManager::EVT_READY:
		pSlider->setStyle(_T("disable:false"));
		break;
	case CImageManager::EVT_INDEX_CHANGED:
		{
			int _min = 0, _max = m_images.size() - 1, _curr = m_currIndex;
			TCHAR buf[128];
			_stprintf_s(buf, 128, _T("slider: %d %d %d"), _min, _max, _curr);
			pSlider->setStyle(buf);

			xl::tstring title = _T("xl / view - ") + m_images[_curr]->getFileName();
			SetWindowText(title);
		}
		break;
	default:
		break;
	}
}
