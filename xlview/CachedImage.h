#ifndef XL_VIEW_CACHED_IMAGE_H
#define XL_VIEW_CACHED_IMAGE_H
#include <memory>
#include "libxl/include/lockable.h"
#include "Image.h"
#include "ImageLoader.h"

class CCachedImage;
typedef std::tr1::shared_ptr<CCachedImage>             CCachedImagePtr;

class CCachedImage
	: public xl::CUserLock
{
	xl::tstring        m_fileName;
	CSize              m_imageSize;
	CImagePtr          m_suitableImage;
	CImagePtr          m_thumbnailImage;

public:
	CCachedImage (const xl::tstring &fileName);
	virtual ~CCachedImage ();

	bool load (CSize szView, IImageOperateCallback *pCancel = NULL);
	bool loadThumbnail (IImageOperateCallback *pCancel = NULL);
	void clear (bool clearThumbnail = false);

	xl::tstring getFileName () const;
	CSize getImageSize () const;
	CImagePtr getCachedImage () const;
};


#endif
