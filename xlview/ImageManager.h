#ifndef XL_VIEW_IMAGE_MANAGER_H
#define XL_VIEW_IMAGE_MANAGER_H
#include <vector>
#include "libxl/include/common.h"
#include "libxl/include/string.h"
#include "libxl/include/dp/Observable.h"
#include "Image.h"

class CImageManager : public xl::dp::CObserableT<CImageManager>
{
protected:
// 	typedef std::vector<xl::tstring>               _FileNames;
// 	typedef _FileNames::iterator                   _FileNameIter;
	typedef std::vector<CDisplayImagePtr>          _Images;
	typedef _Images::iterator                      _ImageIter;
	// typedef std::vector
	xl::tstring        m_directory; // include the last '\\'
//	_FileNames         m_fileNames;
	_Images            m_images;
	xl::uint           m_currIndex;

	virtual bool _IsFileSupported (const xl::tstring &file);
	void _AddFile (const xl::tstring &file);

public:
	// event
	enum EVENT 
		: xl::dp::CObserableT<CImageManager>::EVT
	{
		EVT_INDEX_CHANGED,                     // param not used
		EVT_NUM
	};

	CImageManager ();
	virtual ~CImageManager ();

	int getCurrIndex () const;

	void setFile (const xl::tstring &file);
	void setIndex (int index);
};

#endif
