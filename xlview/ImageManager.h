#ifndef XL_VIEW_IMAGE_MANAGER_H
#define XL_VIEW_IMAGE_MANAGER_H
#include <vector>
#include <atltypes.h>
#include "libxl/include/common.h"
#include "libxl/include/string.h"
#include "libxl/include/utilities.h"
#include "libxl/include/dp/Observable.h"
#include "ImageConfig.h"
#include "CachedImage.h"
#include "ImageLoader.h"


class CImageManager 
	: public xl::dp::CObserableT<CImageManager>
	, public IImageOperateCancel
	, public xl::CUserLock
{
protected:
	enum DIRECTION {
		FORWARD,
		BACKWARD
	};
	typedef std::vector<xl::uint>                  _Indexes;
	typedef std::vector<CCachedImagePtr>           _CachedImages;
	typedef _CachedImages::iterator                _CachedImageIter;
	xl::tstring        m_directory; // include the last '\\'
	_CachedImages      m_cachedImages;
	xl::uint           m_currIndex;
	DIRECTION          m_direction;

	CSize              m_szPrefetch;

	void _SetIndexNoLock (int index); // called when already locked

	// static
	static void _GetPrefetchIndexes (_Indexes &indexes, int currIndex, int count, DIRECTION direction, int range);

	//////////////////////////////////////////////////////////////////////////
	// thread related
	bool               m_exiting;
	bool               m_indexChanged;
	HANDLE             m_hPrefetchThread;
	HANDLE             m_hPrefetchEvent;
	static unsigned __stdcall _PrefetchThread (void *);
	void _CreateThreads ();
	void _TerminateThreads ();
	void _BeginPrefetch ();

public:
	// event
	enum EVENT 
		: xl::dp::CObserableT<CImageManager>::EVT
	{
		EVT_FILELIST_READY,                    // param (pointer to total count)
		EVT_INDEX_CHANGED,                     // param (pointer to the current index)
		EVT_IMAGE_LOADED,                      // param (pointer to the loaded index)
		EVT_IMAGE_ZOOMED,                      // param (pointer to the zoomed index)
		EVT_NUM
	};

	CImageManager ();
	virtual ~CImageManager ();

	int getCurrIndex () const;
	int getImageCount () const;

	bool setFile (const xl::tstring &file);
	void setIndex (int index);

	CDisplayImagePtr getImage (int index);

	//////////////////////////////////////////////////////////////////////////
	// To be notified
	void onViewSizeChanged (CRect rc); // called by the view to notify its size changed

	// IImageOperateCancel
	virtual bool shouldCancel ();
};

#endif
