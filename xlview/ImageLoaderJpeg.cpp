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
	bool _ProcessLine (struct jpeg_decompress_struct &cinfo, 
	                   unsigned char *dst, unsigned char *src) {
		int w = cinfo.output_width;
		if (cinfo.out_color_space == JCS_GRAYSCALE) {
			for (int i = 0; i < w; ++ i) {
				unsigned char c = *src ++;
				*dst ++ = c;
				*dst ++ = c;
				*dst ++ = c;
			}
		} else if (cinfo.out_color_space == JCS_RGB) {
			memcpy (dst, src, w * 3);
		} else if (cinfo.out_color_space == JCS_CMYK) { // the process code copied from FreeImage
			assert(cinfo.out_color_components == 4);
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
			return false;
		}

		return true;
	}

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

	virtual bool load (CImagePtr image, const std::string &data, xl::ILongTimeRunCallback *pCallback = NULL) {
		const int LINE_BLOCK = 32;
		struct jpeg_decompress_struct cinfo;
		safe_jpeg_error_mgr em;
		JSAMPARRAY buffer;
		int decoded_line_count = 0;

		assert(image->getImageCount() == 1);
		xl::ui::CDIBSectionPtr dibPtr = image->getImage(0);
		xl::ui::CDIBSection *dib = dibPtr.get();
		dibPtr.reset();

		cinfo.err = jpeg_std_error(&em.pub);
		em.pub.error_exit = safe_jpeg_error_exit;
		if (setjmp(em.setjmp_buffer)) {
			jpeg_destroy_decompress(&cinfo);
			return decoded_line_count > 0;
		}

		jpeg_create_decompress(&cinfo);
		jpeg_mem_src(&cinfo, (unsigned char *)data.c_str(), data.length());
		if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
			(void) jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			return false;
		}

		// cinfo.dct_method = JDCT_ISLOW;
		// cinfo.scale_num = 1;
		int w = cinfo.image_width;
		int h = cinfo.image_height;
		assert(w == image->getImageWidth() && h == image->getImageHeight());

		(void) jpeg_start_decompress(&cinfo);
		int src_row_stride = cinfo.output_width * cinfo.output_components;
		int dst_row_stride = dib->getStride();
		buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, src_row_stride, 1);

		bool canceled = false;
		unsigned char *dst_data = (unsigned char *)dib->getData();
		unsigned char *src_data = buffer[0];
		while (cinfo.output_scanline < cinfo.output_height) {
			if (decoded_line_count % LINE_BLOCK == 1) {
				if (pCallback && pCallback->shouldStop()) {
					canceled = true;
					break;
				}
			}

			jpeg_read_scanlines(&cinfo, buffer, 1);
			if (!_ProcessLine(cinfo, dst_data, src_data)) {
				canceled = true;
				break;
			}
			dst_data += dst_row_stride;
			++ decoded_line_count;
		}

		if (canceled) {
			jpeg_abort_decompress(&cinfo);
		} else {
			jpeg_finish_decompress(&cinfo);
		}
		jpeg_destroy_decompress(&cinfo);

		return !canceled;
	}


	virtual bool loadResize (CImagePtr image, const std::string &data, xl::ui::CResizeEngine *pResizer, xl::ILongTimeRunCallback *pCallback = NULL) {
		assert(pResizer != NULL);
		const int LINE_BLOCK = 32;
		struct jpeg_decompress_struct cinfo;
		safe_jpeg_error_mgr em;
		JSAMPARRAY buffer;
		int decoded_line_count = 0;
		int zoomed_line_count = 0;

		assert(image->getImageCount() == 1);
		xl::ui::CDIBSectionPtr dib;
		xl::ui::CDIBSectionPtr dibTmp;

		cinfo.err = jpeg_std_error(&em.pub);
		em.pub.error_exit = safe_jpeg_error_exit;
		if (setjmp(em.setjmp_buffer)) {
			jpeg_destroy_decompress(&cinfo);
			if (decoded_line_count > 0) { // try to get the partial image
				if (decoded_line_count > zoomed_line_count) {
					if (!pResizer->horizontalFilter(dib.get(), LINE_BLOCK, dibTmp.get(),
						zoomed_line_count, decoded_line_count - zoomed_line_count, 
						pCallback))
					{
						return false;
					}
				}
				dib = image->getImage(0);
				if (pResizer->verticalFilter(dibTmp.get(), dib.get(), pCallback)) {
					return true;
				}

			}
			return decoded_line_count > 0;
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
		assert((w != image->getImageWidth() || h != image->getImageHeight()));
		dib = xl::ui::CDIBSection::createDIBSection(w, LINE_BLOCK, 24);
		dibTmp = xl::ui::CDIBSection::createDIBSection(image->getImageWidth(), h, 24);
		if (dib == NULL || dibTmp == NULL) {
			return false; // out of memory
		}

		bool canceled = false;
		(void) jpeg_start_decompress(&cinfo);
		int src_row_stride = cinfo.output_width * cinfo.output_components;
		int dst_row_stride = dib->getStride();
		buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, src_row_stride, 1);

		unsigned char *dst_data = (unsigned char *)dib->getData();
		unsigned char *src_data = buffer[0];
		while (cinfo.output_scanline < cinfo.output_height) {
			if (decoded_line_count % LINE_BLOCK == 0 && decoded_line_count > 0) {
				if (pCallback && pCallback->shouldStop()) {
					canceled = true;
					break;
				}

				if (!pResizer->horizontalFilter(dib.get(), LINE_BLOCK, dibTmp.get(), zoomed_line_count, LINE_BLOCK, pCallback)) {
					canceled = true;
					break;
				}
				dst_data = (unsigned char *)dib->getData();
				zoomed_line_count += LINE_BLOCK;
			}
			jpeg_read_scanlines(&cinfo, buffer, 1);
			if (!_ProcessLine(cinfo, dst_data, src_data)) {
				canceled = true;
				break;
			}
			dst_data += dst_row_stride;
			++ decoded_line_count;
		}

		if (canceled) {
			jpeg_abort_decompress(&cinfo);
		} else {
			jpeg_finish_decompress(&cinfo);
		}
		jpeg_destroy_decompress(&cinfo);

		if (!canceled) {
			if (zoomed_line_count < (int)cinfo.output_height) {
				if (!pResizer->horizontalFilter(dib.get(), LINE_BLOCK, dibTmp.get(), zoomed_line_count, cinfo.output_height - zoomed_line_count, pCallback)) {
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

	virtual bool loadThumbnail (CImagePtr image, const std::string &data, xl::ILongTimeRunCallback *pCallback) {
		assert(image != NULL);
		assert(image->getImageCount() == 1);

		const int LINE_BLOCK = 32;
		struct jpeg_decompress_struct cinfo;
		safe_jpeg_error_mgr em;
		JSAMPARRAY buffer;
		int decoded_line_count = 0;
		xl::ui::CDIBSectionPtr dib;
		double ratio;
		int dst_width = image->getImageWidth();
		int dst_height = image->getImageHeight();

		cinfo.err = jpeg_std_error(&em.pub);
		em.pub.error_exit = safe_jpeg_error_exit;
		if (setjmp(em.setjmp_buffer)) {
			jpeg_destroy_decompress(&cinfo);
			return decoded_line_count > 0;
		}

		jpeg_create_decompress(&cinfo);
		jpeg_mem_src(&cinfo, (unsigned char *)data.c_str(), data.length());
		if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
			(void) jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			return false;
		}

		// cinfo.dct_method = JDCT_ISLOW;
		// cinfo.scale_num = 1;
		int w = cinfo.image_width;
		int h = cinfo.image_height;
		assert(w > 0 && h > 0);
		ratio = (double)dst_width / (double)w;
		xl::uint scale_num = (xl::uint)(8 * ratio + 0.5);
		if (scale_num < 1) {
			scale_num = 1;
		}
		cinfo.scale_num = scale_num;

		(void) jpeg_start_decompress(&cinfo);
		dib = xl::ui::CDIBSection::createDIBSection(cinfo.output_width, cinfo.output_height, 24, false);
		if (dib == NULL) {
			jpeg_abort_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			return false;
		}
		int src_row_stride = cinfo.output_width * cinfo.output_components;
		int dst_row_stride = dib->getStride();

		buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, src_row_stride, 1);
		bool canceled = false;
		unsigned char *dst_data = (unsigned char *)dib->getData();
		unsigned char *src_data = buffer[0];
		while (cinfo.output_scanline < cinfo.output_height) {
			if (decoded_line_count % LINE_BLOCK == 1) {
				if (pCallback && pCallback->shouldStop()) {
					canceled = true;
					break;
				}
			}

			jpeg_read_scanlines(&cinfo, buffer, 1);
			if (!_ProcessLine(cinfo, dst_data, src_data)) {
				canceled = true;
				break;
			}
			dst_data += dst_row_stride;
			++ decoded_line_count;
		}

		if (canceled) {
			jpeg_abort_decompress(&cinfo);
		} else {
			jpeg_finish_decompress(&cinfo);
		}
		jpeg_destroy_decompress(&cinfo);

		if (!canceled) {
			CSize szSrc(dib->getWidth(), dib->getHeight());
			xl::ui::CDIBSectionPtr dst = image->getImage(0);
			CSize szDst(dst->getWidth(), dst->getHeight());
			if (szSrc != szDst) {
				if (dib->resize(dst.get(), xl::ui::CDIBSection::RT_BOX, pCallback)) {
					return true;
				} else {
					return false;
				}
			} else {
				int width = szSrc.cx;
				int height = szSrc.cy;
				int stride = dib->getStride();
				assert(stride == dst->getStride());
				for (int y = 0; y < height; ++ y) {
					xl::uint *src_line = (xl::uint *)dib->getLine(y);
					xl::uint *dst_line = (xl::uint *)dst->getLine(y);
					memcpy(dst_line, src_line, stride);
				}
				return true;
			}
		}

		return false;
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