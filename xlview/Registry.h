#ifndef XL_VIEW_REGISTRY_H
#define XL_VIEW_REGISTRY_H
#include <memory>
#include <vector>
#include "libxl/include/common.h"
#include "libxl/include/string.h"

class CRegNodeItem;
typedef std::tr1::shared_ptr<CRegNodeItem>             CRegNodeItemPtr;
typedef std::vector<CRegNodeItemPtr>                   CRegNodeItems;
class CRegNodeItem {
	xl::tstring        m_name;
	DWORD              m_type;
	xl::tstring        m_data;
public:
	CRegNodeItem (const xl::tstring &name, DWORD type, const xl::tstring &data);
	bool write (HKEY hParent);
};



class CRegNode;
typedef std::tr1::shared_ptr<CRegNode>                 CRegNodePtr;
typedef std::vector<CRegNodePtr>                       CRegNodes;
class CRegNode {
	xl::tstring        m_name;
	CRegNodeItems      m_items;
	CRegNodes          m_subKeys;
public:
	CRegNode (const xl::tstring &name);
	bool write (HKEY hParent);
	void addItem (const xl::tstring &name, DWORD type, const xl::tstring &data);
	void addKey (CRegNodePtr key);
};



enum REGISTRYSTATUS {
	RS_OK,             // the registry item is OK
	RS_NONE,           // the registry item does not exist
	RS_PARTIAL         // the registry item exists, but not complete
};


/**
 * test whether @appkey is in HKCU\Software\Classes\
 * and whether it has shell & shell\open key
 */
REGISTRYSTATUS isAppRegistered (const xl::tstring &app);


bool registerApp (const xl::tstring &app, const xl::tstring &shellName);
void unregisterApp (const xl::tstring &app);

bool registerExt (const xl::tstring &ext, const xl::tstring &app);
bool registerExtAsDefault (const xl::tstring &ext, const xl::tstring &app);


#endif
