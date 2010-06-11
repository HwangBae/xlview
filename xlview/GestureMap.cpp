#include <assert.h>
#include <algorithm>
#include "libxl/include/Language.h"
#include "libxl/include/ini.h"
#include "GestureMap.h"

/**
 * The whole command list please refer to the "map" section of Dispatch.cpp
 */
void CGestureMap::_LoadMap () {
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
	m_map.push_back(_MapItem(_T("ULR"), _T("showTop")));
	m_map.push_back(_MapItem(_T("DRL"), _T("showBottom")));
	m_map.push_back(_MapItem(_T("LDU"), _T("showLeft")));
	m_map.push_back(_MapItem(_T("RUD"), _T("showRight")));

	xl::CIni ini(getIniPathName());
	for (auto it = ini.begin(_T("")); it != ini.end(_T("")); ++ it) {
		xl::tstring command = it->first;
		xl::tstring gesture = it->second;

		setGesture(command, gesture);
	}
}

CGestureMap::CGestureMap () {
	_LoadMap ();
}

void CGestureMap::reload () {
	m_map.clear();
	_LoadMap ();
}

void CGestureMap::save () {
	// TODO:
}

xl::tstring CGestureMap::getIniPathName () const {
	return _T("gesture.ini");
}

CGestureMap::Iter CGestureMap::begin () const {
	return m_map.begin();
}

CGestureMap::Iter CGestureMap::end () const {
	return m_map.end();
}

void CGestureMap::setGesture (const xl::tstring &command, const xl::tstring &gesture) {
	auto it = std::find_if(m_map.begin(), m_map.end(),
		[&command](_MapItem &item) {
			return _tcscmp(command.c_str(), item.command) == 0;
		}
	);
	assert(it != m_map.end());
	if (it != m_map.end()) {
		it->gesture = gesture;
	}
}

xl::tstring CGestureMap::translateGesture (const xl::tstring &gesture) {
	xl::CLanguage *pLanguage = xl::CLanguage::getInstance();
	xl::tstring text;
	size_t len = gesture.length();
	xl::tchar c[2] = {0, 0};
	for (size_t i = 0; i < len; ++ i) {
		c[0] = gesture.at(i);
		if (i > 0) {
			text += _T(", ");
		}
		text += pLanguage->getString(c);
	}

	return text;
}



/**
 * Return the command (in string)
 */
xl::tstring CGestureMap::onGesture (const xl::tstring &gesture) {
	xl::tstring command = _T("unknown");
	const xl::tchar *gst = gesture.c_str();
	auto it = std::find_if(m_map.begin(), m_map.end(), 
		[=] (_MapItem &item) {
			return _tcsicmp(gst, item.gesture.c_str()) == 0;
		});
	if (it != m_map.end()) {
		command = it->command;
	}

	return command;
}
