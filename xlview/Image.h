#ifndef XL_VIEW_IMAGE_H
#define XL_VIEW_IMAGE_H
#include <vector>
#include <memory>
#include <limits>
#include "libxl/include/common.h"
#include "libxl/include/string.h"
#include "libxl/include/ui/DIBSection.h"


class CImage;
typedef std::tr1::shared_ptr<CImage>    CImagePtr;


class CImage
{
protected:
	typedef struct BitmapAndDelay {
		enum {
			DELAY_INFINITE = 0
		};
		xl::ui::CDIBSectionPtr bitmap;
		xl::uint delay;

		BitmapAndDelay ();
		~BitmapAndDelay ();
	} _BAD;

	typedef std::tr1::shared_ptr<_BAD>   _BADPtr;
	typedef std::vector<_BADPtr>         _BADContainer;

	_BADContainer m_bads;
	int m_width;
	int m_height;

public:
	CImage ();
	virtual ~CImage ();

	xl::uint getImageCount ();
	SIZE getImageSize ();
	xl::uint getImageDelay (xl::uint index);

	xl::ui::CDIBSectionPtr getImage (xl::uint index);
// 	HBITMAP getImage (xl::uint index, xl::uint x, xl::uint y);

	void insertImage (xl::ui::CDIBSectionPtr bitmap, xl::uint delay);

	static CImagePtr loadFromFile (const xl::tstring &file);
	static SIZE getSuitableSize (SIZE szArea, SIZE szImage);
};




#endif
