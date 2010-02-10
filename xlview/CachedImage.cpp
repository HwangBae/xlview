#include <assert.h>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageConfig.h"
#include "CachedImage.h"
#include "ImageManager.h"

CCachedImage::CCachedImage (const xl::tstring &fileName)
	: m_fileName(fileName)
	, m_imageSize(-1, -1)
{
	assert(xl::file_exists(m_fileName));
}

CCachedImage::~CCachedImage () {

}

bool CCachedImage::load (CSize szView, IImageOperateCancel *pCancel) {
	assert(szView.cx >= MIN_VIEW_WIDTH);
	assert(szView.cy >= MIN_VIEW_HEIGHT);

	xl::CScopeLock lock(this);
		if (m_suitableImage != NULL) {
			return true;
		}
		CImagePtr thumbnailImage = m_thumbnailImage;
	lock.unlock();

	// below is lock free
	CImageLoader *pLoader = CImageLoader::getInstance();
	CImagePtr image = pLoader->load(m_fileName, pCancel);
	if (!image) {
		return false;
	}
	CSize szImage = image->getImageSize();
	CSize szSuitable = CImage::getSuitableSize(szView, szImage);

	if (thumbnailImage == NULL) {
		CSize szThumbnail(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
		szThumbnail = CImage::getSuitableSize(szThumbnail, szImage, false);
		thumbnailImage = image->resize(szThumbnail.cx, szThumbnail.cy, true);
	}

	if (szSuitable != szImage) {
		image = image->resize(szSuitable.cx, szSuitable.cy, true);
		if (image == NULL) {
			return false;
		}
	}

	// lock again
	lock.lock(this);
		m_imageSize = szImage;
		m_suitableImage = image;
		m_thumbnailImage = thumbnailImage;

		image.reset();
		thumbnailImage.reset();
	lock.unlock();
	return true;
}

bool CCachedImage::loadThumbnail (IImageOperateCancel *pCancel) {
	xl::CScopeLock lock(this);
		if (m_thumbnailImage != NULL) {
			return true;
		}
		xl::tstring fileName = m_fileName;
	lock.unlock();

	// below is lock free
	assert(xl::file_exists(fileName)); // TODO
	CImageLoader *pLoader = CImageLoader::getInstance();
	int imageWidth = 0, imageHeight = 0;
	CImagePtr image = pLoader->loadThumbnail(fileName, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, imageWidth, imageHeight, pCancel);
	if (image == NULL) {
		return false;
	}

	lock.lock(this);
		m_thumbnailImage = image;
		m_imageSize.cx = imageWidth;
		m_imageSize.cy = imageHeight;

		image.reset();
	lock.unlock();
	return true;
}

void CCachedImage::clear (bool clearThumbnail) {
	lock();
	m_suitableImage.reset();
	if (clearThumbnail) {
		m_thumbnailImage.reset();
		m_imageSize.cx = -1;
		m_imageSize.cy = -1;
	}
	unlock();
}

CSize CCachedImage::getImageSize () {
	lock();
	CSize sz = m_imageSize;
	unlock();

	return sz;
}

CImagePtr CCachedImage::getCachedImage () {
	lock();
	CImagePtr image = m_suitableImage;
	if (image == NULL) {
		image = m_thumbnailImage;
	}
	image = image->clone();
	unlock();

	return image;
}
