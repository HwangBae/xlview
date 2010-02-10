#ifndef XL_VIEW_IMAGE_LOADER_H
#define XL_VIEW_IMAGE_LOADER_H
#include <vector>
#include <memory>
#include "libxl/include/common.h"
#include "libxl/include/string.h"
#include "Image.h"


//////////////////////////////////////////////////////////////////////////
// interface for canceling image operation
class IImageOperateCancel {
public:
	virtual bool shouldCancel () = 0;
};



//////////////////////////////////////////////////////////////////////////
// loader for image types
class IImageLoaderPlugin {
public:
	virtual xl::tstring getPluginName () = 0;
	virtual bool checkFileName (const xl::tstring &fileName) = 0;
	virtual bool checkHeader (const std::string &header) = 0;
	virtual CImagePtr load (const std::string &data, IImageOperateCancel *pCancel = NULL) = 0;
	virtual CImagePtr loadThumbnail (
	                                 const std::string &data,
	                                 int tw,
	                                 int th,
					 int &imageWidth,
					 int &imageHeight,
	                                 IImageOperateCancel *pCancel = NULL
	                                ) {
		return CImagePtr();
	}
};
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
	bool isFileSupported (const xl::tstring &fileName);
	CImagePtr load (const xl::tstring &fileName, IImageOperateCancel *pCancel = NULL);
	CImagePtr loadThumbnail (
	                         const xl::tstring &fileName,
	                         int tw,
	                         int th,
	                         int &imageWidth,
	                         int &imageHeight,
	                         IImageOperateCancel *pCancel = NULL
	                        );
};


#endif
