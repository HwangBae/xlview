#include <assert.h>
#include "libxl/include/fs.h"
#include "../libs/jpeglib.h"
#include "Image.h"

//////////////////////////////////////////////////////////////////////////
// CImage::BitmapAndDelay
CImage::BitmapAndDelay::BitmapAndDelay () {
	bitmap = NULL;
	delay = 0;
}

CImage::BitmapAndDelay::~BitmapAndDelay () {
	if (bitmap) {
		::DeleteObject(bitmap);
		bitmap = NULL;
	}
	delay = 0;
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

HBITMAP CImage::getImage (xl::uint index) {
	assert(index < getImageCount());
	return m_bads[index]->bitmap;
}

void CImage::insertImage (HBITMAP bitmap, xl::uint delay) {
	assert(bitmap != NULL);
	BITMAP bm;
	VERIFY(::GetObject(bitmap, sizeof(bm), &bm) == sizeof(bm));
	if (m_width != -1 || m_height != -1) {
		assert(m_width == bm.bmWidth && m_height == bm.bmHeight);
	} else {
		m_width = bm.bmWidth;
		m_height = bm.bmHeight;
	}

	_BADPtr bad(new _BAD());
	bad->bitmap = bitmap;
	bad->delay = delay;
	m_bads.push_back(bad);
}

//////////////////////////////////////////////////////////////////////////
// 

CImagePtr CImage::loadFromFile (const xl::tstring &file) {
	xl::string content;
	if (!file_get_contents(file, content, 0)) {
		return CImagePtr();
	}

	// load JPEG
	struct jpeg_decompress_struct cinfo;
	jpeg_error_mgr em;
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */

	cinfo.err = jpeg_std_error(&em);

/* Step 1: allocate and initialize JPEG decompression object */

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_mem_src(&cinfo, (unsigned char *)content.c_str(), content.length());

/* Step 3: read file parameters with jpeg_read_header() */

	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
		(void) jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return CImagePtr();
	}

	int w = cinfo.image_width;
	int h = cinfo.image_height;
	assert(w > 0 && h > 0);

	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO)); 
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = -h;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage     = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed       = 0;
	bmi.bmiHeader.biClrImportant  = 0;

	HDC hdc = ::GetDC(NULL);
	void *data = NULL;
	HBITMAP hBitmap = ::CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &data, NULL, NULL);
	::ReleaseDC(NULL, hdc);
	if (hBitmap) {
		(void) jpeg_start_decompress(&cinfo);
		row_stride = cinfo.output_width * cinfo.output_components;
		int win_row_stride = (w * 3 + 3) & (~(unsigned int)3);
		buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

		unsigned char *p1 = (unsigned char *)data;
		unsigned char *p2 = buffer[0];
		while (cinfo.output_scanline < cinfo.output_height) {
			(void) jpeg_read_scanlines(&cinfo, buffer, 1);
			memcpy (p1, p2, row_stride);
			p1 += win_row_stride;
		}

		(void) jpeg_finish_decompress(&cinfo);
	}
	jpeg_destroy_decompress(&cinfo);

	if (hBitmap) {
		BITMAP bm;
		::GetObject(hBitmap, sizeof(bm), &bm);
		CImagePtr image(new CImage());
		image->insertImage(hBitmap, CImage::BitmapAndDelay::DELAY_INFINITE);
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