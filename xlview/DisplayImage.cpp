#include <assert.h>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageConfig.h"
#include "DisplayImage.h"


CDisplayImage::CDisplayImage (const xl::tstring &fileName)
	: m_zooming(false)
	, m_fileName(fileName)
	, m_widthReal(-1)
	, m_heightReal(-1)
{
	assert(xl::file_exists(m_fileName));
}

CDisplayImage::~CDisplayImage ()
{
}

CDisplayImagePtr CDisplayImage::clone () {
	xl::CScopeLock lock(this);

	CDisplayImagePtr image(new CDisplayImage(m_fileName));
	CDisplayImage *pImage = image.get();
	pImage->m_widthReal = m_widthReal;
	pImage->m_heightReal = m_heightReal;

	if (m_imgThumbnail) {
		pImage->m_imgThumbnail = m_imgThumbnail->clone();
	}

	if (m_imgZoomed) {
		pImage->m_imgZoomed = m_imgZoomed->clone();
	}

	if (m_imgRealSize) {
		pImage->m_imgRealSize = m_imgRealSize->clone();
	}

	return image;
}

bool CDisplayImage::loadZoomed (int width, int height, IImageOperateCancel *pCancel) {
	xl::CScopeLock lock(this);;
	bool clearRealSize = !m_imgRealSize;
	if (!m_imgRealSize && !loadRealSize(pCancel)) {
		return false;
	}
	assert(m_imgRealSize->getImageCount() > 0);
	CImagePtr imgRealSize = m_imgRealSize;

	xl::CTimerLogger logger(false, _T("** resize image from (%d - %d) to (%d - %d) cost"),
		imgRealSize->getImageWidth(), imgRealSize->getImageHeight(), width, height);

	m_imgZooming = imgRealSize->resize(width, height, false);

	logger.log();
	assert(!m_zooming);
	m_zooming = true;
	lock.unlock();

	CImagePtr img = imgRealSize->resize(width, height);

	lock.lock(this);
	m_imgZoomed = img;
	m_imgZooming.reset();
	m_zooming = false;
	lock.unlock();

	if (clearRealSize) {
		this->clearRealSize ();
	}

	return (!!m_imgZoomed) && (m_imgZoomed->getImageCount() > 0);
}

bool CDisplayImage::loadRealSize (IImageOperateCancel *pCancel) {
	xl::CScopeLock lock(this);;
	xl::tstring fileName = getFileName();
	assert(m_imgRealSize == NULL);
	lock.unlock();
	assert(xl::file_exists(fileName));
	CImageLoader *pLoader = CImageLoader::getInstance();
	CImagePtr image = pLoader->load(fileName, pCancel);
	if (!image) {
		return false;
	} else {
		lock.lock(this);
		if (pCancel->shouldCancel()) {
			lock.unlock();
			return false;
		}

		CImage *pImage = image.get();
		if (m_widthReal != -1 && m_heightReal != -1) {
			assert(m_widthReal == pImage->getImageWidth() && m_heightReal == pImage->getImageHeight());
		} else {
			m_widthReal = pImage->getImageWidth();
			m_heightReal = pImage->getImageHeight();
		}
		m_imgRealSize = image;
		XLTRACE(_T("****** load image on 0x%08x (%s)\n"), pImage, fileName.c_str());

		if (m_imgThumbnail == NULL) {
			m_imgThumbnail = image->resize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, false);
		}

		return true;
	}
}

bool CDisplayImage::loadThumbnail (IImageOperateCancel *pCancel) {
	xl::CScopeLock lock(this);;
	if (m_imgZoomed) {
		assert(m_imgZoomed->getImageCount() > 0);
		m_imgThumbnail = m_imgZoomed->resize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, false);
	} else if (m_imgRealSize) {
		assert(m_imgRealSize->getImageCount() > 0);
		m_imgThumbnail = m_imgRealSize->resize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, false);
	} else {
		if (!loadRealSize(pCancel)) {
			return false;
		}
		assert(m_imgRealSize->getImageCount() > 0);
		m_imgThumbnail = m_imgRealSize->resize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, false);
		clearRealSize();
	}
	return true;
}

void CDisplayImage::clearThumbnail () {
	xl::CScopeLock lock(this);;
	m_imgThumbnail.reset();
}

void CDisplayImage::clearRealSize () {
	xl::CScopeLock lock(this);;
	m_imgRealSize.reset();
}

void CDisplayImage::clearZoomed () {
	xl::CScopeLock lock(this);;
	m_imgZoomed.reset();
}

void CDisplayImage::clear () {
	m_imgRealSize.reset();
	m_imgZoomed.reset();
	m_imgThumbnail.reset();
}


xl::tstring CDisplayImage::getFileName () const {
	xl::CScopeLock lock(this);;
	xl::tstring fileName = m_fileName;
	return fileName;
}

int CDisplayImage::getRealWidth () const {
	return m_widthReal;
}

int CDisplayImage::getRealHeight () const {
	return m_heightReal;
}

CSize CDisplayImage::getRealSize () const {
	return CSize(m_widthReal, m_heightReal);
}

CImagePtr CDisplayImage::getThumbnail () {
	lock();
	CImagePtr img =  m_imgThumbnail;
	unlock();
	return img;
}

CImagePtr CDisplayImage::getZoomedImage () {
	CImagePtr img;
	lock();
	if (m_zooming) {
		img = m_imgZooming;
	} else {
		img = m_imgZoomed;
	}
	unlock();
	return img;
}

CImagePtr CDisplayImage::getRealSizeImage () {
	lock();
	CImagePtr img = m_imgRealSize;
	unlock();
	return img;
}
