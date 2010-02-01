#include <assert.h>
#include <setjmp.h>
#include <process.h>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "../libs/jpeglib.h"
#include "Image.h"

//////////////////////////////////////////////////////////////////////////
// jpeg error handle

// for safe error handle
struct safe_jpeg_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};
typedef safe_jpeg_error_mgr *safe_jpeg_error_mgr_ptr;

void safe_jpeg_error_exit (j_common_ptr cinfo) {
	safe_jpeg_error_mgr_ptr myerr = (safe_jpeg_error_mgr_ptr) cinfo->err;
	longjmp(myerr->setjmp_buffer, 1);
}


//////////////////////////////////////////////////////////////////////////
// CImage::BitmapAndDelay
CImage::BitmapAndDelay::BitmapAndDelay () {
	delay = DELAY_INFINITE;
}

CImage::BitmapAndDelay::~BitmapAndDelay () {
	delay = DELAY_INFINITE;
}


//////////////////////////////////////////////////////////////////////////
// CImage

#if 0
void CImage::_CreateThumbnail () {
	// xl::CTimerLogger logger(_T("create thumbnail"));

	assert(getImageCount() > 0);
	assert(m_thumbnail == NULL);
	xl::ui::CDIBSectionPtr dib = getImage(0);
	SIZE szArea = {(int)(THUMBNAIL_WIDTH * 2.5), (int)(THUMBNAIL_HEIGHT * 2.5)};
	SIZE szImage = getImageSize();
	SIZE szThumbnail = getSuitableSize(szArea, szImage);

	xl::ui::CDIBSectionPtr thumbnail = dib->resize(szThumbnail.cx, szThumbnail.cy, false);
	szArea.cx = THUMBNAIL_WIDTH;
	szArea.cy = THUMBNAIL_HEIGHT;
	szThumbnail = getSuitableSize(szArea, szImage);
	m_thumbnail = thumbnail->resize(szThumbnail.cx, szThumbnail.cy, true);
}
#endif


CImage::CImage () : m_width(-1), m_height(-1) {

}

CImage::~CImage () {

}

bool CImage::load (const xl::tstring &file, ICancel *pCancel) {
	xl::string content;
	if (!file_get_contents(file, content, 0)) {
		return false;
	}
	xl::ui::CDIBSectionPtr dib;

	// load JPEG
	struct jpeg_decompress_struct cinfo;
	safe_jpeg_error_mgr em;
	JSAMPARRAY buffer;
	int row_stride;

	cinfo.err = jpeg_std_error(&em.pub);
	em.pub.error_exit = safe_jpeg_error_exit;
	if (setjmp(em.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return false;
	}


	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, (unsigned char *)content.c_str(), content.length());

	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
		(void) jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return false;
	}

	int w = cinfo.image_width;
	int h = cinfo.image_height;
	assert(w > 0 && h > 0);
	// int bit_counts = cinfo.out_color_space == JCS_GRAYSCALE ? 8 : 24;
	dib = xl::ui::CDIBSection::createDIBSection(w, h, 24, false);

	bool canceled = false;
	if (dib) {
		(void) jpeg_start_decompress(&cinfo);
		row_stride = cinfo.output_width * cinfo.output_components;
		int win_row_stride = dib->getStride();
		buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

		unsigned char *p1 = (unsigned char *)dib->getData();
		unsigned char *p2 = buffer[0];
		int lines = 0;
		while (cinfo.output_scanline < cinfo.output_height) {
			if (pCancel && (lines % 10) == 0 && pCancel->cancelLoading()) {
				canceled = true;
				break;
			}
			jpeg_read_scanlines(&cinfo, buffer, 1);
			if (cinfo.out_color_space == JCS_GRAYSCALE) {
				unsigned char *dst = p1;
				unsigned char *src = p2;
				for (int i = 0; i < w; ++ i) {
					unsigned char c = *src ++;
					*dst ++ = c;
					*dst ++ = c;
					*dst ++ = c;
				}
			} else if (cinfo.out_color_space == JCS_RGB) {
				memcpy (p1, p2, row_stride);
			} else {
				assert(false); // not suppported
				canceled = true;
				break;
			}
			p1 += win_row_stride;
			++ lines;
		}

		if (canceled) {
			jpeg_abort_decompress(&cinfo);
		} else {
			jpeg_finish_decompress(&cinfo);
		}
	}
	jpeg_destroy_decompress(&cinfo);

	if (dib && !canceled) {
		insertImage(dib, CImage::BitmapAndDelay::DELAY_INFINITE);
		return true;
	}

	return false;
}

void CImage::operator = (const CImage &image) {
	clear();
	m_width = image.m_width;
	m_height = image.m_height;

	for (size_t i = 0; i < image.m_bads.size(); ++ i) {
		_BADPtr bad(new _BAD());
		bad->bitmap = image.m_bads[i]->bitmap->clone();
		bad->delay = image.m_bads[i]->delay;

		m_bads.push_back(bad);
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
	m_bads.clear();
}

xl::uint CImage::getImageCount () const {
	return m_bads.size();
}

SIZE CImage::getImageSize () const {
	SIZE sz;
	sz.cx = m_width;
	sz.cy = m_height;
	return sz;
}

xl::uint CImage::getImageDelay (xl::uint index) const {
	assert(index < getImageCount());
	return m_bads[index]->delay;
}

xl::ui::CDIBSectionPtr CImage::getImage (xl::uint index) {
	assert(index < getImageCount());
	return m_bads[index]->bitmap;
}

void CImage::insertImage (xl::ui::CDIBSectionPtr bitmap, xl::uint delay) {
	assert(bitmap != NULL);
	if (m_width != -1 || m_height != -1) {
		assert(m_width == bitmap->getWidth() && m_height == bitmap->getHeight());
	} else {
		m_width = bitmap->getWidth();
		m_height = bitmap->getHeight();
	}

	_BADPtr bad(new _BAD());
	bad->bitmap = bitmap;
	bad->delay = delay;
	m_bads.push_back(bad);
}

CImagePtr CImage::resize (int width, int height, bool usehalftone) {
	if (width == m_width && height == m_height) {
		return clone();
	} else {
		assert(width > 0 && height > 0);
		CImage *pImage = new CImage();
		CImagePtr image(pImage);

		pImage->m_width = width;
		pImage->m_height = height;

		for (size_t i = 0; i < m_bads.size(); ++ i) {
			xl::ui::CDIBSectionPtr src = m_bads[i]->bitmap;
			xl::ui::CDIBSectionPtr dib = src->resize(width, height, usehalftone, src->getBitCounts());
			pImage->insertImage(dib, m_bads[i]->delay);
		}

		return image;
	}
}


SIZE CImage::getSuitableSize (SIZE szArea, SIZE szImage, bool dontEnlarge) {
	SIZE sz;
	if (szArea.cx * szArea.cy <= 0 || szImage.cx * szImage.cy <= 0) {
		sz.cx = 1;
		sz.cy = 1;
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

	return sz;
}


//////////////////////////////////////////////////////////////////////////
// CDisplayImage

CDisplayImage::CDisplayImage (const xl::tstring &fileName)
	: m_fileName(fileName)
	, m_widthReal(-1)
	, m_heightReal(-1)
{
	assert(xl::file_exists(m_fileName));
	::InitializeCriticalSection(&m_cs);
}

CDisplayImage::~CDisplayImage ()
{
	::DeleteCriticalSection(&m_cs);
}

CDisplayImagePtr CDisplayImage::clone () {
	xl::CSimpleLock lock(&m_cs);
	CDisplayImage *pImage = new CDisplayImage(m_fileName);
	CDisplayImagePtr image(pImage);

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

bool CDisplayImage::loadZoomed (int width, int height, CImage::ICancel *pCancel) {
	xl::CSimpleLock lock(&m_cs);
	bool clearRealSize = !m_imgRealSize;
	if (!m_imgRealSize && !loadRealSize(pCancel)) {
		return false;
	}
	assert(m_imgRealSize->getImageCount() > 0);
	m_imgZoomed.reset();

	m_imgZoomed = m_imgRealSize->resize(width, height);
	if (clearRealSize) {
		this->clearRealSize ();
	}

	return (!!m_imgZoomed) && (m_imgZoomed->getImageCount() > 0);
}

bool CDisplayImage::loadRealSize (CImage::ICancel *pCancel) {
	xl::CSimpleLock lock(&m_cs);
	assert(m_imgRealSize == NULL);
	assert(xl::file_exists(getFileName()));
	CImage *pImage = new CImage();
	CImagePtr image(pImage);
	if (!pImage->load(getFileName(), pCancel)) {
		return false;
	} else {
		if (m_widthReal != -1 && m_heightReal != -1) {
			assert(m_widthReal == pImage->getImageWidth() && m_heightReal == pImage->getImageHeight());
		} else {
			m_widthReal = pImage->getImageWidth();
			m_heightReal = pImage->getImageHeight();
		}
		m_imgRealSize = image;
		return true;
	}
}

bool CDisplayImage::loadThumbnail (CImage::ICancel *pCancel) {
	xl::CSimpleLock lock(&m_cs);
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
	xl::CSimpleLock lock(&m_cs);
	m_imgThumbnail.reset();
}

void CDisplayImage::clearRealSize () {
	xl::CSimpleLock lock(&m_cs);
	m_imgRealSize.reset();
}

void CDisplayImage::clearZoomed () {
	xl::CSimpleLock lock(&m_cs);
	m_imgZoomed.reset();
}

void CDisplayImage::clear () {
	m_imgRealSize.reset();
	m_imgZoomed.reset();
	m_imgThumbnail.reset();
}


xl::tstring CDisplayImage::getFileName () const {
	return m_fileName;
}

int CDisplayImage::getRealWidth () const {
	return m_widthReal;
}

int CDisplayImage::getRealHeight () const {
	return m_heightReal;
}