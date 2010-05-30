#ifndef XL_VIEW_GESTURE_MAP_H
#define XL_VIEW_GESTURE_MAP_H
/**
 * Maintains the mapping relationship of the gesture and the action
 */
#include <vector>

#include "libxl/include/common.h"
#include "libxl/include/string.h"

class CGestureMap {
	struct _MapItem {
		xl::tstring gesture;
		xl::tstring action;
		_MapItem (const xl::tstring &gst, const xl::tstring &act) : gesture(gst), action(act) 
		{}
	};
	typedef std::vector<_MapItem>                  _MapType;
	_MapType                                       m_map;

	void _LoadMap ();

public:
	CGestureMap ();
	void reload ();
	xl::tstring onGesture (const xl::tstring &gesture);
};

#endif
