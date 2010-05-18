; The script for xlview
;
; References:
; http://nsis.sourceforge.net/Shortcuts_removal_fails_on_Windows_Vista
;
OutFile "xlview.setup.exe"

;-----------------------------------------------------------

; Local variables

; windows version, xp is 5 and vista, win7 is 6
; initialized in detectWindowsVersion
Var winMajorVersion

; initialized in .onInit and ** un.onInit ** !!important!!
Var progId ; xlview.image.1
Var appName ; xlview


; The default installation directory
InstallDir $PROGRAMFILES\xlview

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\xlview" "Install_Dir"

; Request application privileges for windows vista or 7
RequestExecutionLevel admin

;-----------------------------------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;-----------------------------------------------------------

; The stuff to install

Section "xlview (required)"

	SectionIn RO

	; Set output path to the installation directory
	SetOutPath $INSTDIR

	; Put file there
	File "..\..\Release\xlview.exe"

	; Write the installation path into the registry
	WriteRegStr HKLM SOFTWARE\$appName "Install_Dir" "$INSTDIR"

	; Write the uninstall keys for Windows
	Call writeUninstallKeys
	Call writeApplicationKeys
	Call writeFileAssociation

	; write the uninstall program
	WriteUninstaller "uninstall.exe"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

	SetShellVarContext all
	CreateDirectory "$SMPROGRAMS\$appName"
	CreateShortCut "$SMPROGRAMS\$appName\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	; CreateShortCut "$SMPROGRAMS\$appName\xlview.lnk" "$INSTDIR\xlview.exe" "" "$INSTDIR\xlview.exe" 0

SectionEnd

section "uninstall"
	StrCmp $appName "" var_error

	; Remove registry keys
	Call un.deleteUninstallKeys
	Call un.deleteApplicationKeys
	Call un.deleteFileAssociation
	DeleteRegKey HKLM SOFTWARE\$appName

	; Remove files and uninstaller
	Delete $INSTDIR\xlview.exe
	Delete $INSTDIR\uninstall.exe

	SetShellVarContext all
	; Remove shortcuts, if any
	Delete "$SMPROGRAMS\$appName\*.*"

	; Remove directories used
	RMDir "$SMPROGRAMS\$appName"
	RMDir "$INSTDIR"
	Quit

var_error:
	MessageBox MB_OK "error!"
	Quit
sectionEnd


Function .onInit
	StrCpy $progId 'xlview.image.1'
	StrCpy $appName 'xlview'

#TODO: call UserInfo plugin to make sure user in admin
	Call detectWindowsVersion
FunctionEnd

Function un.onInit
	StrCpy $progId 'xlview.image.1'
	StrCpy $appName 'xlview'
FunctionEnd


; Function write uninstall keys for Windows
Function writeUninstallKeys
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$appName" "DisplayName" "$appName"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$appName" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$appName" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$appName" "NoRepair" 1
FunctionEnd

Function un.deleteUninstallKeys
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$appName"
FunctionEnd

; Function write the application keys
Function writeApplicationKeys

	; progid
	WriteRegStr HKLM "Software\Classes\$progId" "DefaultIcon" "$INSTDIR\xlview.exe"
	WriteRegStr HKLM "Software\Classes\$progId\shell\open" "" "Open with xlview"
	WriteRegStr HKLM "Software\Classes\$progId\shell\open\command" "" '"$INSTDIR\xlview.exe" "%1"'

	; Capabilities
	WriteRegStr HKLM "Software\$appName\Capabilities" "ApplicationDescription" "xlview is a fast and easy to use image viewer"
	WriteRegStr HKLM "Software\$appName\Capabilities" "ApplicationName" "xlview"
	WriteRegStr HKLM "Software\$appName\Capabilities\FileAssociations" ".jpg" "$progId"
	WriteRegStr HKLM "Software\$appName\Capabilities\FileAssociations" ".jpeg" "$progId"
	WriteRegStr HKLM "Software\$appName\Capabilities\FileAssociations" ".jfif" "$progId"
	WriteRegStr HKLM "Software\$appName\Capabilities\FileAssociations" ".png" "$progId"

	; registered application 
	WriteRegStr HKLM "Software\RegisteredApplications" "$progId" "Software\$appName\Capabilities"

FunctionEnd

Function un.deleteApplicationKeys

	; delete progid
	DeleteRegKey HKLM "Software\Classes\$progId"

	; delete capabilities
	DeleteRegKey HKLM "Software\$appName"

	; delete registered application
	DeleteRegValue HKLM "Software\RegisteredApplications" "$progId"

FunctionEnd

; Function write the file association
Function writeFileAssociation
	WriteRegStr HKCU "Software\Classes\.png\OpenWithProgids" "$progId" ""
	WriteRegStr HKCU "Software\Classes\.jpg\OpenWithProgids" "$progId" ""
	WriteRegStr HKCU "Software\Classes\.jpeg\OpenWithProgids" "$progId" ""
	WriteRegStr HKCU "Software\Classes\.jfif\OpenWithProgids" "$progId" ""
FunctionEnd

Function un.deleteFileAssociation
	DeleteRegValue HKCU "Software\Classes\.png\OpenWithProgids" "$progId"
	DeleteRegValue HKCU "Software\Classes\.jpg\OpenWithProgids" "$progId"
	DeleteRegValue HKCU "Software\Classes\.jpeg\OpenWithProgids" "$progId"
	DeleteRegValue HKCU "Software\Classes\.jfif\OpenWithProgids" "$progId"
FunctionEnd

; Function detect windows version
; The major version number is stored in winMajorVersion
; from: http://nsis.sourceforge.net/Windows_Version_Detection
Function detectWindowsVersion
	Push $R0
	Push $R1
 
	ClearErrors
	StrCpy $winMajorVersion '0'
 
	ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
	IfErrors 0 version_gotten
version_gotten:
	StrCpy $winMajorVersion $R0 1
 
	Pop $R1
	Exch $R0
FunctionEnd
