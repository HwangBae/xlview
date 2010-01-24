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

	LONG len = w * h;
	unsigned char *bits = new unsigned char[len * 4];
	std::auto_ptr<unsigned char> bits_ptr(bits); 

/* Step 4: set parameters for decompression */

	/**
	 * In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

/* Step 5: Start decompressor */

	(void) jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	* In this example, we need to make an output work buffer of the right size.
	*/ 
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	int y = 0;
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		* Here the array is only one element long, but you could ask for
		* more than one scanline at a time if that's more convenient.
		*/
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		// put_scanline_someplace(buffer[0], row_stride);
		unsigned char *p1 = bits + w * 4 * y;
		unsigned char *p2 = buffer[0];
		for (int i = 0; i < w; ++ i) {
			*(unsigned int *)p1 = *(unsigned int *)p2;
			*(p1 + 3) = 0;
			p1 += 4;
			p2 += 3;
		}
		++ y;
	}

	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO)); 
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = -h;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage     = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed       = 0;
	bmi.bmiHeader.biClrImportant  = 0;

	HDC hdc = ::GetDC(NULL);
	HBITMAP hBitmap = ::CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, bits, &bmi, DIB_RGB_COLORS);
	::ReleaseDC(NULL, hdc);
	if (hBitmap) {
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