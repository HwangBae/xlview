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


CImage::CImage () : m_width(-1), m_height(-1) {

}

CImage::~CImage () {

}

void CImage::operator = (const CImage &image) {
	clear();
	m_width = image.m_width;
	m_height = image.m_height;
	if (image.m_thumbnail) {
		m_thumbnail = image.m_thumbnail->clone();
	}

	for (size_t i = 0; i < image.m_bads.size(); ++ i) {
		_BADPtr bad(new _BAD());
		bad->bitmap = image.m_bads[i]->bitmap->clone();
		bad->delay = image.m_bads[i]->delay;

		m_bads.push_back(bad);
	}
}

void CImage::clear () {
	m_height = m_width = -1;
	m_bads.clear();
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
			memcpy (p1, p2, row_stride);
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



xl::uint CImage::getImageCount () const {
	return m_bads.size();
}

SIZE CImage::getImageSize () const {
	SIZE sz;
	sz.cx = m_width;
	sz.cy = m_height;
	return sz;
}

SIZE CImage::getThumbnailSize () const {
	SIZE sz;
	sz.cx = THUMBNAIL_WIDTH;
	sz.cy = THUMBNAIL_HEIGHT;
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

xl::ui::CDIBSectionPtr CImage::getThumbnail () {
	return m_thumbnail;
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

	if (getImageCount() == 1) {
		_CreateThumbnail();
	}
}





SIZE CImage::getSuitableSize (SIZE szArea, SIZE szImage) {
	SIZE sz;
	if (szArea.cx * szArea.cy <= 0 || szImage.cx * szImage.cy <= 0) {
		sz.cx = 1;
		sz.cy = 1;
	} else if (szArea.cx >= szImage.cx && szArea.cy >= szImage.cy) {
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
	, m_isThumbnail(false)
	, m_widthReal(-1)
	, m_heightReal(-1)
{
	assert(xl::file_exists(m_fileName));
}

CDisplayImage::~CDisplayImage ()
{
}

CDisplayImagePtr CDisplayImage::clone () {
	CDisplayImage *pImage = new CDisplayImage(m_fileName);
	pImage->m_isThumbnail = m_isThumbnail;
	pImage->m_widthReal = m_widthReal;
	pImage->m_heightReal = m_heightReal;

	*(CImage *)pImage = *(CImage *)this;

	CDisplayImagePtr image(pImage);
	return image;
}

bool CDisplayImage::load (ICancel *pCancel) {
	assert(xl::file_exists(m_fileName));
	bool loaded = CImage::load(m_fileName, pCancel);
	if (loaded) {
		m_isThumbnail = false;
		m_widthReal = m_width;
		m_heightReal = m_height;
	}
	return loaded;
}

void CDisplayImage::resize (int w, int h) {
	m_width = w;
	m_height = h;

	for (size_t i = 0; i < getImageCount(); ++ i) {
		xl::ui::CDIBSectionPtr dib = getImage(i)->resize(w, h);
		m_bads[i]->bitmap = dib;
	}
}

void CDisplayImage::changeToThumbnail (CDisplayImagePtr source) {
	assert(source->getImageCount() > 0);
	CImage::clear();
	m_fileName = source->getFileName();
	m_isThumbnail = true;
	m_widthReal = source->m_widthReal;
	m_heightReal = source->m_heightReal;
	assert(m_widthReal > 0 && m_heightReal > 0);

	if (source->m_thumbnail) {
		insertImage(source->m_thumbnail->clone(), _BAD::DELAY_INFINITE);
	} else {
		insertImage(source->getImage(0)->resize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, false), _BAD::DELAY_INFINITE);
	}
}


xl::tstring CDisplayImage::getFileName () const {
	return m_fileName;
}

bool CDisplayImage::isThumbnail () const {
	return m_isThumbnail;
}

int CDisplayImage::getRealWidth () const {
	return m_widthReal;
}

int CDisplayImage::getRealHeight () const {
	return m_heightReal;
}