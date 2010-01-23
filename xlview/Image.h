#ifndef XL_VIEW_IMAGE_H
#define XL_VIEW_IMAGE_H
#include <vector>
#include <memory>
#include <limits>
#include "libxl/include/common.h"
#include "libxl/include/string.h"
#include <Windows.h>

#ifdef max // <windows.h> defines max & min
#define RESTORE_MIN_MAX
#pragma push_macro ("min")
#pragma push_macro ("max")
#undef max
#undef min
#endif

class CImage;
typedef std::tr1::shared_ptr<CImage>    CImagePtr;


class CImage
{
protected:
	typedef struct BitmapAndDelay {
		enum {
			DELAY_INFINITE = 0
		};
		HBITMAP bitmap;
		xl::uint delay;

		BitmapAndDelay ();
		~BitmapAndDelay ();
	} _BAD;

	typedef std::tr1::shared_ptr<_BAD>   _BADPtr;
	typedef std::vector<_BADPtr>         _BADContainer;

	_BADContainer m_bads;

public:
	CImage ();
	virtual ~CImage ();

	xl::uint getImageCount ();
	xl::uint getImageDelay (xl::uint index);

 	HBITMAP getImage (xl::uint index);
// 	HBITMAP getImage (xl::uint index, xl::uint x, xl::uint y);

	void insertImage (HBITMAP bitmap, xl::uint delay);

	static CImagePtr loadFromFile (const xl::tstring &file);
	static SIZE getProperSize (SIZE szArea, SIZE szImage);
};



#ifdef RESTORE_MIN_MAX
#pragma pop_macro ("max")
#pragma pop_macro ("min")
#undef RESTORE_MIN_MAX
#endif



#endif
