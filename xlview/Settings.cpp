#include <tuple>
#include <Windows.h>
#include <Shobjidl.h>
#include "libxl/include/utilities.h"
#include "libxl/include/Registry.h"
#include "Settings.h"

static const wchar_t *appRegName = L"xlview.image.1";
static const wchar_t *appName = L"xlview";
static const xl::tchar *appRegNameT = _T("xlview.image.1");
static const xl::tchar *appNameT = _T("xlview");


bool launchAssociationOnVista () {
	IApplicationAssociationRegistrationUI *pUI = NULL;

	HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistrationUI, 
			NULL,
			CLSCTX_INPROC,
			__uuidof(IApplicationAssociationRegistrationUI),
			(void **)&pUI);

	if (SUCCEEDED(hr)) {
		hr = pUI->LaunchAdvancedAssociationUI(appRegName);
		pUI->Release();

		return SUCCEEDED(hr);
	}

	return false;
}

bool restoreDefault4Vista (const xl::tstring &ext) {
	static wchar_t *defaultAppRegName = L"Windows Photo Viewer";

	if (ext.length() == 0) {
		assert(false);
		return false;
	}
	xl::wstring wext = xl::ts2ws(ext);
	if (wext.at(0) != _T('.')) {
		wext = _T(".") + wext;
	}
	IApplicationAssociationRegistration *paar = NULL;
	HRESULT hr = CoCreateInstance(
	                              CLSID_ApplicationAssociationRegistration,
	                              NULL,
	                              CLSCTX_INPROC,
	                              __uuidof(IApplicationAssociationRegistration),
	                              (void **)&paar
	                             );
	if (SUCCEEDED(hr)) {
		BOOL isDefault = false;
		HRESULT hr = paar->QueryAppIsDefault(
		                                     wext.c_str(),
		                                     AT_FILEEXTENSION,
		                                     AL_EFFECTIVE,
		                                     defaultAppRegName,
		                                     &isDefault
		                                    );
		if (SUCCEEDED(hr)) {
			if (!isDefault) {
				hr = paar->SetAppAsDefault(defaultAppRegName, wext.c_str(), AT_FILEEXTENSION);
			}
			paar->Release();
			return SUCCEEDED(hr);
		} else {
			paar->Release();
			return false;
		}
	}
	return false;
}

bool restoreDefault4Vista () {
	const xl::tchar *exts[] = {
		_T("jpg"),
		_T("jpeg"),
		_T("jfif"),
		_T("png"),
	};

	for (size_t i = 0; i < COUNT_OF(exts); ++ i) {
		if (!restoreDefault4Vista(exts[i])) {
			return false;
		}
	}
	return true;
}

void launchAssociation () {
	if (xl::os_is_vista_or_later()) {
		launchAssociationOnVista();
	} else {
	}
}

#if 0
void test () {
	IApplicationAssociationRegistration *paar = NULL;

	HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistration, 
			NULL,
			CLSCTX_INPROC,
			__uuidof(IApplicationAssociationRegistration),
			(void **)&paar);

	if (SUCCEEDED(hr)) {
		paar->Release();
	}

}
#endif

bool isAppRegistered () {
	// check for:
	// - HKLM\Software\Classes\xlview.image.1\shell\open\command
	// - HKLM\Software\xlview\Capabilities\FileAssociations
	xl::tstring value;
	xl::tstring key = _T("HKLM\\Software\\Classes\\");
	key += appRegNameT;
	key += _T("\\shell\\open\\command");
	if (!xl::CRegistry::getStringValue(key, _T(""), value)) {
		return false;
	}

	key = _T("HKLM\\Software\\");
	key += appNameT;
	key += _T("\\Capabilities\\FileAssociations");
	if (!xl::CRegistry::getStringValue(key, _T(".jpg"), value)) {
		return false;
	}

	return true;
}

bool registerApplication () {
	xl::tchar pathName[MAX_PATH];
	::GetModuleFileName(NULL, pathName, MAX_PATH);
	xl::tstring command = _T("\"");
	command += pathName;
	command += _T("\" \"%1\"");

	typedef std::tr1::tuple<const xl::tchar *, const xl::tchar *, const xl::tchar *>  RegTuple;
	RegTuple tuples[] = {
		std::make_tuple(_T("HKLM\\Software\\Classes\\$appRegName"), _T(""), _T("Image supported by xlview")),
		std::make_tuple(_T("HKLM\\Software\\Classes\\$appRegName\\DefaultIcon"), _T(""), pathName),
		std::make_tuple(_T("HKLM\\Software\\Classes\\$appRegName\\shell\\open"), _T(""), _T("Open with xlview")),
		std::make_tuple(_T("HKLM\\Software\\Classes\\$appRegName\\shell\\open"), _T("MuiVerb"), _T("Open with xlview")),
		std::make_tuple(_T("HKLM\\Software\\Classes\\$appRegName\\shell\\open\\command"), _T(""), command.c_str()),

		// Capabilities
		std::make_tuple(_T("HKLM\\Software\\$appName\\Capabilities"), _T("ApplicationDescription"), _T("xlview is a fast and easy to use image viewer")),
		std::make_tuple(_T("HKLM\\Software\\$appName\\Capabilities"), _T("ApplicationName"), _T("xlview")),
		std::make_tuple(_T("HKLM\\Software\\$appName\\Capabilities\\FileAssociations"), _T(".jpg"), _T("$appRegName")),
		std::make_tuple(_T("HKLM\\Software\\$appName\\Capabilities\\FileAssociations"), _T(".jpeg"), _T("$appRegName")),
		std::make_tuple(_T("HKLM\\Software\\$appName\\Capabilities\\FileAssociations"), _T(".jfif"), _T("$appRegName")),
		std::make_tuple(_T("HKLM\\Software\\$appName\\Capabilities\\FileAssociations"), _T(".png"), _T("$appRegName")),
	
		// registered application 
		std::make_tuple(_T("HKLM\\Software\\RegisteredApplications"), _T("$appRegName"), _T("Software\\$appName\\Capabilities")),
	};

	for (size_t i = 0; i < COUNT_OF(tuples); ++ i) {
		xl::tstring keyName = std::get<0>(tuples[i]);
		xl::tstring valueName = std::get<1>(tuples[i]);
		xl::tstring value = std::get<2>(tuples[i]);

		keyName.replace(_T("$appRegName"), appRegName);
		keyName.replace(_T("$appName"), appName);
		valueName.replace(_T("$appRegName"), appRegName);
		valueName.replace(_T("$appName"), appName);
		value.replace(_T("$appRegName"), appRegName);
		value.replace(_T("$appName"), appName);

		if (!xl::CRegistry::setValue(keyName, valueName, value)) {
			return false;
		}
	}

	return true;
}

bool isDefault4Xp (const xl::tstring &ext) {
	assert(xl::os_is_xp());
	assert(ext.length() > 0 && ext.at(0) != _T('.'));
	if (!xl::os_is_xp()) {
		return false;
	}

	// We check it should use HKCR
	xl::tstring keyName = _T("HKCR\\.") + ext;
	xl::tstring valueName = _T("");
	xl::tstring value;
	if (!xl::CRegistry::getStringValue(keyName, valueName, value)) {
		return false;
	}

	return _tcscmp(value.c_str(), appRegName) == 0;
}


// set our settings into HKCU
bool setDefault4Xp (const xl::tstring &ext) {
	assert(xl::os_is_xp());
	assert(ext.length() > 0 && ext.at(0) != _T('.'));
	if (!xl::os_is_xp()) {
		return false;
	}

	xl::tstring keyName = _T("HKCU\\Software\\Classes\\.") + ext;
	xl::tstring valueName = _T("");
	xl::tstring value = appRegName;
	return xl::CRegistry::setValue(keyName, valueName, value);
}

bool restoreDefault4Xp (const xl::tstring &ext, const xl::tstring &def) {
	assert(xl::os_is_xp());
	assert(ext.length() > 0 && ext.at(0) != _T('.'));
	assert(def.length() > 0);
	if (!xl::os_is_xp()) {
		return false;
	}

	// 1. check for HKLM
	xl::tstring keyName = _T("HKLM\\Software\\Classes\\.") + ext;
	xl::tstring valueName = _T("");
	xl::tstring value;
	if (xl::CRegistry::getStringValue(keyName, valueName, value) && value.length() > 0) {
		// if HKLM has value, just delete ours from HKCU
		keyName = _T("HKCU\\Software\\Classes\\.") + ext;
		return xl::CRegistry::deleteKey(keyName);
	} else {
		// if HKLM doesn't has value, so we set it into HKLM
		value = def;
		return xl::CRegistry::setValue(keyName, valueName, value);
	}
}

