#include <algorithm>
#include "libxl/include/Language.h"
#include "GestureMap.h"

/**
 * The whole command list please refer to the "map" section of Dispatch.cpp
 */
void CGestureMap::_LoadMap () {
	m_map.clear();
	m_map.reserve(20);

	m_map.push_back(_MapItem(_T("DR"), _T("Exit")));
	m_map.push_back(_MapItem(_T("L"), _T("showPrev")));
	m_map.push_back(_MapItem(_T("R"), _T("showNext")));
	m_map.push_back(_MapItem(_T("DL"), _T("Minimize")));
	m_map.push_back(_MapItem(_T("RU"), _T("MaximizeOrRestore")));
	m_map.push_back(_MapItem(_T("LD"), _T("showSuitable")));
	m_map.push_back(_MapItem(_T("UR"), _T("showRealSize")));
	m_map.push_back(_MapItem(_T("RDR"), _T("showSwitch")));
	m_map.push_back(_MapItem(_T("U"), _T("showLarger")));
	m_map.push_back(_MapItem(_T("D"), _T("showSmaller")));
	m_map.push_back(_MapItem(_T("URL"), _T("showTop")));
	m_map.push_back(_MapItem(_T("DRL"), _T("showBottom")));
	m_map.push_back(_MapItem(_T("LUD"), _T("showLeft")));
	m_map.push_back(_MapItem(_T("RUD"), _T("showRight")));
}

CGestureMap::CGestureMap () {
	_LoadMap ();
}

void CGestureMap::reload () {
	_LoadMap ();
}


/**
 * Return the command (in string)
 */
xl::tstring CGestureMap::onGesture (const xl::tstring &gesture) {
	xl::tstring command = _T("unknown");
	const xl::tchar *gst = gesture.c_str();
	auto it = std::find_if(m_map.begin(), m_map.end(), 
		[=, &command] (_MapItem &item) {
			return _tcsicmp(gst, item.gesture.c_str()) == 0;
		});
	if (it != m_map.end()) {
		command = it->command;
	}

	return command;
}
