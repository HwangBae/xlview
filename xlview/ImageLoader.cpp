#include <assert.h>
#include <Windows.h>
#include "libxl/include/fs.h"
#include "libxl/include/utilities.h"
#include "ImageConfig.h"
#include "ImageLoader.h"


CImageLoader::CImageLoader () {

}

CImageLoader::~CImageLoader () {

}

CImagePtr CImageLoader::_CreateImageFromHeaderInfo (ImageHeaderInfo &info) {
	assert(info.width > 0 && info.height > 0 && info.bitcount > 16 && info.frame_count > 0);

	int w = info.width;
	int h = info.height;
	int bitcount = info.bitcount;

	CImagePtr image(new CImage());
	if (image != NULL) {
		for (int i = 0; i < info.frame_count; ++ i) {
			xl::ui::CDIBSectionPtr dib = 
				xl::ui::CDIBSection::createDIBSection(w, h, bitcount);
			if (dib == NULL) {
				return CImagePtr(); // TODO: out of memory
			}

			// the "delay" is left for the loader to modify
			image->insertImage(dib, CImage::DELAY_INFINITE);
		}

		return image;
	}

	return CImagePtr();
}

CImagePtr CImageLoader::_CreateSuitableImageFromHeaderInfo (CSize szArea, ImageHeaderInfo &info, bool dontEnlarge) {
	assert(szArea.cx >= MIN_ZOOM_WIDTH && szArea.cy >= MIN_ZOOM_HEIGHT);
	assert(info.width > 0 && info.height > 0 && info.bitcount > 16 && info.frame_count > 0);

	int w = info.width;
	int h = info.height;
	int bitcount = info.bitcount;
	CSize szImage(w, h);
	CSize szSuitable = CImage::getSuitableSize(szArea, szImage, dontEnlarge);
	w = szSuitable.cx;
	h = szSuitable.cy;

	CImagePtr image(new CImage());
	if (image != NULL) {
		for (int i = 0; i < info.frame_count; ++ i) {
			xl::ui::CDIBSectionPtr dib = 
				xl::ui::CDIBSection::createDIBSection(w, h, bitcount);
			if (dib == NULL) {
				return CImagePtr(); // TODO: out of memory
			}

			// the "delay" is left for the loader to modify
			image->insertImage(dib, CImage::DELAY_INFINITE);
		}

		return image;
	}

	return CImagePtr();
}


//////////////////////////////////////////////////////////////////////////
// static 
CImageLoader* CImageLoader::getInstance () {
	static CImageLoader loader;
	return &loader;
}

//////////////////////////////////////////////////////////////////////////
// public methods
void CImageLoader::registerPlugin (ImageLoaderPluginRawPtr plugin) {
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->getPluginName() == plugin->getPluginName()) {
			assert(false);
			return;
		}
	}

	m_plugins.push_back(plugin);
	plugin->registerExt(m_exts);
}

bool CImageLoader::isFileSupported (const xl::tstring &fileName) {
	size_t offset = fileName.rfind(_T("."));
	if (offset == fileName.npos) { // no extension
		return false;
	}

	const xl::tchar *ext = &fileName.c_str()[offset + 1];
	for (size_t i = 0; i < m_exts.size(); ++ i) {
		if (_tcsicmp(ext, m_exts[i]) == 0) {
			return true;
		}
	}

	return false;

	return false;
}

CImagePtr CImageLoader::load (const xl::tstring &fileName, xl::ILongTimeRunCallback *pCallback) {
	std::string data;
	if (!file_get_contents(fileName, data)) {
		return CImagePtr();
	}

	ImageHeaderInfo info;
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->readHeader(data, info)) {
			CImagePtr image = _CreateImageFromHeaderInfo(info);

			if (image != NULL) {
				if ((*it)->load(image, data, NULL, pCallback)) {
					return image;
				}
			}
			break;
		}
	}

	return CImagePtr();
}

CImagePtr CImageLoader::loadSuitable (const xl::tstring &fileName, CSize *szImage, CSize szArea, xl::ILongTimeRunCallback *pCallback) {
	assert(szImage != NULL);
	std::string data;
	if (!file_get_contents(fileName, data)) {
		return CImagePtr();
	}

	ImageHeaderInfo info;
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->readHeader(data, info)) {
			CImagePtr image = _CreateSuitableImageFromHeaderInfo(szArea, info, true);

			if (image != NULL) {
				xl::ui::CBoxFilter filter;
				xl::ui::CResizeEngine resizer(&filter);
				if ((*it)->load(image, data, &resizer, pCallback)) {
				// xl::ui::CResizeEngine r(NULL);
				// if ((*it)->load(image, data, &r, pCallback)) {
					szImage->cx = info.width;
					szImage->cy = info.height;
					return image;
				}
			}
			break;
		}
	}

	return CImagePtr();
}

CImagePtr CImageLoader::loadThumbnail (
                                       const xl::tstring &fileName,
                                       CSize szThumbnail,
                                       int &imageWidth,
                                       int &imageHeight,
                                       xl::ILongTimeRunCallback *pCallback
                                      ) {
	std::string data;
	if (!file_get_contents(fileName, data)) {
		return CImagePtr();
	}

	ImageHeaderInfo info;
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->readHeader(data, info)) {
			imageWidth = info.width;
			imageHeight = info.height;
			CImagePtr thumbnail = (*it)->loadThumbnail(data, szThumbnail, imageWidth, imageHeight, pCallback);
			if (thumbnail != NULL) {
				return thumbnail;
			} else {
				CImagePtr thumbnail = _CreateSuitableImageFromHeaderInfo(szThumbnail, info, false);
				xl::ui::CBoxFilter filter;
				xl::ui::CResizeEngine resizer(&filter);
				if (thumbnail && (*it)->load(thumbnail, data, &resizer, pCallback)) {
					return thumbnail;
				}
			}
			break;
		}
	}

	return CImagePtr();
}

