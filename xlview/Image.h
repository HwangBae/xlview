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

//////////////////////////////////////////////////////////////////////////
// CImage

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

	typedef std::tr1::shared_ptr<_BAD>             _BADPtr;
	typedef std::vector<_BADPtr>                   _BADContainer;

	_BADContainer                                  m_bads;
	xl::ui::CDIBSectionPtr                         m_thumbnail;
	int                                            m_width;
	int                                            m_height;

	void _CreateThumbnail ();

public:
	enum {
		THUMBNAIL_WIDTH = 48,
		THUMBNAIL_HEIGHT = 48,
	};

	CImage ();
	virtual ~CImage ();

	void operator = (const CImage &);
	void clear ();
	bool load (const xl::tstring &file);

	xl::uint getImageCount () const;
	SIZE getImageSize () const;
	SIZE getThumbnailSize () const;

	xl::uint getImageDelay (xl::uint index) const;
	xl::ui::CDIBSectionPtr getImage (xl::uint index);
	xl::ui::CDIBSectionPtr getThumbnail ();

	void insertImage (xl::ui::CDIBSectionPtr bitmap, xl::uint delay);

	static SIZE getSuitableSize (SIZE szArea, SIZE szImage);
};


//////////////////////////////////////////////////////////////////////////
// CDisplayImage
class CDisplayImage;
typedef std::tr1::shared_ptr<CDisplayImage>            CDisplayImagePtr;

class CDisplayImage : public CImage
{
	xl::tstring m_fileName;

public:
	CDisplayImage (const xl::tstring &fileName);
	virtual ~CDisplayImage ();
	CDisplayImagePtr clone ();

	bool load ();
	xl::tstring getFileName () const;
};



#endif
