#ifndef XL_VIEW_IMAGE_MANAGER_H
#define XL_VIEW_IMAGE_MANAGER_H
#include <vector>
#include <atltypes.h>
#include "libxl/include/common.h"
#include "libxl/include/string.h"
#include "libxl/include/dp/Observable.h"
#include "Image.h"


class CImageManager 
	: public xl::dp::CObserableT<CImageManager>
	, public CImage::ICancel
{
protected:
	typedef std::vector<CDisplayImagePtr>          _Images;
	typedef _Images::iterator                      _ImageIter;
	xl::tstring        m_directory; // include the last '\\'
	_Images            m_images;
	xl::uint           m_currIndex;

	CRect              m_rcView;

	virtual bool _IsFileSupported (const xl::tstring &file);
	void _AddFile (const xl::tstring &file);

	// threads
	bool               m_exit;
	HANDLE             m_semaphoreWorking;
	HANDLE             m_threadWorking;
	static unsigned int __stdcall _WorkingThread (void *);

	void _CreateThreads ();
	void _TerminateThreads ();

	void _BeginPrefetch ();


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
	int getImageCount () const;

	void setFile (const xl::tstring &file);
	void setIndex (int index);

	CDisplayImagePtr getImage (int index);

	//////////////////////////////////////////////////////////////////////////
	// To be notified
	void onViewSizeChanged (CRect rc); // called by the view to notify its size changed

	// CImage::ICancel
	virtual bool cancelLoading ();
};

#endif
