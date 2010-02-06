#ifndef XL_VIEW_IMAGE_LOADER_H
#define XL_VIEW_IMAGE_LOADER_H
#include <vector>
#include <memory>
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
	virtual xl::tstring getName () = 0;
	virtual bool checkHeader (const std::string &header) = 0;
	virtual CImagePtr load (const std::string &data, IImageLoaderCancel *pCancel = NULL) = 0;
};
// typedef std::tr1::shared_ptr<IImageLoaderPlugin>       ImageLoaderPluginRawPtr;
typedef IImageLoaderPlugin                            *ImageLoaderPluginRawPtr;


//////////////////////////////////////////////////////////////////////////
// Image loader
class CImageLoader
{
	typedef std::vector<ImageLoaderPluginRawPtr>   _Plugins;
	_Plugins           m_plugins;

	CImageLoader ();
	~CImageLoader ();
public:
	static CImageLoader* getInstance ();

	void registerPlugin (ImageLoaderPluginRawPtr);
	CImagePtr load (const xl::tstring &fileName, IImageLoaderCancel *pCancel = NULL);
};


#endif
