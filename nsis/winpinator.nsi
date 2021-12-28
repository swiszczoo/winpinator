# Winpinator main install script

!include "MUI2.nsh"
!include "FileFunc.nsh"

# Make output directory
!system "mkdir build"

# Setup some constants
!define SETUP_NAME "winpinator_setup"
!define RELEASE_FOLDER "..\vs16\x64\Deploy"
!define SOFTWARE_NAME "Winpinator"
!define ARCH "x64"
!define REG_KEY "Software\Winpinator"

# Retrieve the version of Winpinator we're building this installer for
!getdllversion "${RELEASE_FOLDER}\Winpinator.exe" Version

# Setup compressor
SetCompressor /SOLID lzma
SetCompressorDictSize 32
SetDatablockOptimize on

# Setup NSIS
Name "${SOFTWARE_NAME}"
OutFile "build\${SETUP_NAME}_${Version1}.${Version2}.${Version3}_${ARCH}.exe"
ManifestDPIAware true
Unicode true
RequestExecutionLevel admin
ShowInstDetails show

# Setup VERSIONINFO
VIAddVersionKey "ProductName" "${SOFTWARE_NAME} Installer"
VIAddVersionKey "Comments" "${SOFTWARE_NAME} Installer"
VIAddVersionKey "CompanyName" "Łukasz Świszcz"
VIAddVersionKey "LegalCopyright" "(C) 2021-2022 Łukasz Świszcz"
VIAddVersionKey "FileDescription" "${SOFTWARE_NAME} Installer"
VIAddVersionKey "FileVersion" "${Version1}.${Version2}.${Version3}.${Version4}"
VIAddVersionKey "ProductVersion" "${Version1}.${Version2}.${Version3}.${Version4}"
VIProductVersion "${Version1}.${Version2}.${Version3}.${Version4}"

# Setup install location
!if ARCH == "x64"
    InstallDir "$PROGRAMFILES32\${SOFTWARE_NAME}"
!else
    InstallDir "$PROGRAMFILES64\${SOFTWARE_NAME}"
!endif

InstallDirRegKey HKLM ${REG_KEY} "InstallLocation_${ARCH}"

# Setup MUI2

!define MUI_ICON "icons/install.ico"
!define MUI_UNICON "icons/uninstall.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "inst-banner.bmp"
!define MUI_HEADERIMAGE_BITMAP_STRETCH AspectFitHeight
!define MUI_HEADERIMAGE_UNBITMAP "uninst-banner.bmp"
!define MUI_HEADERIMAGE_UNBITMAP_STRETCH AspectFitHeight

!define MUI_WELCOMEFINISHPAGE_BITMAP "inst-welcome.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP_STRETCH AspectFitHeight
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "uninst-welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP_STRETCH AspectFitHeight

!define MUI_ABORTWARNING
!define MUI_ABORTWARNING_CANCEL_DEFAULT

!define MUI_UNABORTWARNING
!define MUI_UNABORTWARNING_CANCEL_DEFAULT

!define MUI_FINISHPAGE_RUN "$INSTDIR\Winpinator.exe"
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_NOAUTOCLOSE

!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\RELEASE NOTES.txt"
!define MUI_FINISHPAGE_TEXT "text"

!define MUI_UNFINISHPAGE_NOAUTOCLOSE

!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${SOFTWARE_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${REG_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartMenuFolder"

# MUI pages

Var StartMenuDir

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "gpl-3.0.rtf"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuDir
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

# MUI languages

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Polish"

# Multilingual strings

LangString RELEASE_NOTES ${LANG_ENGLISH} "Show Release notes"
LangString RELEASE_NOTES ${LANG_POLISH} "Pokaż informacje o wydaniu"

# View language selector on start
Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Section "DUMMY"
    # todo
SectionEnd
