; The script for xlview
;
; References:
; http://nsis.sourceforge.net/Shortcuts_removal_fails_on_Windows_Vista
;

; Include the Modern UI
!include "MUI2.nsh"

; The name of the installer
Name "xlview"

; The file to write
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
!define MUI_ABORTWARNING

;-----------------------------------------------------------

; Pages

; Page components
;
; Page directory
; Page instfiles
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; UninstPage uninstConfirm
; UninstPage instfiles
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;-----------------------------------------------------------
; Languages
!insertmacro MUI_LANGUAGE "English" ;first language is the default language
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Croatian"
!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Thai"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Latvian"
!insertmacro MUI_LANGUAGE "Macedonian"
!insertmacro MUI_LANGUAGE "Estonian"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Lithuanian"
!insertmacro MUI_LANGUAGE "Slovenian"
!insertmacro MUI_LANGUAGE "Serbian"
!insertmacro MUI_LANGUAGE "SerbianLatin"
!insertmacro MUI_LANGUAGE "Arabic"
!insertmacro MUI_LANGUAGE "Farsi"
!insertmacro MUI_LANGUAGE "Hebrew"
!insertmacro MUI_LANGUAGE "Indonesian"
!insertmacro MUI_LANGUAGE "Mongolian"
!insertmacro MUI_LANGUAGE "Luxembourgish"
!insertmacro MUI_LANGUAGE "Albanian"
!insertmacro MUI_LANGUAGE "Breton"
!insertmacro MUI_LANGUAGE "Belarusian"
!insertmacro MUI_LANGUAGE "Icelandic"
!insertmacro MUI_LANGUAGE "Malay"
!insertmacro MUI_LANGUAGE "Bosnian"
!insertmacro MUI_LANGUAGE "Kurdish"
!insertmacro MUI_LANGUAGE "Irish"
!insertmacro MUI_LANGUAGE "Uzbek"
!insertmacro MUI_LANGUAGE "Galician"
!insertmacro MUI_LANGUAGE "Afrikaans"
!insertmacro MUI_LANGUAGE "Catalan"
!insertmacro MUI_LANGUAGE "Esperanto"

; Reserve Files
; !insertmacro MUI_RESERVEFILE_LANGDLL

;-----------------------------------------------------------

; The stuff to install

Section "xlview (required)"

	SectionIn RO

	; Set output path to the installation directory
	SetOutPath $INSTDIR

	; Put file there
	File "..\..\Release\xlview.exe"
	File "..\lang-0409.ini"
	File "..\lang-0804.ini"

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

; --------------------------------------------------------------
; Uninstaller Section

section "Uninstall"
	StrCmp $progId "" var_error
	StrCmp $appName "" var_error

	; Remove registry keys
	Call un.deleteFileAssociation
	Call un.deleteApplicationKeys
	Call un.deleteUninstallKeys
	DeleteRegKey HKLM SOFTWARE\$appName

	; Remove files and uninstaller
	Delete $INSTDIR\xlview.exe
	Delete $INSTDIR\uninstall.exe
	Delete $INSTDIR\*.*

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

	Call un.detectWindowsVersion
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
	WriteRegStr HKLM "Software\Classes\$progId" "" "Image supported by xlview"
	WriteRegStr HKLM "Software\Classes\$progId\DefaultIcon" "" "$INSTDIR\xlview.exe"
	WriteRegStr HKLM "Software\Classes\$progId\shell\open" "" "Open with xlview"
	WriteRegStr HKLM "Software\Classes\$progId\shell\open" "MuiVerb" "Open with xlview" ; it seems no use
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

	; write for different windows version
	StrCmp $winMajorVersion '5' write_4_xp
	StrCmp $winMajorVersion '6' write_4_vista
	Goto write_done

	write_4_xp:
	WriteRegStr HKCU "Software\Classes\.jpg" "" "$progId"
	WriteRegStr HKCU "Software\Classes\.jpeg" "" "$progId"
	WriteRegStr HKCU "Software\Classes\.jfif" "" "$progId"
	WriteRegStr HKCU "Software\Classes\.png" "" "$progId"
	Goto write_done

	write_4_vista:
	ExecWait '"$INSTDIR\xlview.exe" /setdefault'
	Goto write_done

	write_done:

FunctionEnd

Function un.deleteApplicationKeys

	; delete progid
	DeleteRegKey HKLM "Software\Classes\$progId"

	; delete capabilities
	DeleteRegKey HKLM "Software\$appName"

	; delete registered application
	DeleteRegValue HKLM "Software\RegisteredApplications" "$progId"

	; delete the shell muicache
	DeleteRegValue HKCU "Software\Classes\Local Settings\Software\Microsoft\Windows\Shell\MuiCache" "$INSTDIR\xlview.exe"

	; for different windows version, restore the default settings
	StrCmp $winMajorVersion '5' write_4_xp
	StrCmp $winMajorVersion '6' write_4_vista
	Goto write_done

	write_4_xp:
;	WriteRegStr HKCU "Software\Classes\.jpg" "" "jpegfile"
;	WriteRegStr HKCU "Software\Classes\.jpeg" "" "jpegfile"
;	WriteRegStr HKCU "Software\Classes\.jfif" "" "jpegfile"
;	WriteRegStr HKCU "Software\Classes\.png" "" "pngfile"
	DeleteRegKey HKCU "Software\Classes\.jpg"
	DeleteRegKey HKCU "Software\Classes\.jpeg"
	DeleteRegKey HKCU "Software\Classes\.jfif"
	DeleteRegKey HKCU "Software\Classes\.png"
	Goto write_done

	write_4_vista:
	ExecWait '"$INSTDIR\xlview.exe" /restoredefault'
	Goto write_done

	write_done:

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

Function un.detectWindowsVersion
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
