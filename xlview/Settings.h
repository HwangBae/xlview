#ifndef XL_VIEW_SETTINGS_H
#define XL_VIEW_SETTINGS_H
#include "libxl/include/string.h"

bool isAppRegistered ();
bool registerApplication (); // call it under XP or in administrator mode
void launchAssociation ();
bool launchAssociationOnVista ();

bool isDefault4Xp (const xl::tstring &ext);
bool setDefault4Xp (const xl::tstring &ext);
bool restoreDefault4Xp (const xl::tstring &ext, const xl::tstring &def);

#endif
