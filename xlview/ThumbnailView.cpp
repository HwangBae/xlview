#include <assert.h>
#include "libxl/include/ui/Gdi.h"
#include "libxl/include/ui/CtrlMain.h"
#include "libxl/include/ui/CtrlTarget.h"
#include "CommandId.h"
#include "ThumbnailView.h"

static const int TV_WIDTH = 60;
static const int TV_HEIGHT = 80;
static const int TV_MARGIN = 0;
static const int TV_PADDING = 12;
static const int TV_PADDING_ACTIVE = 4;

//////////////////////////////////////////////////////////////////////////
// 
CThumbnailView::_CThumbnail::_CThumbnail (int index, CRect rc, CImagePtr thumbnail)
	: m_index(index), m_rect(rc), m_thumbnail(thumbnail)
{

}

CThumbnailView::_CThumbnail::~_CThumbnail () {

}

int CThumbnailView::_CThumbnail::getIndex () const {
	return m_index;
}

CRect CThumbnailView::_CThumbnail::getRect () const {
	return m_rect;
}

bool CThumbnailView::_CThumbnail::hasThumbnail () const {
	return m_thumbnail != NULL;
}

void CThumbnailView::_CThumbnail::setThumbnail (CImagePtr thumbnail) {
	m_thumbnail = thumbnail;
}

void CThumbnailView::_CThumbnail::draw (HDC hdc, int currIndex, int hoverIndex) {
	if (m_thumbnail == NULL) {
		return;
	}
	xl::ui::CDIBSectionPtr thumbnail = m_thumbnail->getImage(0);
	assert(thumbnail != NULL);
	CSize szImage = m_thumbnail->getImageSize();
	CRect rc = m_rect;
	if (currIndex != m_index && hoverIndex != m_index) {
		rc.DeflateRect(TV_PADDING, TV_PADDING, TV_PADDING, TV_PADDING);
	} else {
		rc.DeflateRect(TV_PADDING_ACTIVE, TV_PADDING_ACTIVE, TV_PADDING_ACTIVE, TV_PADDING_ACTIVE);
	}

	CSize szArea(rc.Width(), rc.Height());
	CSize szDraw = CImage::getSuitableSize(szArea, szImage);
	int x = rc.left + (rc.Width() - szDraw.cx) / 2;
	int y = rc.top + (rc.Height() - szDraw.cy) / 2;

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);

	xl::ui::CDIBSectionHelper helper(thumbnail, mdc);
	int oldMode = dc.SetStretchBltMode(HALFTONE);
	dc.StretchBlt(x, y, szDraw.cx, szDraw.cy,
		mdc, 0, 0, thumbnail->getWidth(), thumbnail->getHeight(), SRCCOPY);
	dc.SetStretchBltMode(oldMode);
	helper.detach();

	if (currIndex == m_index || hoverIndex == m_index) {
		rc = m_rect;
		rc.DeflateRect(2, 2, 2, 2);
		COLORREF color = currIndex == m_index ? RGB(255,255,255) : RGB(168,168,192);
		dc.drawRectangle(rc, TV_PADDING_ACTIVE, color);
	}
}


//////////////////////////////////////////////////////////////////////////
//
void CThumbnailView::_CreateThumbnailList () {
	assert(m_pImageManager->getLockLevel() > 0);
	assert(getLockLevel() > 0);

	m_thumbnails.clear();
	if (m_targetIndex == -1) {
		return;
	}

	CRect rc = getClientRect();
	if (rc.Width() <= 0 || rc.Height() <= 0) {
		return;
	}

	int x1 = rc.left + rc.Width() / 2;
	int y1 = rc.top;
	int x2 = x1;
	int y2 = rc.bottom;
	int index = -1;
	CImagePtr thumbnail;

	// 1. current
	x1 -= TV_WIDTH / 2;
	x2 = x1 + TV_WIDTH;
	index = m_currIndex;
	thumbnail = m_pImageManager->getThumbnail(index);
	m_thumbnails.push_back(_CThumbnailPtr(new _CThumbnail(index, CRect(x1, y1, x2, y2), thumbnail)));

	// 2. left
	while (x2 > rc.left && index > 0) {
		x2 = x1 - TV_MARGIN;
		x1 -= TV_WIDTH;
		index --;
		thumbnail = m_pImageManager->getThumbnail(index);
		m_thumbnails.push_back(_CThumbnailPtr(new _CThumbnail(index, CRect(x1, y1, x2, y2), thumbnail)));
	}

	// 3. right
	x1 = rc.left + rc.Width() / 2;
	x1 -= TV_WIDTH / 2;
	x2 = x1 + TV_WIDTH;
	index = m_currIndex + 1;
	int count = (int)m_pImageManager->getImageCount();
	while (x2 < rc.right && index < count) {
		x1 = x2 + TV_MARGIN;
		x2 = x1 + TV_WIDTH;
		thumbnail = m_pImageManager->getThumbnail(index);
		m_thumbnails.push_back(_CThumbnailPtr(new _CThumbnail(index, CRect(x1, y1, x2, y2), thumbnail)));
		index ++;
	}
}

void CThumbnailView::_OnThumbnailLoaded (int index) {
	assert(m_pImageManager->getLockLevel() > 0);
	assert(getLockLevel() > 0);

	for (_Thumbnails::iterator it = m_thumbnails.begin(); it != m_thumbnails.end(); ++ it) {
		if ((*it)->getIndex() == index) {
			if (!(*it)->hasThumbnail()) {
				CImagePtr thumbnail = m_pImageManager->getThumbnail(index);
				(*it)->setThumbnail(thumbnail);
				invalidate();
			}
			break;
		}
	}
}

void CThumbnailView::_ProcessSlide () {
	CScopeMultiLock lock(this, true);
	assert(m_targetIndex != m_currIndex);
	int step = m_targetIndex > m_currIndex ? 1 : -1;
	m_currIndex += step;
	_CreateThumbnailList();
	if (m_currIndex != m_targetIndex) {
		_SetTimer(25, (xl::uint)this);
	} else {
		// get the hover item
		CPoint pt = _GetMainCtrl()->getCursorPos();
		onMouseMove(pt, 0);
	}
	invalidate();
}

CThumbnailView::CThumbnailView (CImageManager *pImageManager)
	: CMultiLock(pImageManager)
	, m_pImageManager(pImageManager)
	, m_targetIndex(-1)
	, m_currIndex(-1)
	, m_responseIndexChange(true)
	, m_hoverIndex(-1)
{
	assert(m_pImageManager != NULL);
	m_pImageManager->subscribe(this);
}

CThumbnailView::~CThumbnailView() {

}

void CThumbnailView::drawMe (HDC hdc) {
	CRect rc = getClientRect();
	xl::ui::CDCHandle dc(hdc);

	CScopeMultiLock lock(this, true);
	for (_Thumbnails::iterator it = m_thumbnails.begin(); it != m_thumbnails.end(); ++ it) {
		(*it)->draw(hdc, m_targetIndex, m_hoverIndex);
	}
}

void CThumbnailView::onSize () {
	CScopeMultiLock lock(this, true);
	_CreateThumbnailList();
}

void CThumbnailView::onLButtonDown (CPoint pt, xl::uint) {
	int index = -1;
	CScopeMultiLock lock(this, false);
	if (m_currIndex != m_targetIndex) {
		return;
	}
	int currIndex = m_currIndex;
	for (_Thumbnails::iterator it = m_thumbnails.begin(); it != m_thumbnails.end(); ++ it) {
		if ((*it)->getRect().PtInRect(pt)) {
			index = (*it)->getIndex();
			break;
		}
	}
	lock.unlock();

	if (index != -1 && index != currIndex) {
		if (abs(index - m_currIndex) == 1) {
			m_pImageManager->setIndex(index);
			CPoint pt = _GetMainCtrl()->getCursorPos();
			onMouseMove(pt, 0); // get hover
		} else {
			lock.lock(this, true);
			m_responseIndexChange = false;
			m_pImageManager->setIndex(index);
			m_targetIndex = index;
			m_responseIndexChange = true;
			lock.unlock();
			_ProcessSlide();
		}
	}
}

void CThumbnailView::onMouseMove (CPoint pt, xl::uint) {
	CScopeMultiLock lock(this, false);
	int hover = -1;
	for (_Thumbnails::iterator it = m_thumbnails.begin(); it != m_thumbnails.end(); ++ it) {
		if ((*it)->getRect().PtInRect(pt)) {
			hover = (*it)->getIndex();
			break;
		}
	}
	if (hover != m_hoverIndex) {
		m_hoverIndex = hover;
		invalidate();
	}
}

void CThumbnailView::onMouseWheel (CPoint /*pt*/, int delta, xl::uint /*key*/) {
	if (delta > 0) {
		_GetTarget()->onCommand(ID_NAV_PREV, shared_from_this());
	} else {
		_GetTarget()->onCommand(ID_NAV_NEXT, shared_from_this());
	}
}

void CThumbnailView::onMouseOut (CPoint) {
	CScopeMultiLock lock(this, false);
	if (m_hoverIndex != -1) {
		m_hoverIndex = -1;
		invalidate();
	}
}

void CThumbnailView::onTimer (xl::uint id) {
	XL_PARAMETER_NOT_USED(id);
	assert(id == (xl::uint)this);

	CScopeMultiLock lock(this, true);
	if (m_currIndex != m_targetIndex) {
		_ProcessSlide();
	}
}

void CThumbnailView::onEvent (EVT evt, void *param) {
	assert(m_pImageManager->getLockLevel() > 0);

	CScopeMultiLock lock(this, false);

	switch (evt) {
	case CImageManager::EVT_INDEX_CHANGED:
		assert(param);
		if (m_responseIndexChange) {
			m_currIndex = m_targetIndex = *(int *)param;
			_CreateThumbnailList();
			invalidate();
		}
		break;
	case CImageManager::EVT_IMAGE_LOADED:
		break;
	case CImageManager::EVT_THUMBNAIL_LOADED:
		assert(param);
		_OnThumbnailLoaded(*(int *)param);
		break;
	case CImageManager::EVT_FILELIST_READY:
		break;
	case CImageManager::EVT_I_AM_DEAD:
		clearExternalLock();
		break;
	default:
		assert(false);
		break;
	}
}