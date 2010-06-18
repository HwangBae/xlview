#include <Windows.h>
#include <Shobjidl.h>
#include "libxl/include/utilities.h"
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
	HKEY hKey = NULL;
	long status = 0;
	xl::tstring key = _T("Software\\Classes\\");
	key += appRegNameT;
	key += _T("\\shell\\open\\command");
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_QUERY_VALUE, &hKey);
	if (status != ERROR_SUCCESS) {
		return false;
	}
	RegCloseKey(hKey);
	hKey = NULL;

	key = _T("Software\\");
	key += appNameT;
	key += _T("\\Capabilities\\FileAssociations");
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_QUERY_VALUE, &hKey);
	if (status != ERROR_SUCCESS) {
		return false;
	}
	RegCloseKey(hKey);
	hKey = NULL;
	return true;
}
