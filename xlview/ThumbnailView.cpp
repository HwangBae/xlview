#include <assert.h>
#include "libxl/include/ui/Gdi.h"
#include "ThumbnailView.h"

static const int TV_WIDTH = 60;
static const int TV_HEIGHT = 80;
static const int TV_PADDING = 10;

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

void CThumbnailView::_CThumbnail::draw (HDC hdc, int currIndex) {
	if (m_thumbnail == NULL) {
		return;
	}
	xl::ui::CDIBSectionPtr thumbnail = m_thumbnail->getImage(0);
	assert(thumbnail != NULL);
	CRect rc = m_rect;
	if (currIndex == m_index) {
		rc.top -= 10;
	}

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);

	xl::ui::CDIBSectionHelper helper(thumbnail, mdc);
	int oldMode = dc.SetStretchBltMode(HALFTONE);
	dc.StretchBlt(rc.left, rc.top, rc.Width(), rc.Height(),
		mdc, 0, 0, thumbnail->getWidth(), thumbnail->getHeight(), SRCCOPY);
	dc.SetStretchBltMode(oldMode);
	helper.detach();
}


//////////////////////////////////////////////////////////////////////////
//
void CThumbnailView::_CreateThumbnailList () {
	assert(getLockLevel() > 0);
	assert(m_pImageManager->getLockLevel() > 0);

	m_thumbnails.clear();
	if (m_currIndex == -1) {
		return;
	}

	CRect rc = getClientRect();
	if (rc.Width() <= 0 || rc.Height() <= 0) {
		return;
	}

	int x1 = rc.left + rc.Width() / 2;
	int y1 = rc.top + 10;
	int x2 = x1;
	int y2 = rc.bottom;
	int index = -1;
	CImagePtr thumbnail;

	// 1. current
	x1 -= TV_WIDTH / 2;
	x2 = x1 + TV_WIDTH;
	index = m_currIndex;
	thumbnail = m_pImageManager->getCurrentCachedImage()->getThumbnailImage();
	m_thumbnails.push_back(_CThumbnailPtr(new _CThumbnail(index, CRect(x1, y1, x2, y2), thumbnail)));

	// 2. left
	while (x2 > rc.left && index > 0) {
		x2 = x1 - TV_PADDING;
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
		x1 = x2 + TV_PADDING;
		x2 = x1 + TV_WIDTH;
		thumbnail = m_pImageManager->getThumbnail(index);
		m_thumbnails.push_back(_CThumbnailPtr(new _CThumbnail(index, CRect(x1, y1, x2, y2), thumbnail)));
		index ++;
	}
}

void CThumbnailView::_OnThumbnailLoaded (int index) {
	assert(getLockLevel() > 0);
	assert(m_pImageManager->getLockLevel() > 0);

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

CThumbnailView::CThumbnailView (CImageManager *pImageManager)
	: CMultiLock(pImageManager)
	, m_pImageManager(pImageManager)
	, m_currIndex(-1)
{
	assert(m_pImageManager != NULL);
	m_pImageManager->subscribe(this);
}

CThumbnailView::~CThumbnailView() {

}

void CThumbnailView::drawMe (HDC hdc) {
	CRect rc = getClientRect();
	xl::ui::CDCHandle dc(hdc);

	CScopeMultiLock lock(this, false);
	for (_Thumbnails::iterator it = m_thumbnails.begin(); it != m_thumbnails.end(); ++ it) {
		(*it)->draw(hdc, m_currIndex);
	}
}

void CThumbnailView::onSize () {
	CScopeMultiLock lock(this, true);
	_CreateThumbnailList();
}

void CThumbnailView::onLButtonDown (CPoint pt, xl::uint) {
	int index = -1;
	CScopeMultiLock lock(this, false);
	for (_Thumbnails::iterator it = m_thumbnails.begin(); it != m_thumbnails.end(); ++ it) {
		if ((*it)->getRect().PtInRect(pt)) {
			index = (*it)->getIndex();
			break;
		}
	}
	lock.unlock();

	if (index != -1) {
		m_pImageManager->setIndex(index);
	}
}

void CThumbnailView::onEvent (EVT evt, void *param) {
	assert(m_pImageManager->getLockLevel() > 0);

	CScopeMultiLock lock(this, false);

	switch (evt) {
	case CImageManager::EVT_INDEX_CHANGED:
		assert(param);
		m_currIndex = *(int *)param;
		_CreateThumbnailList();
		invalidate();
		break;
	case CImageManager::EVT_IMAGE_LOADED:
		break;
	case CImageManager::EVT_THUMBNAIL_LOADED:
		assert(param);
		_OnThumbnailLoaded(*(int *)param);
		break;
	case CImageManager::EVT_FILELIST_READY:
		break;
	default:
		assert(false);
		break;
	}
}