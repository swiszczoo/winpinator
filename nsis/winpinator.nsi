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
!define MUI_FINISHPAGE_SHOWREADME_TEXT "$(RELEASE_NOTES)"

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

LangString DESKTOP_SHORTCUT ${LANG_ENGLISH} "Create Desktop shortcut"
LangString DESKTOP_SHORTCUT ${LANG_POLISH} "Utwórz skrót na pulpicie"

LangString RELEASE_NOTES ${LANG_ENGLISH} "Show Release notes"
LangString RELEASE_NOTES ${LANG_POLISH} "Pokaż informacje o wydaniu"

# View language selector on start
Function .onInit
    !insertmacro MUI_LANGDLL_DISPLAY

    # Retrieve last install dir from registry
    ReadRegStr $0 HKLM ${REG_KEY} "InstallLocation"
    StrLen $1 $0
    IntCmp $1 5 done done
        IntOp $2 $1 - 2
        StrCpy $INSTDIR $0 $2 1
    done:
FunctionEnd

Section "DUMMY"
    # todo
SectionEnd

# https://nsis.sourceforge.io/Check_if_dir_is_empty
Function isEmptyDir
  # Stack ->                    # Stack: <directory>
  Exch $0                       # Stack: $0
  Push $1                       # Stack: $1, $0
  FindFirst $0 $1 "$0\*.*"
  strcmp $1 "." 0 _notempty
    FindNext $0 $1
    strcmp $1 ".." 0 _notempty
      ClearErrors
      FindNext $0 $1
      IfErrors 0 _notempty
        FindClose $0
        Pop $1                  # Stack: $0
        StrCpy $0 1
        Exch $0                 # Stack: 1 (true)
        goto _end
     _notempty:
       FindClose $0
       ClearErrors
       Pop $1                   # Stack: $0
       StrCpy $0 0
       Exch $0                  # Stack: 0 (false)
  _end:
FunctionEnd
