#include <assert.h>
#include "../libs/jpeglib.h"
#include "ImageLoader.h"

CImageLoader::CImageLoader () {

}

CImageLoader::~CImageLoader () {

}

CImageLoader* CImageLoader::getInstance () {
	static CImageLoader loader;
	return &loader;
}

CImagePtr CImageLoader::load (const xl::tstring &fileName, IImageLoaderCancel *pCancel) {

}

//////////////////////////////////////////////////////////////////////////
// jpeg loader

class CImageLoaderPluginJpeg
{
public:
	virtual bool checkHeader (const std::string &header) {
		return header.find("JFIF") == 6;
	}

	virtual CImagePtr load (const std::string &data, IImageLoaderCancel *pCancel = NULL) {

	}
};