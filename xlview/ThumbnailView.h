#ifndef XL_VIEW_THUMBNAILVIEW_H
#define XL_VIEW_THUMBNAILVIEW_H
#include <vector>
#include <memory>
#include "libxl/include/lockable.h"
#include "libxl/include/ui/Control.h"
#include "ImageManager.h"
#include "MultiLock.h"

class CThumbnailView
	: CMultiLock
	, public xl::ui::CControl
	, CImageManager::IObserver
{
	//////////////////////////////////////////////////////////////////////////
	// inner class
	class _CThumbnail {
		int        m_index;
		CRect      m_rect;
		CImagePtr  m_thumbnail;
	public:
		_CThumbnail (int index, CRect rc, CImagePtr thumbnail);
		~_CThumbnail ();
		int getIndex () const;
		CRect getRect () const;
		bool hasThumbnail () const;
		void setThumbnail (CImagePtr);

		void draw (HDC hdc, int currIndex, int hoverIndex);
	};
	typedef std::tr1::shared_ptr<_CThumbnail>      _CThumbnailPtr;
	typedef std::vector<_CThumbnailPtr>            _Thumbnails;

	CImageManager     *m_pImageManager;
	int                m_currIndex;
	int                m_hoverIndex;
	_Thumbnails        m_thumbnails;

	void _CreateThumbnailList ();
	void _OnThumbnailLoaded (int);

public:
	CThumbnailView(CImageManager *pImageManager);
	virtual ~CThumbnailView(void);

	//////////////////////////////////////////////////////////////////////////
	// virtual

	// xl::ui::CControl
	virtual void drawMe (HDC hdc);
	virtual void onSize ();
	virtual void onLButtonDown (CPoint, xl::uint);
	virtual void onMouseMove (CPoint, xl::uint);
	virtual void onMouseOut (CPoint);

	// CImageManager::IObserver
	virtual void onEvent (EVT evt, void *param);
};

#endif
