#ifndef XL_VIEW_GESTURE_MAP_H
#define XL_VIEW_GESTURE_MAP_H
/**
 * Maintains the mapping relationship of the gesture and the command
 */
#include <vector>

#include "libxl/include/common.h"
#include "libxl/include/string.h"

class CGestureMap {
	struct _MapItem {
		xl::tstring gesture;
		xl::tstring command;
		_MapItem (const xl::tstring &gst, const xl::tstring &cmd) : gesture(gst), command(cmd) 
		{}
	};
	typedef std::vector<_MapItem>                  _MapType;
	_MapType                                       m_map;

	void _LoadMap ();

public:
	typedef _MapType::const_iterator               Iter;
	static const int MAX_LENGTH = 16; // A gesture can't have more than ... characters

	CGestureMap ();
	void reload ();
	void save ();
	xl::tstring getIniPathName () const;
	xl::tstring onGesture (const xl::tstring &gesture);

	Iter begin () const;
	Iter end () const;
	void setGesture (const xl::tstring &command, const xl::tstring &gesture);

	xl::tstring translateGesture (const xl::tstring &gesture);
	xl::tstring translateCommand (const xl::tstring &command);
};

#endif
