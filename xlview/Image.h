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

	typedef std::tr1::shared_ptr<_BAD>   _BADPtr;
	typedef std::vector<_BADPtr>         _BADContainer;

	_BADContainer            m_bads;
	xl::ui::CDIBSectionPtr   m_thumbnail;
	int                      m_width;
	int                      m_height;

	void _CreateThumbnail ();

public:
	enum {
		THUMBNAIL_WIDTH = 48,
		THUMBNAIL_HEIGHT = 48,
	};

	CImage ();
	virtual ~CImage ();

	void clear ();

	xl::uint getImageCount () const;
	SIZE getImageSize () const;
	SIZE getThumbnailSize () const;

	xl::uint getImageDelay (xl::uint index) const;
	xl::ui::CDIBSectionPtr getImage (xl::uint index);
	xl::ui::CDIBSectionPtr getThumbnail ();

	void insertImage (xl::ui::CDIBSectionPtr bitmap, xl::uint delay);

	static CImagePtr loadFromFile (const xl::tstring &file);
	static SIZE getSuitableSize (SIZE szArea, SIZE szImage);
};


//////////////////////////////////////////////////////////////////////////
// CDisplayImage

/**
 * Contains three type of images:
 * 1. thumbnail
 * 2. resized image
 * 3. original image
 *
 * When display:
 *
 * 1. display thumbnail
 *  1.1 m_thumbnail exists, display it
 *  1.2 m_thumbnail doesn't exist, display blank
 * 2. display image
 *
 */

class CDisplayImage : public CImage
{
	static bool m_initialized;
	static HANDLE m_semaphore;
	static int m_thread_command;
	static HANDLE m_hThread;

	xl::tstring m_file;
	xl::ui::CDIBSectionPtr        m_thumbnail;

	//////////////////////////////////////////////////////////////////////////
	// private methods
	static unsigned int __stdcall _Thread (void *);
	void _Initilize ();

public:
	CDisplayImage ();
	~CDisplayImage ();
	void drawThumbnail ();
};



#endif
