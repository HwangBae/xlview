#include <Windows.h>
#include <Shobjidl.h>
#include "libxl/include/utilities.h"
#include "Settings.h"

static const wchar_t *appRegName = L"xlview.image.1";

static void launchAssociationOnVista () {
	IApplicationAssociationRegistrationUI *pUI = NULL;

	HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistrationUI, 
			NULL,
			CLSCTX_INPROC,
			__uuidof(IApplicationAssociationRegistrationUI),
			(void **)&pUI);

	if (SUCCEEDED(hr)) {
		hr = pUI->LaunchAdvancedAssociationUI(appRegName);
		pUI->Release();
	}
}

void launchAssociation () {
	if (xl::os_is_vista_or_later()) {
		launchAssociationOnVista();
	} else {
	}
}
