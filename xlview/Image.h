#ifndef XL_VIEW_IMAGE_H
#define XL_VIEW_IMAGE_H
#include <vector>
#include <memory>
#include <limits>
#include "libxl/include/common.h"
#include <Windows.h>

#ifdef max // <windows.h> defines max & min
#define RESTORE_MIN_MAX
#pragma push_macro ("min")
#pragma push_macro ("max")
#undef max
#undef min
#endif


class CImage
{
protected:
	typedef struct BitmapAndDelay {
		// static const xl::uint INFINITE = std::numeric_limits<xl::uint>::max();
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

	void insertImage (HBITMAP bitmap, xl::uint delay);

// 	HBITMAP getImage (xl::uint index);
// 	HBITMAP getImage (xl::uint index, xl::uint x, xl::uint y);
};



#ifdef RESTORE_MIN_MAX
#pragma pop_macro ("max")
#pragma pop_macro ("min")
#undef RESTORE_MIN_MAX
#endif



#endif
