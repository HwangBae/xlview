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
	assert((szArea.cx >= MIN_ZOOM_WIDTH && szArea.cy >= MIN_ZOOM_HEIGHT) 
		|| (szArea.cx == MIN_ZOOM_WIDTH / 2 && szArea.cy == MIN_ZOOM_HEIGHT / 2));
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
				if ((*it)->load(image, data, pCallback)) {
					return image;
				}
			}
			break;
		}
	}

	return CImagePtr();
}

CImagePtr CImageLoader::loadSuitable (const xl::tstring &fileName, CSize *szImageRS, CSize szArea, xl::ILongTimeRunCallback *pCallback) {
	assert(szImageRS != NULL);
	std::string data;
	if (!file_get_contents(fileName, data)) {
		return CImagePtr();
	}

	ImageHeaderInfo info;
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->readHeader(data, info)) {
			szImageRS->cx = info.width;
			szImageRS->cy = info.height;
			CImagePtr image = _CreateSuitableImageFromHeaderInfo(szArea, info, true);

			if (image != NULL) {
				if (image->getImageSize() == CSize(info.width, info.height)) {
					if ((*it)->load(image, data, pCallback)) {
						return image;
					}
				} else {
					xl::ui::CBoxFilter filter;
					xl::ui::CResizeEngine resizer(&filter);
					if ((*it)->loadResize(image, data, &resizer, pCallback)) {
						return image;
					}
				}
			}
			break;
		}
	}

	return CImagePtr();
}

CImagePtr CImageLoader::loadThumbnail (
                                       const xl::tstring &fileName,
                                       CSize &szImageRS,
                                       CSize szThumbnail,
                                       bool fastOnly,
                                       xl::ILongTimeRunCallback *pCallback
                                      ) {
	std::string data;
	if (!file_get_contents(fileName, data)) {
		return CImagePtr();
	}

	ImageHeaderInfo info;
	for (_Plugins::iterator it = m_plugins.begin(); it != m_plugins.end(); ++ it) {
		if ((*it)->readHeader(data, info)) {
			szImageRS.cx = info.width;
			szImageRS.cy = info.height;
			CImagePtr thumbnail = _CreateSuitableImageFromHeaderInfo(szThumbnail, info, false);
			if ((*it)->loadThumbnail(thumbnail, data, pCallback)) {
				return thumbnail;
			} else if (!fastOnly) {
				// xl::ui::CBoxFilter filter;
				// xl::ui::CResizeEngine resizer(&filter);
				xl::ui::CResizeEngine resizer(NULL);
				if (thumbnail && (*it)->loadResize(thumbnail, data, &resizer, pCallback)) {
					return thumbnail;
				}
			}
			break;
		}
	}

	return CImagePtr();
}

