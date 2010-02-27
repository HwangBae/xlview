#ifndef XL_VIEW_IMAGE_LOADER_H
#define XL_VIEW_IMAGE_LOADER_H
#include <vector>
#include <memory>
#include "libxl/include/common.h"
#include "libxl/include/interfaces.h"
#include "libxl/include/string.h"
#include "libxl/include/ui/DIBResizer.h"
#include "Image.h"

//////////////////////////////////////////////////////////////////////////
// header information
struct ImageHeaderInfo {
	int width;
	int height;
	int bitcount;
	int frame_count;

	ImageHeaderInfo () {
		memset(this, 0, sizeof(*this));
	}
};

//////////////////////////////////////////////////////////////////////////
// loader for different image types
class IImageLoaderPlugin {
public:
	virtual xl::tstring getPluginName () = 0;
	virtual bool checkFileName (const xl::tstring &fileName) = 0;
	virtual bool readHeader (const std::string &data, ImageHeaderInfo &info) = 0;
	virtual bool load (CImagePtr image, const std::string &data, xl::ui::CResizeEngine *pResizer = NULL, xl::ILongTimeRunCallback *pCallback = NULL) = 0;
	virtual CImagePtr loadThumbnail (
	                                 const std::string &data,
	                                 int tw,
	                                 int th,
					 int &imageWidth,
					 int &imageHeight,
					 xl::ILongTimeRunCallback *pCallback = NULL
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

	CImagePtr _CreateImageFromHeaderInfo (ImageHeaderInfo &info);
	CImagePtr _CreateSuitableImageFromHeaderInfo (CSize szArea, ImageHeaderInfo &info, bool dontEnlarge = true);

public:
	static CImageLoader* getInstance ();

	void registerPlugin (ImageLoaderPluginRawPtr);
	bool isFileSupported (const xl::tstring &fileName);
	CImagePtr load (const xl::tstring &fileName, xl::ILongTimeRunCallback *pCallback = NULL);
	CImagePtr loadSuitable (const xl::tstring &fileName, CSize *szImage, CSize szArea, xl::ILongTimeRunCallback *pCallback = NULL);
	CImagePtr loadThumbnail (
	                         const xl::tstring &fileName,
	                         int tw,
	                         int th,
	                         int &imageWidth,
	                         int &imageHeight,
	                         xl::ILongTimeRunCallback *pCallback = NULL
	                        );
};


#endif
