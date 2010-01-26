#include <assert.h>
#include <setjmp.h>
#include "libxl/include/fs.h"
#include "../libs/jpeglib.h"
#include "Image.h"

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
CImage::CImage () : m_width(-1), m_height(-1) {

}

CImage::~CImage () {

}

xl::uint CImage::getImageCount () {
	return m_bads.size();
}

SIZE CImage::getImageSize () {
	SIZE sz;
	sz.cx = m_width;
	sz.cy = m_height;
	return sz;
}

xl::uint CImage::getImageDelay (xl::uint index) {
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

//////////////////////////////////////////////////////////////////////////
// 

// for safe error handle
struct safe_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};
typedef safe_error_mgr *safe_error_mgr_ptr;

void safe_error_exit (j_common_ptr cinfo) {
	safe_error_mgr_ptr myerr = (safe_error_mgr_ptr) cinfo->err;
	// (*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}




CImagePtr CImage::loadFromFile (const xl::tstring &file) {
	xl::string content;
	if (!file_get_contents(file, content, 0)) {
		return CImagePtr();
	}
	xl::ui::CDIBSectionPtr dib;

	// load JPEG
	struct jpeg_decompress_struct cinfo;
	safe_error_mgr em;
	JSAMPARRAY buffer;
	int row_stride;

	cinfo.err = jpeg_std_error(&em.pub);
	if (setjmp(em.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return CImagePtr();
	}


	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, (unsigned char *)content.c_str(), content.length());

	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
		(void) jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return CImagePtr();
	}

	int w = cinfo.image_width;
	int h = cinfo.image_height;
	assert(w > 0 && h > 0);
	dib = xl::ui::CDIBSection::createDIBSection(w, h, 24, false);

	if (dib) {
		(void) jpeg_start_decompress(&cinfo);
		row_stride = cinfo.output_width * cinfo.output_components;
		int win_row_stride = dib->getStride();
		buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

		unsigned char *p1 = (unsigned char *)dib->getData();
		unsigned char *p2 = buffer[0];
		while (cinfo.output_scanline < cinfo.output_height) {
			(void) jpeg_read_scanlines(&cinfo, buffer, 1);
			memcpy (p1, p2, row_stride);
			p1 += win_row_stride;
		}

		(void) jpeg_finish_decompress(&cinfo);
	}
	jpeg_destroy_decompress(&cinfo);

	if (dib) {
		CImagePtr image(new CImage());
		image->insertImage(dib, CImage::BitmapAndDelay::DELAY_INFINITE);
		return image;
	}

	return CImagePtr();
}

SIZE CImage::getSuitableSize (SIZE szArea, SIZE szImage) {
	SIZE sz;
	if (szArea.cx * szArea.cy == 0 || szImage.cx * szImage.cy == 0) {
		sz.cx = 1;
		sz.cy = 1;
	} else if ((szArea.cx * szImage.cy) > (szImage.cx * szArea.cy)) {
		sz.cx = szImage.cx * szArea.cy / szImage.cy;
		sz.cy = szArea.cy;
	} else {
		sz.cx = szArea.cx;
		sz.cy = szImage.cy * szArea.cx / szImage.cx;
	}

	return sz;
}