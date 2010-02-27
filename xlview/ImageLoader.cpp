#include <assert.h>
#include <Windows.h>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageLoader.h"

static const int IMAGE_HEADER_LENGTH = 256;

CImageLoader::CImageLoader () {

}

CImageLoader::~CImageLoader () {

}

CImageLoader* CImageLoader::getInstance () {
	static CImageLoader loader;
	return &loader;
}

void CImageLoader::registerPlugin (ImageLoaderPluginRawPtr plugin) {
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->getPluginName() == plugin->getPluginName()) {
			assert(false);
			return;
		}
	}

	m_plugins.push_back(plugin);
}

bool CImageLoader::isFileSupported (const xl::tstring &fileName) {
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->checkFileName(fileName)) {
			return true;
		}
	}

	return false;
}

CImagePtr CImageLoader::load (const xl::tstring &fileName, xl::ILongTimeRunCallback *pCallback) {
	std::string data;
	if (!file_get_contents(fileName, data)) {
		return CImagePtr();
	}

	size_t header_length = IMAGE_HEADER_LENGTH;
	if (header_length > data.length()) {
		header_length = data.length();
	}
	std::string header = data.substr(0, IMAGE_HEADER_LENGTH);
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->checkFileName(fileName) && (*it)->checkHeader(header)) {
			return (*it)->load(data, pCallback);
		}
	}

	return CImagePtr();
}

CImagePtr CImageLoader::loadThumbnail (
                                       const xl::tstring &fileName,
                                       int tw, 
                                       int th,
                                       int &imageWidth,
                                       int &imageHeight,
                                       xl::ILongTimeRunCallback *pCallback
                                      ) {
	std::string data;
	if (!file_get_contents(fileName, data)) {
		return CImagePtr();
	}

	size_t header_length = IMAGE_HEADER_LENGTH;
	if (header_length > data.length()) {
		header_length = data.length();
	}
	std::string header = data.substr(0, IMAGE_HEADER_LENGTH);
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->checkFileName(fileName) && (*it)->checkHeader(header)) {
			CImagePtr thumbnail = (*it)->loadThumbnail(data, tw, th, imageWidth, imageHeight, pCallback);
			if (thumbnail != NULL) {
				return thumbnail;
			} else {
				CImagePtr image = (*it)->load(data, pCallback);
				if (image) {
					CSize szArea(tw, th);
					CSize szImage = image->getImageSize();
					imageWidth = szImage.cx;
					imageHeight = szImage.cy;
					CSize sz = CImage::getSuitableSize(szArea, szImage, false);
					thumbnail = image->resize(sz.cx, sz.cy, true);
					return thumbnail;
				}
			}
			break;
		}
	}

	return CImagePtr();
}

