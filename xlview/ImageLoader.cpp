#include <assert.h>
#include <stdio.h>
#include <setjmp.h>
#include <Windows.h>
#include "../libs/jpeglib.h"
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageLoader.h"

CImageLoader::CImageLoader () {

}

CImageLoader::~CImageLoader () {

}

CImageLoader* CImageLoader::getInstance () {
	static CImageLoader loader;
	return &loader;
}

void CImageLoader::registerPlugin (ImageLoaderPluginRawPtr plugin) {
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->getName() == plugin->getName()) {
			assert(false);
			return;
		}
	}

	m_plugins.push_back(plugin);
}

CImagePtr CImageLoader::load (const xl::tstring &fileName, IImageLoaderCancel *pCancel) {
	std::string data;
	if (!file_get_contents(fileName, data)) {
		return CImagePtr();
	}

	size_t header_length = 256;
	if (header_length > data.length()) {
		header_length = data.length();
	}
	std::string header = data.substr(0, 256);
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->checkHeader(header)) {
			return (*it)->load(data, pCancel);
		}
	}

	return CImagePtr();
}



//////////////////////////////////////////////////////////////////////////
// jpeg loader

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

class CImageLoaderPluginJpeg : public IImageLoaderPlugin
{
public:
	CImageLoaderPluginJpeg () {
		XLTRACE(_T("Jpeg decoder created!\n"));
	}

	~CImageLoaderPluginJpeg () {
		XLTRACE(_T("Jpeg decoder destroyed!\n"));
	}

	virtual xl::tstring getName () {
		return _T("JPEG Loader");
	}

	virtual bool checkHeader (const std::string &header) {
		return header.find("JFIF") == 6;
	}

	virtual CImagePtr load (const std::string &data, IImageLoaderCancel *pCancel = NULL) {
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
			return CImagePtr();
		}

		jpeg_create_decompress(&cinfo);
		jpeg_mem_src(&cinfo, (unsigned char *)data.c_str(), data.length());
		if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
			(void) jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			return CImagePtr();
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
				if (pCancel && (lines % 10) == 0 && pCancel->shouldCancel()) {
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
					assert(false); // not supported
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
			CImagePtr image(new CImage());
			image->insertImage(dib, CImage::DELAY_INFINITE);
			return image;
		}

		return CImagePtr();
	}
};

// register
namespace {
	class CJpegRegister {
		CImageLoaderPluginJpeg *m_jpeg;
	public:
		CJpegRegister () 
			: m_jpeg(new CImageLoaderPluginJpeg())
		{
			CImageLoader *pLoader = CImageLoader::getInstance();
			pLoader->registerPlugin(m_jpeg);
		}

		~CJpegRegister () {
			delete m_jpeg;
		}
	};

	static CJpegRegister jr;
}