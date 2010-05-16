#include <Windows.h>
#include <Shobjidl.h>
#include "libxl/include/utilities.h"
#include "Registry.h"

///////////////////////////////////////////////////////////////////////////////
CRegNodeItem::CRegNodeItem (const xl::tstring &name, DWORD type, const xl::tstring &data)
	: m_name(name)
	, m_type(type)
	, m_data(data)
{
}

bool CRegNodeItem::write (HKEY hParent) {
	long status = ~ERROR_SUCCESS;
	switch (m_type) {
	case REG_NONE:
		status = ::RegSetValueEx(hParent, m_name.c_str(), 0, REG_NONE, NULL, 0);
		break;
	case REG_SZ:
	case REG_EXPAND_SZ:
	case REG_MULTI_SZ:
		status = ::RegSetValueEx(hParent, m_name.c_str(), 0, m_type,
				(const BYTE *)m_data.c_str(), (m_data.length() + 1) * sizeof(TCHAR)); // include the \0 terminator
		break;
	default:
		assert(false); // not supported now :)
		break;
	}

	return status == ERROR_SUCCESS;
}


CRegNode::CRegNode (const xl::tstring &name) : m_name(name) {
}


bool CRegNode::write (HKEY hParent) {
	long status = 0;
	HKEY hKey = NULL;
	DWORD dwDisp = 0;

	// 1. write the key itself
	status = ::RegCreateKeyEx(hParent, m_name.c_str(), 0, NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL,
			&hKey, &dwDisp);
	if (status != ERROR_SUCCESS) {
		return false;
	}

	bool result = true;

	// 2. write the items
	for (auto it = m_items.begin(); it != m_items.end(); ++ it) {
		if (!(*it)->write(hKey)) {
			result = false;
		}
	}

	// 3. write the sub-keys
	for (auto it = m_subKeys.begin(); it != m_subKeys.end(); ++ it) {
		if (!(*it)->write(hKey)) {
			result = false;
		}
	}

	::RegCloseKey(hKey);
	hKey = NULL;
	return result;
}

void CRegNode::addItem (const xl::tstring &name, DWORD type, const xl::tstring &data) {
	CRegNodeItemPtr item(new CRegNodeItem(name, type, data));
	m_items.push_back(item);
}

void CRegNode::addKey (CRegNodePtr key) {
	m_subKeys.push_back(key);
}



///////////////////////////////////////////////////////////////////////////////

static const HKEY          ROOT_KEY = HKEY_CURRENT_USER;

REGISTRYSTATUS isAppRegistered (const xl::tstring &app) {
	DWORD dwType = 0;
	BYTE data[MAX_PATH];
	DWORD dwDataLength = MAX_PATH;

	HKEY hKey = NULL;
	xl::tstring key = _T("Software\\Classes\\") + app;
	long status = RegOpenKeyEx(ROOT_KEY, key.c_str(), 0, KEY_QUERY_VALUE, &hKey);
	if (status != ERROR_SUCCESS) {
		return RS_NONE;
	}
	RegCloseKey(hKey);
	hKey = NULL;

	key += _T("\\shell\\open\\command");
	status = RegOpenKeyEx(ROOT_KEY, key.c_str(), 0, KEY_QUERY_VALUE, &hKey);
	if (status != ERROR_SUCCESS) {
		return RS_PARTIAL;
	}
	status = RegQueryValueEx(hKey, NULL, NULL, &dwType, data, &dwDataLength);
	RegCloseKey(hKey);

	if (status != ERROR_SUCCESS) {
		return RS_PARTIAL;
	}

	// test if the command is right
	TCHAR pathModule[MAX_PATH] = {_T('0')};
	xl::tstring command = _T("\"");
	::GetModuleFileName(NULL, pathModule, MAX_PATH);
	command += pathModule;
	command += _T("\" \"%1\"");
	if (dwType != REG_SZ || _tcsicmp(command.c_str(), (TCHAR *)data) != 0) {
		return RS_PARTIAL;
	}

	return RS_OK;
}

bool registerApp (const xl::tstring &app, const xl::tstring &name) {
	if (isAppRegistered(app) == RS_OK) {
		return true;
	}

	TCHAR pathModule[MAX_PATH] = {_T('0')};
	::GetModuleFileName(NULL, pathModule, MAX_PATH);

	xl::tstring key = _T("Software\\Classes\\") + app;
	CRegNode node(key);

	// DefaultIcon
	CRegNodePtr defaultIcon(new CRegNode(_T("DefaultIcon")));
	node.addKey(defaultIcon);
	defaultIcon->addItem(_T(""), REG_SZ, pathModule);

	// shell
	CRegNodePtr shell(new CRegNode(_T("shell")));
	node.addKey(shell);
	// shell\open
	CRegNodePtr open(new CRegNode(_T("open")));
	shell->addKey(open);
	open->addItem(_T("FriendlyAppName"), REG_SZ, name);
	// shell\open\command
	CRegNodePtr command(new CRegNode(_T("command")));
	open->addKey(command);
	// command line
	xl::tstring commandLine = _T("\"");
	commandLine += pathModule;
	commandLine += _T("\" \"%1\"");
	command->addItem(_T(""), REG_SZ, commandLine);

	return node.write(ROOT_KEY);
}

void unregisterApp (const xl::tstring &app) {
}

bool registerExt (const xl::tstring &ext, const xl::tstring &app) {
	if (isAppRegistered(app) != RS_OK) {
		return false;
	}

	xl::tstring key = _T("Software\\Classes\\.") + ext;
	CRegNode node(key);
	CRegNodePtr OpenWithProgids(new CRegNode(_T("OpenWithProgids")));
	node.addKey(OpenWithProgids);
	OpenWithProgids->addItem(app, REG_NONE, _T(""));
	return node.write(ROOT_KEY);
}

bool registerExtAsDefault (const xl::tstring &ext, const xl::tstring &app) {
	if (!registerExt(ext, app)) {
		return false;
	}

	// below copy from MSDN (http://msdn.microsoft.com/en-us/library/bb776337%28VS.85%29.aspx)
	IApplicationAssociationRegistration* pAAR;

	HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistration,
				NULL,
				CLSCTX_INPROC,
				__uuidof(IApplicationAssociationRegistration),
				(void**)&pAAR);
	if (SUCCEEDED(hr))
	{
		xl::tstring e = _T(".") + ext;
		hr = pAAR->SetAppAsDefault(app.c_str(),
				e.c_str(),
				AT_FILEEXTENSION);
		pAAR->Release();
	}

	return SUCCEEDED(hr);
}
