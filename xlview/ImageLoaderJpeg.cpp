#include <stdio.h>
#include <setjmp.h>
#include <basetsd.h> // for refedinition for INT32, and so on (which defined in jmorecfg.h)
#include "../libs/jpeglib.h"
#include "libxl/include/utilities.h"
#include "ImageLoader.h"


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
// CImageLoaderPluginJpeg

class CImageLoaderPluginJpeg : public IImageLoaderPlugin
{
public:
	CImageLoaderPluginJpeg () {
		XLTRACE(_T("Jpeg decoder created!\n"));
	}

	~CImageLoaderPluginJpeg () {
		XLTRACE(_T("Jpeg decoder destroyed!\n"));
	}

	virtual xl::tstring getPluginName () {
		return _T("JPEG Loader");
	}

	virtual void registerExt (ImageExts &exts) {
		static xl::tchar *extensions[] = {
			_T("jpg"),
			_T("jpeg"),
			_T("jif"),
			_T("jfif"),
		};
		exts.reserve(exts.size() + COUNT_OF(extensions));
		for (int i = 0; i < COUNT_OF(extensions); ++ i) {
			exts.push_back(extensions[i]);
		}
	}

	virtual bool checkHeader (const std::string &header) {
		if (header.length() < 3) {
			return false;
		} else {
			const unsigned char *data = (const unsigned char *)header.c_str();
			return *data ++ == 0xFF && *data ++ == 0xD8 && *data ++ == 0xFF;
		}
	}

	virtual bool readHeader (const std::string &data, ImageHeaderInfo &info) {
		struct jpeg_decompress_struct cinfo;
		safe_jpeg_error_mgr em;

		cinfo.err = jpeg_std_error(&em.pub);
		em.pub.error_exit = safe_jpeg_error_exit;
		if (setjmp(em.setjmp_buffer)) {
			jpeg_destroy_decompress(&cinfo);
			return false;
		}

		jpeg_create_decompress(&cinfo);
		jpeg_mem_src(&cinfo, (unsigned char *)data.c_str(), data.length());
		if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
			(void) jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			return false;
		}

		info.width = cinfo.image_width;
		info.height = cinfo.image_height;
		info.bitcount = 24;
		info.frame_count = 1;
		assert(info.width > 0 && info.height > 0);

		jpeg_destroy_decompress(&cinfo);
		return true;
	}


	virtual bool load (CImagePtr image, const std::string &data, xl::ui::CResizeEngine *pResizer = NULL, xl::ILongTimeRunCallback *pCallback = NULL) {
		// load JPEG
		const int LINE_BLOCK = 32;
		struct jpeg_decompress_struct cinfo;
		safe_jpeg_error_mgr em;
		JSAMPARRAY buffer;
		int row_stride;
		int lines = 0;

		assert(image->getImageCount() == 1);
		xl::ui::CDIBSectionPtr dib = image->getImage(0);
		xl::ui::CDIBSectionPtr dibTmp;

		cinfo.err = jpeg_std_error(&em.pub);
		em.pub.error_exit = safe_jpeg_error_exit;
		if (setjmp(em.setjmp_buffer)) {
			jpeg_destroy_decompress(&cinfo);
			dib.reset();
			dibTmp.reset();
			return lines > 0;
		}

		jpeg_create_decompress(&cinfo);
		jpeg_mem_src(&cinfo, (unsigned char *)data.c_str(), data.length());
		if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
			(void) jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			return false;
		}

		int w = cinfo.image_width;
		int h = cinfo.image_height;
		assert((w == image->getImageWidth() && h == image->getImageHeight()) || pResizer != NULL);
		if (image->getImageWidth() < w) {
			dib = xl::ui::CDIBSection::createDIBSection(w, LINE_BLOCK, 24);
			dibTmp = xl::ui::CDIBSection::createDIBSection(image->getImageWidth(), h, 24);
			if (dib == NULL || dibTmp == NULL) {
				return false; // out of memory
			}
		}

		bool canceled = false;
		int process_line = 0;
		(void) jpeg_start_decompress(&cinfo);
		row_stride = cinfo.output_width * cinfo.output_components;
		int win_row_stride = dib->getStride();
		buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

		unsigned char *p1 = (unsigned char *)dib->getData();
		unsigned char *p2 = buffer[0];
		while (cinfo.output_scanline < cinfo.output_height) {
			if (lines % LINE_BLOCK == 0 && lines > 0) {
				if (pCallback && pCallback->shouldStop()) {
					canceled = true;
					break;
				}
				if (image->getImageWidth() < w) {
					if (!pResizer->horizontalFilter(dib.get(), LINE_BLOCK, dibTmp.get(), process_line, LINE_BLOCK, pCallback)) {
						canceled = true;
						break;
					}
					p1 = (unsigned char *)dib->getData();
					process_line += LINE_BLOCK;
				}
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
			} else if (cinfo.out_color_space == JCS_CMYK) {
				assert(cinfo.out_color_components == 4);
				unsigned char *dst = p1;
				unsigned char *src = p2;
				for (int i = 0; i < w; ++ i) {
					unsigned int K = (unsigned int)src[3];
					dst[2]   = (unsigned char)((K * src[0]) / 255);
					dst[1] = (unsigned char)((K * src[1]) / 255);
					dst[0]  = (unsigned char)((K * src[2]) / 255);
					src += 4;
					dst += 3;
				}
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
		jpeg_destroy_decompress(&cinfo);

		if (!canceled && image->getImageWidth() < w) {
			if (process_line < (int)cinfo.output_height) {
				if (!pResizer->horizontalFilter(dib.get(), LINE_BLOCK, dibTmp.get(), process_line, cinfo.output_height - process_line, pCallback)) {
					return false;
				}
			}
			dib = image->getImage(0);
			if (!pResizer->verticalFilter(dibTmp.get(), dib.get(), pCallback))
			{
				return false;
			}
		}


		return !canceled;
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