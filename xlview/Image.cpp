#include <assert.h>
#include <setjmp.h>
#include <process.h>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageConfig.h"
#include "Image.h"
#include "ImageLoader.h"


//////////////////////////////////////////////////////////////////////////
// CImage::BitmapAndDelay
CImage::Frame::Frame () {
	delay = DELAY_INFINITE;
}

CImage::Frame::~Frame () {
	delay = DELAY_INFINITE;
}


//////////////////////////////////////////////////////////////////////////
// CImage


CImage::CImage () : m_width(-1), m_height(-1) {
	//XLTRACE(_T("CImage(0x%08x) created by thread(%d)\n"), this, ::GetCurrentThreadId());
}

CImage::~CImage () {
	//XLTRACE(_T("CImage(0x%08x) destroyed by thread(%d)\n"), this, ::GetCurrentThreadId());
}


void CImage::operator = (const CImage &image) {
	clear();
	m_width = image.m_width;
	m_height = image.m_height;

	for (size_t i = 0; i < image.m_frames.size(); ++ i) {
		_FramePtr bad(new Frame());
		bad->bitmap = image.m_frames[i]->bitmap->clone();
		bad->delay = image.m_frames[i]->delay;

		m_frames.push_back(bad);
	}
}

CImagePtr CImage::clone () {
	CImage *pImage = new CImage();
	CImagePtr image(pImage);

	*pImage = *this; // call operator = ()
	
	return image;
}

void CImage::clear () {
	m_height = m_width = -1;
	m_frames.clear();
}

xl::uint CImage::getImageCount () const {
	return m_frames.size();
}

xl::uint CImage::getImageDelay (xl::uint index) const {
	assert(index < getImageCount());
	return m_frames[index]->delay;
}

xl::ui::CDIBSectionPtr CImage::getImage (xl::uint index) {
	assert(index < getImageCount());
	return m_frames[index]->bitmap;
}

void CImage::insertImage (xl::ui::CDIBSectionPtr bitmap, xl::uint delay) {
	assert(bitmap != NULL);
	if (m_width != -1 || m_height != -1) {
		assert(m_width == bitmap->getWidth() && m_height == bitmap->getHeight());
	} else {
		m_width = bitmap->getWidth();
		m_height = bitmap->getHeight();
	}

	_FramePtr bad(new Frame());
	bad->bitmap = bitmap;
	bad->delay = delay;
	m_frames.push_back(bad);
}

CImagePtr CImage::resize (int width, int height, bool highQuality) {
	if (width == m_width && height == m_height) {
		return clone();
	} else {
		assert(width > 0 && height > 0);
		CImage *pImage = new CImage();
		CImagePtr image(pImage);

		pImage->m_width = width;
		pImage->m_height = height;

		// xl::ui::CDIBSection::RESIZE_TYPE rt = highQuality ? xl::ui::CDIBSection::RT_BICUBIC : xl::ui::CDIBSection::RT_FAST;
		xl::ui::CDIBSection::RESIZE_TYPE rt = highQuality ? xl::ui::CDIBSection::RT_BOX : xl::ui::CDIBSection::RT_FAST;
		for (size_t i = 0; i < m_frames.size(); ++ i) {
			xl::ui::CDIBSectionPtr src = m_frames[i]->bitmap;
			xl::ui::CDIBSectionPtr dib = xl::ui::CDIBSection::createDIBSection(width, height, src->getBitCounts(), false);
			if (!dib) {
				return CImagePtr();
			}
			src->resize(dib.get(), rt);
			pImage->insertImage(dib, m_frames[i]->delay);
		}

		return image;
	}
}


CSize CImage::getSuitableSize (CSize szArea, CSize szImage, bool dontEnlarge) {
	CSize sz(1, 1);
	if (szArea.cx * szArea.cy <= 0 || szImage.cx * szImage.cy <= 0) {

	} else if (szArea.cx >= szImage.cx && szArea.cy >= szImage.cy && dontEnlarge) {
		sz.cx = szImage.cx;
		sz.cy = szImage.cy;
	} else if ((szArea.cx * szImage.cy) > (szImage.cx * szArea.cy)) {
		sz.cx = szImage.cx * szArea.cy / szImage.cy;
		sz.cy = szArea.cy;
	} else {
		sz.cx = szArea.cx;
		sz.cy = szImage.cy * szArea.cx / szImage.cx;
	}

	if (sz.cx <= 0) {
		sz.cx = 1;
	}
	if (sz.cy <= 0) {
		sz.cy = 1;
	}

	return sz;
}


