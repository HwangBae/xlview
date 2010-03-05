#include <assert.h>
#include "libxl/include/ui/Gdi.h"
#include "ThumbnailView.h"

static const int TV_WIDTH = 60;
static const int TV_HEIGHT = 80;

//////////////////////////////////////////////////////////////////////////
// 
CThumbnailView::_CThumbnail::_CThumbnail (int index, CRect rc, CImagePtr thumbnail)
	: m_index(index), m_rect(rc), m_thumbnail(thumbnail)
{

}

CThumbnailView::_CThumbnail::~_CThumbnail () {

}

void CThumbnailView::_CThumbnail::draw (HDC hdc, int currIndex) {
	if (m_thumbnail == NULL) {
		return;
	}
	xl::ui::CDIBSectionPtr thumbnail = m_thumbnail->getImage(0);
	assert(thumbnail != NULL);

	xl::ui::CDCHandle dc(hdc);
	xl::ui::CDC mdc;
	mdc.CreateCompatibleDC(dc);

	xl::ui::CDIBSectionHelper helper(thumbnail, mdc);
	int oldMode = dc.SetStretchBltMode(HALFTONE);
	dc.StretchBlt(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(),
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
	int y1 = rc.top;
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
		x2 = x1;
		x1 -= TV_WIDTH;
		index --;
		thumbnail = m_pImageManager->getThumbnail(index);
		m_thumbnails.push_back(_CThumbnailPtr(new _CThumbnail(index, CRect(x1, y1, x2, y2), thumbnail)));
	}

	// 3. right
	x1 = rc.left + rc.Width() / 2;
	x1 -= TV_WIDTH / 2;
	x2 = x1 + TV_WIDTH;
	int count = (int)m_pImageManager->getImageCount();
	while (x2 < rc.right && index < count) {
		x1 = x2;
		x2 = x1 + TV_WIDTH;
		index ++;
		thumbnail = m_pImageManager->getThumbnail(index);
		m_thumbnails.push_back(_CThumbnailPtr(new _CThumbnail(index, CRect(x1, y1, x2, y2), thumbnail)));
	}
}

CThumbnailView::CThumbnailView (CImageManager *pImageManager)
	: m_pImageManager(pImageManager)
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
	xl::CScopeLock lock(this);

	for (_Thumbnails::iterator it = m_thumbnails.begin(); it != m_thumbnails.end(); ++ it) {
		(*it)->draw(hdc, m_currIndex);
	}
}

void CThumbnailView::onEvent (EVT evt, void *param) {
	assert(m_pImageManager->getLockLevel() > 0);

	xl::CScopeLock lock(this);

	switch (evt) {
	case CImageManager::EVT_INDEX_CHANGED:
		assert(param);
		// _OnIndexChanged(*(int *)param);
		m_currIndex = *(int *)param;
		_CreateThumbnailList();
		invalidate();
		break;
	case CImageManager::EVT_IMAGE_LOADED:
		break;
	case CImageManager::EVT_THUMBNAIL_LOADED:
		assert(param);
		// _OnThumbnailLoaded(*(int *)param);
		break;
	case CImageManager::EVT_FILELIST_READY:
		break;
	default:
		assert(false);
		break;
	}
}