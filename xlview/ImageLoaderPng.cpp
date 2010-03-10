#include <assert.h>
#include <setjmp.h>
#include "../libs/png.h"
#include "libxl/include/utilities.h"
#include "ImageLoader.h"

//////////////////////////////////////////////////////////////////////////
// local functions
class _DataSource {
	const xl::uint8 *m_data;
	xl::uint   m_length;
	xl::uint   m_position;

public:
	_DataSource (const std::string &data)
		: m_data((const xl::uint8 *)data.c_str())
		, m_length(data.length())
		, m_position(0)
	{
	}

	void read (png_structp png_ptr, xl::uint8 *data, xl::uint length) {
		assert(data != NULL);
		xl::uint count = length;
		xl::uint remain = m_length - m_position;
		if (count > remain) {
			// XLTRACE(_T("PNG::IO want read %d but only %d bytes remain\n"), length, remain);
			png_error(png_ptr, "PNG IO failed!");
			count = remain;
		}
		void *dst = data;
		const void *src = m_data + m_position;
		memcpy(dst, src, count);
		m_position += count;
	}
};

static void _read_data (png_structp png_ptr, png_bytep data, png_size_t length) {
	_DataSource *source = (_DataSource *)png_get_io_ptr(png_ptr);
	assert(source != NULL);
	source->read(png_ptr, data, length);
}

static void _error_handle (png_structp /*png_ptr*/, const char *error) {
	throw error;
}

static void _warning_handle (png_structp /*png_ptr*/, const char * /*warning*/) {
	// do nothing
}

//////////////////////////////////////////////////////////////////////////
// CImageLoaderPluginPng

class CImageLoaderPluginPng : public IImageLoaderPlugin
{
	png_structp _CreateStructs (const std::string &data, png_infop *infop, png_infop *endp) {
		assert(infop != NULL && endp != NULL);
		xl::uint bufLength = data.length();
		xl::uint8 *buf = (xl::uint8 *)data.c_str();
		if (bufLength < 8 || png_sig_cmp(buf, 0, 8)) {
			return NULL;
		}

		png_structp psp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, _error_handle, _warning_handle);
		if (!psp) {
			return NULL;
		}

		png_infop info_ptr = png_create_info_struct(psp);
		if (!info_ptr) {
			png_destroy_read_struct(&psp, (png_infopp)NULL, (png_infopp)NULL);
			return NULL;
		}

		png_infop end_info = png_create_info_struct(psp);
		if (!end_info) {
			png_destroy_read_struct(&psp, &info_ptr, (png_infopp)NULL);
			return NULL;
		}

		*infop = info_ptr;
		*endp = end_info;
		return psp;
	}

	void _SetProperty (png_structp psp, png_infop infop) {
		int color_type = png_get_color_type(psp, infop);
		int bit_depth = png_get_bit_depth(psp, infop);

		if (color_type == PNG_COLOR_TYPE_PALETTE) {
			png_set_palette_to_rgb(psp);
		} else if (color_type == PNG_COLOR_TYPE_GRAY &&	bit_depth < 8) {
			png_set_expand_gray_1_2_4_to_8(psp);
		}

		if (png_get_valid(psp, infop, PNG_INFO_tRNS)) {
			png_set_tRNS_to_alpha(psp);
		}

		if (bit_depth == 16) {
			png_set_strip_16(psp);
		}
		if (bit_depth < 8) {
			png_set_packing(psp);
		}

// 		if (color_type == PNG_COLOR_TYPE_RGB) {
// 			png_set_filler(psp, filler, PNG_FILLER_BEFORE);
// 		}

	}
public:
	CImageLoaderPluginPng () {
		XLTRACE(_T("Png decoder created!\n"));
	}

	virtual ~CImageLoaderPluginPng () {
		XLTRACE(_T("Png decoder destroyed!\n"));
	}

	virtual xl::tstring getPluginName () {
		return _T("PNG Loader");
	}

	virtual xl::tstring getFileTypeName () {
		return _T("PNG");
	}

	virtual void registerExt (ImageExts &exts) {
		static xl::tchar *extensions[] = {
			_T("png"),
		};
		exts.reserve(exts.size() + COUNT_OF(extensions));
		for (int i = 0; i < COUNT_OF(extensions); ++ i) {
			exts.push_back(extensions[i]);
		}
	}

	virtual bool readHeader (const std::string &data, ImageHeaderInfo &info) {
		xl::uint width, height;
		int bit_depth, color_type, pixel_depth;
		png_infop infop = NULL, endp = NULL;
		png_structp psp = _CreateStructs(data, &infop, &endp);
		_DataSource ds(data);
		png_set_read_fn(psp, &ds, _read_data);

// 		if (setjmp(png_jmpbuf(psp))) {
// 			png_destroy_read_struct(&psp, &infop, &endp);
// 			return false;
// 		}

		try {
			png_read_info(psp, infop);
			png_get_IHDR(psp, infop, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
			pixel_depth = infop->pixel_depth;
			png_destroy_read_struct(&psp, &infop, &endp);
			info.width = (int)width;
			info.height = (int)height;
			info.frame_count = 1;
			info.bitcount = pixel_depth <= 24 ? 24 : 32;
			return true;
		} catch (...) {
			png_destroy_read_struct(&psp, &infop, &endp);
			return false;
		}
	}

	virtual bool load (CImagePtr image, const std::string &data, xl::ILongTimeRunCallback *pCallback = NULL) {
		assert(image->getImageCount() == 1);
		xl::ui::CDIBSectionPtr dibPtr = image->getImage(0);
		xl::ui::CDIBSection *dib = dibPtr.get();
		dibPtr.reset();

		xl::uint width, height;
		int bit_depth, color_type, pixel_depth, row_bytes;
		png_infop infop = NULL, endp = NULL;
		png_structp psp = _CreateStructs(data, &infop, &endp);
		_DataSource ds(data);
		png_set_read_fn(psp, &ds, _read_data);

		try {
			png_read_info(psp, infop);
			png_get_IHDR(psp, infop, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
			pixel_depth = infop->pixel_depth;
			assert((int)width == dib->getWidth() && (int)height == dib->getHeight());
			row_bytes = png_get_rowbytes(psp, infop);

			_SetProperty(psp, infop);



			png_destroy_read_struct(&psp, &infop, &endp);
		} catch (...) {
			png_destroy_read_struct(&psp, &infop, &endp);
			return false;
		}
	}

	virtual bool loadResize (CImagePtr image, const std::string &data, xl::ui::CResizeEngine *pResizer, xl::ILongTimeRunCallback *pCallback = NULL) {
		return false;
	}

	virtual bool loadThumbnail (
                                    CImagePtr /*image*/,
                                    const std::string &/*data*/,
                                    xl::ILongTimeRunCallback *pCallback = NULL
                                   ) {
		XL_PARAMETER_NOT_USED(pCallback);
		return false;
	}
};


// register
namespace {
	class CPngRegister {
		CImageLoaderPluginPng *m_png;
	public:
		CPngRegister () 
			: m_png(new CImageLoaderPluginPng())
		{
			CImageLoader *pLoader = CImageLoader::getInstance();
			pLoader->registerPlugin(m_png);
		}

		~CPngRegister () {
			delete m_png;
		}
	};

	static CPngRegister pr;
}
