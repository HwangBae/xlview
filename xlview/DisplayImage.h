#ifndef XL_VIEW_DISPLAY_IMAGE_H
#define XL_VIEW_DISPLAY_IMAGE_H
#include <memory>
#include "libxl/include/lockable.h"
#include "Image.h"
#include "ImageLoader.h"


class CDisplayImage;
typedef std::tr1::shared_ptr<CDisplayImage>            CDisplayImagePtr;

class CDisplayImage 
	: public xl::CUserLock
{
	bool               m_zooming;
	xl::tstring        m_fileName;

	int                m_widthReal;
	int                m_heightReal;

	CImagePtr          m_imgThumbnail;
	CImagePtr          m_imgZoomed;
	CImagePtr          m_imgZooming;
	CImagePtr          m_imgRealSize;

public:
	CDisplayImage (const xl::tstring &fileName);
	virtual ~CDisplayImage ();
	CDisplayImagePtr clone ();

	bool loadZoomed (int width, int height, IImageLoaderCancel *pCancel = NULL);
	bool loadRealSize (IImageLoaderCancel *pCancel = NULL);
	bool loadThumbnail (IImageLoaderCancel *pCancel = NULL);

	void clearThumbnail ();
	void clearRealSize ();
	void clearZoomed ();
	void clear ();

	xl::tstring getFileName () const;
	int getRealWidth () const;
	int getRealHeight () const;
	CSize getRealSize () const;

	CImagePtr getThumbnail ();
	CImagePtr getZoomedImage ();
	CImagePtr getRealSizeImage ();
};


#endif
