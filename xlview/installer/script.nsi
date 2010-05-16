; The script for xlview
;
; References:
; http://nsis.sourceforge.net/Shortcuts_removal_fails_on_Windows_Vista
;
OutFile "xlview.setup.exe"

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
	WriteRegStr HKLM SOFTWARE\xlview "Install_Dir" "$INSTDIR"

	; Write the uninstall keys for Windows
	Call writeUninstallKeys
	Call writeApplicationKeys

	; write the uninstall program
	WriteUninstaller "uninstall.exe"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

	SetShellVarContext all
	CreateDirectory "$SMPROGRAMS\xlview"
	CreateShortCut "$SMPROGRAMS\xlview\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	; CreateShortCut "$SMPROGRAMS\xlview\xlview.lnk" "$INSTDIR\xlview.exe" "" "$INSTDIR\xlview.exe" 0

SectionEnd

section "uninstall"
	; Remove registry keys
	Call un.deleteUninstallKeys
	Call un.deleteApplicationKeys
	DeleteRegKey HKLM SOFTWARE\xlview

	; Remove files and uninstaller
	Delete $INSTDIR\xlview.exe
	Delete $INSTDIR\uninstall.exe

	SetShellVarContext all
	; Remove shortcuts, if any
	Delete "$SMPROGRAMS\xlview\*.*"

	; Remove directories used
	RMDir "$SMPROGRAMS\xlview"
	RMDir "$INSTDIR"
sectionEnd


Function .onInit
#TODO: call UserInfo plugin to make sure user in admin
FunctionEnd


; Function write uninstall keys for Windows
Function writeUninstallKeys
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\xlview" "DisplayName" "xlview"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\xlview" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\xlview" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\xlview" "NoRepair" 1
FunctionEnd

Function un.deleteUninstallKeys
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\xlview"
FunctionEnd

; Function write the application keys
Function writeApplicationKeys

	; progid
	WriteRegStr HKLM "Software\Classes\xlview.image.1" "DefaultIcon" "$INSTDIR\xlview.exe"
	WriteRegStr HKLM "Software\Classes\xlview.image.1\shell\open" "" "Open with xlview"
	WriteRegStr HKLM "Software\Classes\xlview.image.1\shell\open\command" "" '"$INSTDIR\xlview.exe" "%1"'

	; Capabilities
	WriteRegStr HKLM "Software\xlview\Capabilities" "ApplicationDescription" "xlview is a fast and easy to use image viewer"
	WriteRegStr HKLM "Software\xlview\Capabilities" "ApplicationName" "xlview"
	WriteRegStr HKLM "Software\xlview\Capabilities\FileAssociations" ".jpg" "xlview.image.1"
	WriteRegStr HKLM "Software\xlview\Capabilities\FileAssociations" ".jpeg" "xlview.image.1"
	WriteRegStr HKLM "Software\xlview\Capabilities\FileAssociations" ".jfif" "xlview.image.1"
	WriteRegStr HKLM "Software\xlview\Capabilities\FileAssociations" ".png" "xlview.image.1"

	; registered application 
	WriteRegStr HKLM "Software\RegisteredApplications" "xlview" "Software\xlview\Capabilities"

FunctionEnd

Function un.deleteApplicationKeys

	; delete progid
	DeleteRegKey HKLM "Software\Classes\xlview.image.1"

	; delete capabilities
	DeleteRegKey HKLM "Software\xlview"

	; delete registered application
	DeleteRegValue HKLM "Software\RegisteredApplications" "xlview"

FunctionEnd


