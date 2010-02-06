#ifndef XL_VIEW_IMAGE_H
#define XL_VIEW_IMAGE_H
#include <vector>
#include <memory>
#include <limits>
#include <atltypes.h>
#include "libxl/include/common.h"
#include "libxl/include/string.h"
#include "libxl/include/ui/DIBSection.h"

class CImage;
typedef std::tr1::shared_ptr<CImage>    CImagePtr;

enum {
	THUMBNAIL_WIDTH = 48,
	THUMBNAIL_HEIGHT = 48,
};


//////////////////////////////////////////////////////////////////////////
// CImage

class CImage
{
protected:
	struct Frame {
		enum {
			DELAY_INFINITE = 0
		};
		xl::ui::CDIBSectionPtr bitmap;
		xl::uint delay;

		Frame ();
		~Frame ();
	};

	typedef std::tr1::shared_ptr<Frame>            _FramePtr;
	typedef std::vector<_FramePtr>                 _FrameContainer;

	_FrameContainer                                m_bads;
	int                                            m_width;
	int                                            m_height;

	// void _CreateThumbnail ();

public:
	enum {
		DELAY_INFINITE = Frame::DELAY_INFINITE
	};

	CImage ();
	virtual ~CImage ();

	void operator = (const CImage &);
	CImagePtr clone ();
	void clear ();

	xl::uint getImageCount () const;
	CSize getImageSize () const { return CSize(m_width, m_height); }
	int getImageWidth () const { return m_width; }
	int getImageHeight () const { return m_height; }

	xl::uint getImageDelay (xl::uint index) const;
	xl::ui::CDIBSectionPtr getImage (xl::uint index);

	void insertImage (xl::ui::CDIBSectionPtr bitmap, xl::uint delay);
	CImagePtr resize (int width, int height, bool usehalftone = true);

	static CSize getSuitableSize (CSize szArea, CSize szImage, bool dontEnlarge = true);
};




#endif
