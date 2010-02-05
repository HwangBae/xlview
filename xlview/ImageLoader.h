#ifndef XL_VIEW_IMAGE_LOADER_H
#define XL_VIEW_IMAGE_LOADER_H
#include "libxl/include/common.h"
#include "libxl/include/string.h"
#include "Image.h"


//////////////////////////////////////////////////////////////////////////
// interface for canceling image loading
class IImageLoaderCancel {
public:
	virtual bool shouldCancel () = 0;
};



//////////////////////////////////////////////////////////////////////////
// loader for image types
class IImageLoaderPlugin {
public:
	virtual bool checkHeader (const std::string &header) = 0;
	virtual CImagePtr load (const std::string &data, IImageLoaderCancel *pCancel = NULL) = 0;
};


//////////////////////////////////////////////////////////////////////////
// Image loader
class CImageLoader
{
	CImageLoader ();
	~CImageLoader ();
public:
	static CImageLoader* getInstance ();

	CImagePtr load (const xl::tstring &fileName, IImageLoaderCancel *pCancel = NULL);
};


#endif
