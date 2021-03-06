#include <assert.h>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageConfig.h"
#include "CachedImage.h"
#include "ImageManager.h"

CCachedImage::CCachedImage (const xl::tstring &fileName)
	: m_fileName(fileName)
	, m_szImage(-1, -1)
{
	assert(xl::file_exists(m_fileName));
}

CCachedImage::~CCachedImage () {

}

bool CCachedImage::loadSuitable (CSize szView, xl::ILongTimeRunCallback *pCallback) {
	assert(szView.cx >= MIN_ZOOM_WIDTH);
	assert(szView.cy >= MIN_ZOOM_HEIGHT);

	xl::CScopeLock lock(this);
		if (m_suitableImage != NULL) {
			return true;
		}
		CImagePtr thumbnailImage = m_thumbnailImage;
	lock.unlock();

	// below is lock free
	CSize szImage;
	CImageLoader *pLoader = CImageLoader::getInstance();
	CImagePtr image = pLoader->loadSuitable(m_fileName, &szImage, szView, pCallback);
	if (!image) {
		return false;
	}

	if (thumbnailImage == NULL) {
		CSize szThumbnail(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
		szThumbnail = CImage::getSuitableSize(szThumbnail, szImage, false);
		thumbnailImage = image->resize(szThumbnail.cx, szThumbnail.cy, true, pCallback);
	}

	// lock again
	lock.lock(this);
		m_szImage = szImage;
		m_suitableImage = image;
		m_thumbnailImage = thumbnailImage;

		image.reset();
		thumbnailImage.reset();
	lock.unlock();
	return true;
}

bool CCachedImage::loadThumbnail (bool fastOnly, xl::ILongTimeRunCallback *pCancel) {
	xl::CScopeLock lock(this);
		if (m_thumbnailImage != NULL) {
			return true;
		}
		xl::tstring fileName = m_fileName;
	lock.unlock();

	// below is lock free
	assert(xl::file_exists(fileName)); // TODO
	CImageLoader *pLoader = CImageLoader::getInstance();
	CSize szImageRS;
	CImagePtr image = pLoader->loadThumbnail(fileName, szImageRS, CSize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT), fastOnly, pCancel);
	if (image == NULL) {
		return false;
	}

	lock.lock(this);
		m_thumbnailImage = image;
		m_szImage = szImageRS;

		image.reset();
	lock.unlock();
	return true;
}

void CCachedImage::setSuitableImage (CImagePtr image, CSize realSize) {
	assert(image != NULL);
	xl::CScopeLock lock(this);
	if (m_suitableImage == NULL || m_suitableImage->getImageSize() != image->getImageSize()) {
		if (m_szImage != CSize(-1, -1)) {
			assert(m_szImage == realSize);
		}
		CImagePtr thumbnail = m_thumbnailImage;
		lock.unlock();

		if (thumbnail == NULL) {
			CSize szThumbnail(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
			szThumbnail = CImage::getSuitableSize(szThumbnail, realSize, false);
			thumbnail = image->resize(szThumbnail.cx, szThumbnail.cy, true);
		}

		lock.lock(this);
			m_szImage = realSize;
			m_suitableImage = image;
			if (m_thumbnailImage == NULL) {
				m_thumbnailImage = thumbnail;
			}
		lock.unlock();
	}
}

void CCachedImage::clear (bool clearThumbnail) {
	lock();
	m_suitableImage.reset();
	if (clearThumbnail) {
		m_thumbnailImage.reset();
		m_szImage.cx = -1;
		m_szImage.cy = -1;
	}
	unlock();
}

xl::tstring CCachedImage::getFileName () const {
	lock();
	xl::tstring fileName = m_fileName;
	unlock();
	return fileName;
}

CSize CCachedImage::getImageSize () const {
	lock();
	CSize sz = m_szImage;
	unlock();

	return sz;
}

CImagePtr CCachedImage::getCachedImage () const {
	lock();
	CImagePtr image = m_suitableImage;
	if (image == NULL) {
		image = m_thumbnailImage;
	}
	unlock();

	return image;
}

CImagePtr CCachedImage::getSuitableImage () const {
	lock();
	CImagePtr image = m_suitableImage;
	unlock();
	return image;
}

CImagePtr CCachedImage::getThumbnailImage () const {
	lock();
	CImagePtr image = m_thumbnailImage;
	unlock();
	return image;
}
