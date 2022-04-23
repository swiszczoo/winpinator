# Winpinator main install script

!include "MUI2.nsh"
!include "FileFunc.nsh"

# Make output directory
!system "mkdir build"

# Setup some constants
!define SETUP_NAME "winpinator_setup"
!define SOFTWARE_NAME "Winpinator"
!define REG_KEY "Software\Winpinator"

!ifndef RELEASE_FOLDER
!define RELEASE_FOLDER "..\vs16\x64\Deploy"
!endif

# Retrieve the version of Winpinator we're building this installer for
!getdllversion "${RELEASE_FOLDER}\Winpinator.exe" Version

# Setup compressor
SetCompressor /SOLID lzma
SetCompressorDictSize 64
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
VIAddVersionKey "LegalCopyright" "Copyright (C) 2021-2022 Łukasz Świszcz"
VIAddVersionKey "FileDescription" "${SOFTWARE_NAME} Installer"
VIAddVersionKey "FileVersion" "${Version1}.${Version2}.${Version3}.${Version4}"
VIAddVersionKey "ProductVersion" "${Version1}.${Version2}.${Version3}.${Version4}"
VIProductVersion "${Version1}.${Version2}.${Version3}.${Version4}"

# Setup install location
!if ${ARCH} == "x64"
    InstallDir "$PROGRAMFILES64\${SOFTWARE_NAME}"
!else
    InstallDir "$PROGRAMFILES32\${SOFTWARE_NAME}"
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
Var ins86_1
Var ins86_1_maj
Var ins86_1_min
Var ins64_1
Var ins64_1_maj
Var ins64_1_min
Var ins86_2
Var ins86_2_maj
Var ins86_2_min
Var ins64_2
Var ins64_2_maj
Var ins64_2_min

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
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Polish"

# Multilingual strings

LangString DESKTOP_SHORTCUT ${LANG_ENGLISH} "Create Desktop shortcut"
LangString DESKTOP_SHORTCUT ${LANG_POLISH} "Utwórz skrót na pulpicie"
LangString DESKTOP_SHORTCUT ${LANG_FRENCH} "Créer un raccourci sur le bureau"
LangString DESKTOP_SHORTCUT ${LANG_GERMAN} "Erstelle Desktop Verknüpfung"

LangString RELEASE_NOTES ${LANG_ENGLISH} "Show Release notes"
LangString RELEASE_NOTES ${LANG_POLISH} "Pokaż informacje o wydaniu"
LangString RELEASE_NOTES ${LANG_FRENCH} "Voir les notes de mise à jour"
LangString RELEASE_NOTES ${LANG_GERMAN} "Zeige die Versions-Hinweise"

LangString SECTION_WINPINATOR ${LANG_ENGLISH} "Winpinator"
LangString SECTION_WINPINATOR ${LANG_POLISH} "Winpinator"
LangString SECTION_WINPINATOR ${LANG_FRENCH} "Winpinator"
LangString SECTION_WINPINATOR ${LANG_GERMAN} "Winpinator"

LangString DESC_WINPINATOR ${LANG_ENGLISH} "Winpinator application with all dependencies"
LangString DESC_WINPINATOR ${LANG_POLISH} "Aplikacja Winpinator wraz z zależnościami"
LangString DESC_WINPINATOR ${LANG_FRENCH} "Application Winpinator avec toutes les dépendances"
LangString DESC_WINPINATOR ${LANG_GERMAN} "Winpinator mit allen Abhängigkeiten"

LangString SECTION_VCREDIST ${LANG_ENGLISH} "Runtime libraries"
LangString SECTION_VCREDIST ${LANG_POLISH} "Biblioteki uruchomieniowe"
LangString SECTION_VCREDIST ${LANG_FRENCH} "Librairies Runtime"
LangString SECTION_VCREDIST ${LANG_GERMAN} "Laufzeitbibliotheken"

LangString DESC_VCREDIST ${LANG_ENGLISH} "Microsoft Visual C++ 2015-2022 runtime libraries"
LangString DESC_VCREDIST ${LANG_POLISH} "Biblioteki uruchomieniowe Microsoft Visual C++ 2015-2022"
LangString DESC_VCREDIST ${LANG_FRENCH} "Librairies runtime Microsoft Visual C++ 2015-2022 "
LangString DESC_VCREDIST ${LANG_GERMAN} "Microsoft Visual C++ 2015-2022 Laufzeitbibliothek"

LangString SECTION_DESKTOP_SHORTCUT ${LANG_ENGLISH} "Create desktop shortcut"
LangString SECTION_DESKTOP_SHORTCUT ${LANG_POLISH} "Utwórz skrót na pulpicie"
LangString SECTION_DESKTOP_SHORTCUT ${LANG_FRENCH} "Créer un raccourci sur le bureau"
LangString SECTION_DESKTOP_SHORTCUT ${LANG_GERMAN} "Erstelle Desktop Verknüpfung"

LangString DESC_DESKTOP_SHORTCUT ${LANG_ENGLISH} "Creates a shortcut to Winpinator on Desktop"
LangString DESC_DESKTOP_SHORTCUT ${LANG_POLISH} "Tworzy skrót do Winpinatora na pulpicie"
LangString DESC_DESKTOP_SHORTCUT ${LANG_FRENCH} "Crée un raccourci sur le bureau"
LangString DESC_DESKTOP_SHORTCUT ${LANG_GERMAN} "Erstellt eine Verknüpfung zu Winpinator auf dem Desktop"

LangString SECTION_INTEGRATE ${LANG_ENGLISH} "Explorer integration"
LangString SECTION_INTEGRATE ${LANG_POLISH} "Integracja z Eksploratorem"
LangString SECTION_INTEGRATE ${LANG_FRENCH} "Intégration à l'explorateur"
LangString SECTION_INTEGRATE ${LANG_GERMAN} "Im Explorer einbinden"

LangString DESC_INTEGRATE ${LANG_ENGLISH} "Insert shortcut to Winpinator into 'Send to' menu"
LangString DESC_INTEGRATE ${LANG_POLISH} 'Wstaw skrót do Winpinatora do menu "Wyślij do"'
LangString DESC_INTEGRATE ${LANG_FRENCH} "Insérer un raccourci vers Winpinator dans le menu 'Envoyer vers'."
LangString DESC_INTEGRATE ${LANG_GERMAN} "Erstellt eine Verknüpfung zu Winpinator im 'Senden an'-Menü"

LangString UNDESC_WINPINATOR ${LANG_ENGLISH} "Uninstall Winpinator but don't remove any data"
LangString UNDESC_WINPINATOR ${LANG_POLISH} "Odinstaluj Winpinatora, ale pozostaw wszelkie dane"
LangString UNDESC_WINPINATOR ${LANG_FRENCH} "Désinstaller Winpinator mais ne pas supprimer les données"
LangString UNDESC_WINPINATOR ${LANG_GERMAN} "Deinstalliere Winpinator ohne Daten zu löschen"

LangString UNSECTION_USERDATA ${LANG_ENGLISH} "Remove user data"
LangString UNSECTION_USERDATA ${LANG_POLISH} "Usuń dane użytkownika"
LangString UNSECTION_USERDATA ${LANG_FRENCH} "Supprimer les données de l'utilisateur"
LangString UNSECTION_USERDATA ${LANG_GERMAN} "Lösche Winpinator-Benutzerdaten"

LangString UNDESC_USERDATA ${LANG_ENGLISH} "Additionally wipe all user data from computer"
LangString UNDESC_USERDATA ${LANG_POLISH} "Dodatkowo usuń wszystkie dane użytkownika z komputera"
LangString UNDESC_USERDATA ${LANG_FRENCH} "Supprimez également toutes les données de l'utilisateur de l'ordinateur."
LangString UNDESC_USERDATA ${LANG_GERMAN} "Lösche zusätzlich alle Winpinator-Benutzerdaten vom Computer"

LangString DOWNLOADING ${LANG_ENGLISH} "Downloading"
LangString DOWNLOADING ${LANG_POLISH} "Pobieranie"
LangString DOWNLOADING ${LANG_FRENCH} "En cours de téléchargement"
LangString DOWNLOADING ${LANG_GERMAN} "Herunterladen"

LangString DOWNLOAD_FINISHED ${LANG_ENGLISH} "Download finished."
LangString DOWNLOAD_FINISHED ${LANG_POLISH} "Pobieranie ukończone."
LangString DOWNLOAD_FINISHED ${LANG_FRENCH} "Téléchargement terminé."
LangString DOWNLOAD_FINISHED ${LANG_GERMAN} "Herunterladen beendet."

LangString VCREDIST_FAILED ${LANG_ENGLISH} "Can't download Microsoft Visual C++ runtime libraries. Winpinator won't work. Download"
LangString VCREDIST_FAILED2 ${LANG_ENGLISH} "and install them manually."
LangString VCREDIST_FAILED ${LANG_POLISH} "Nie można pobrać bibliotek uruchomieniowych Microsoft Visual C++. Winpinator nie będzie działał. Pobierz"
LangString VCREDIST_FAILED2 ${LANG_POLISH} "i zainstaluj je manualnie."
LangString VCREDIST_FAILED ${LANG_FRENCH} "Impossible de télécharger les librairies d'exécution de Microsoft Visual C++. Winpinator ne fonctionne pas. Téléchargez"
LangString VCREDIST_FAILED2 ${LANG_FRENCH} "et installez-les manuellement."
LangString VCREDIST_FAILED ${LANG_GERMAN} "Die Microsoft Visual C++ 2022 Laufzeitbibliothek kann nicht heruntergeladen werden. Winpinator wird nicht funktionieren. Lade"
LangString VCREDIST_FAILED2 ${LANG_GERMAN} "manuell herunter und installiere sie."

LangString DIRECTORY_NOT_EMPTY ${LANG_ENGLISH} "Installation directory is not empty. Its content is to be entirely removed. Are you sure you want to continue?"
LangString DIRECTORY_NOT_EMPTY ${LANG_POLISH} "Folder instalacji nie jest pusty. Instalacja spowoduje usunięcie całej jego zawartości. Czy chcesz kontynuować?"
LangString DIRECTORY_NOT_EMPTY ${LANG_FRENCH} "Le répertoire d'installation n'est pas vide. Son contenu doit être entièrement supprimé. Vous êtes sûr de vouloir continuer ?"
LangString DIRECTORY_NOT_EMPTY ${LANG_GERMAN} "Der Installationsordner ist nicht leer. Sein ganzer Inhalt wird gelöscht. Bist Du sicher, dass Du fortfahren willst?"

LangString ABORT_WRONG_DIR ${LANG_ENGLISH} "Can't install in this directory."
LangString ABORT_WRONG_DIR ${LANG_POLISH} "Nie można zainstalować w tym folderze."
LangString ABORT_WRONG_DIR ${LANG_FRENCH} "Impossible d'installer dans ce répertoire."
LangString ABORT_WRONG_DIR ${LANG_GERMAN} "Kann nicht in diesen Ordner installieren."

LangString ADD_REMOVE ${LANG_ENGLISH} "Add information to 'Add or remove programs' window"
LangString ADD_REMOVE ${LANG_POLISH} 'Dodaj informacje do okna "Dodaj lub usuń programy"'
LangString ADD_REMOVE ${LANG_FRENCH} "Ajouter les informations à la fenêtre 'Ajouter ou supprimer des programmes'."
LangString ADD_REMOVE ${LANG_GERMAN} "Aktualisiere 'Programme hinzufügen oder entfernen' Fenster"

LangString STARTMENU ${LANG_ENGLISH} "Optionally create Start Menu shortcut"
LangString STARTMENU ${LANG_POLISH} "Opcjonalnie utwórz skrót w Menu Start"
LangString STARTMENU ${LANG_FRENCH} "Création facultative d'un raccourci dans le menu Démarrer"
LangString STARTMENU ${LANG_GERMAN} "Erstelle optionale Start Menü Verknüpfung"

LangString UNNAME ${LANG_ENGLISH} "Uninstall ${SOFTWARE_NAME}"
LangString UNNAME ${LANG_POLISH} "Odinstaluj ${SOFTWARE_NAME}"
LangString UNNAME ${LANG_FRENCH} "Déinstaller ${SOFTWARE_NAME}"
LangString UNNAME ${LANG_GERMAN} "Deinstalliere ${SOFTWARE_NAME}"

LangString CREATING_DESKTOP_SHORTCUT ${LANG_ENGLISH} "Creating Desktop shortcut"
LangString CREATING_DESKTOP_SHORTCUT ${LANG_POLISH} "Tworzenie skrótu na Pulpicie"
LangString CREATING_DESKTOP_SHORTCUT ${LANG_FRENCH} "Création du raccourci bureau"
LangString CREATING_DESKTOP_SHORTCUT ${LANG_GERMAN} "Erstelle Desktop Verknüpfung"

LangString INTEGRATING ${LANG_ENGLISH} "Adding Winpinator to 'Send to' list"
LangString INTEGRATING ${LANG_POLISH} 'Dodawanie Winpinatora do listy "Wyślij do"'
LangString INTEGRATING ${LANG_FRENCH} "Ajout de Winpinator à la liste 'Envoyer vers'"
LangString INTEGRATING ${LANG_GERMAN} "Füge Winpinator zur 'Senden an' Liste"

LangString UN_WRONG_DIR ${LANG_ENGLISH} "Winpinator does not appear to be installed in this directory. Are you sure you want to continue?"
LangString UN_WRONG_DIR ${LANG_POLISH} "Wygląda na to, że Winpinator nie jest zainstalowany w tym folderze. Czy na pewno chcesz kontynuować?"
LangString UN_WRONG_DIR ${LANG_FRENCH} "Winpinator ne semble pas être installé dans ce répertoire. Êtes-vous sûr de vouloir continuer ?"
LangString UN_WRONG_DIR ${LANG_GERMAN} "Winpinator scheint nicht in diesem Ordner installiert zu sein. Bist Du sicher, dass Du fortfahren willst?"

LangString UNABORT ${LANG_ENGLISH} "Uninstallation aborted."
LangString UNABORT ${LANG_POLISH} "Deinstalacja przerwana."
LangString UNABORT ${LANG_FRENCH} "Déinstallation interrompue."
LangString UNABORT ${LANG_GERMAN} "Deinstallation abgebrochen."

LangString UNWRONG_INSTDIR ${LANG_ENGLISH} "We're not uninstalling from the original install location. Do not remove entry from 'Add or remove programs'"
LangString UNWRONG_INSTDIR ${LANG_POLISH} 'Winpinator nie jest odinstalowywany z oryginalnej ścieżki instalacji. Nie usuwaj wpisu z listy "Dodaj lub usuń programy"'
LangString UNWRONG_INSTDIR ${LANG_FRENCH} "Nous ne désinstallons pas à partir de l'emplacement d'installation d'origine. Ne pas supprimer l'entrée de 'Ajout ou suppression de programmes'."
LangString UNWRONG_INSTDIR ${LANG_GERMAN} "Deinstallation erfolgt nicht vom ursprünglichen Installationsort. Entferne den Eintrag im 'Programme hinzufügen oder entfernen' nicht."


Section "!$(SECTION_WINPINATOR)" WINPINATOR
  SectionIn RO
  SetShellVarContext all

  IfFileExists "$INSTDIR\*.*" directory_exists
    CreateDirectory $INSTDIR

  directory_exists:
  Push $INSTDIR
  Call isEmptyDir
  Pop $0

  ${If} $0 == "1"
    Goto install
  ${Else}
    IfFileExists "$INSTDIR\Winpinator.exe" install
    MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(DIRECTORY_NOT_EMPTY)" /SD IDNO IDYES continue IDNO abort
      continue:
        RMDir /r $INSTDIR
        CreateDirectory $INSTDIR
        Goto install
      abort:
        Abort "$(ABORT_WRONG_DIR)"
  ${EndIf}

  install:
  SetOutPath $INSTDIR
  File "${RELEASE_FOLDER}\Winpinator.exe"
  File "${RELEASE_FOLDER}\Languages.xml"
  File "${RELEASE_FOLDER}\LICENSE.html"
  File "${RELEASE_FOLDER}\LICENSE.rtf"
  File "${RELEASE_FOLDER}\RELEASE NOTES.txt"
  File "${RELEASE_FOLDER}\*.dll"

  SetOutPath $INSTDIR\flags
  File "${RELEASE_FOLDER}\flags\*"

  SetOutPath $INSTDIR\locales
  File /r "${RELEASE_FOLDER}\locales\*"
SectionEnd

Section -AddOrRemovePrograms
  DetailPrint "$(ADD_REMOVE)"
  SetShellVarContext all
	
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"DisplayName" "${SOFTWARE_NAME} (${ARCH})"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"UninstallString" "$\"$INSTDIR\uninstall.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"InstallLocation" "$\"$INSTDIR$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"DisplayIcon" "$\"$INSTDIR\${SOFTWARE_NAME}.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"Publisher" "Łukasz Świszcz"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"DisplayVersion" "${Version1}.${Version2}.${Version3}"
	WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"VersionMajor" "${Version1}"
	WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"VersionMinor" "${Version2}"
	WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"NoModify" "1"
	WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"NoRepair" "1"

	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	IntFmt $0 "0x%08X" $0
	
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" \
		"EstimatedSize" "$0"
SectionEnd

Section -StartMenu
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  DetailPrint "$(STARTMENU)"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	CreateDirectory "$SMPrograms\$StartMenuDir"
	CreateShortCut "$SMPROGRAMS\$StartMenuDir\${SOFTWARE_NAME} (${ARCH}).lnk" "$INSTDIR\Winpinator.exe"
	CreateShortCut "$SMPROGRAMS\$StartMenuDir\$(UNNAME) (${ARCH}).lnk" "$INSTDIR\uninstall.exe"
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -Uninstaller
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  WriteUninstaller "uninstall.exe"

  WriteRegStr HKLM ${REG_KEY} "InstallLocation_${ARCH}" $INSTDIR

  ${RefreshShellIcons}
SectionEnd

Section "$(SECTION_VCREDIST)" VCREDIST
  SectionIn RO
  SetShellVarContext all

  SetOutPath "$INSTDIR\redist"
  AddSize 13800

!if ${ARCH} == "x64"
  DetailPrint "$(DOWNLOADING) https://aka.ms/vs/17/release/vc_redist.x64.exe"
  inetc::get /silent "https://aka.ms/vs/17/release/vc_redist.x64.exe" "$INSTDIR\redist\vc_redist.x64.exe" /end
  Pop $0

  ${If} $0 == "OK"
    DetailPrint "$(DOWNLOAD_FINISHED)"
    ExecWait "$INSTDIR\redist\vc_redist.x64.exe /q /norestart"
  ${Else}
    DetailPrint "$(VCREDIST_FAILED) https://aka.ms/vs/17/release/vc_redist.x64.exe $(VCREDIST_FAILED2)"
    MessageBox MB_ICONEXCLAMATION "$(VCREDIST_FAILED) https://aka.ms/vs/17/release/vc_redist.x64.exe $(VCREDIST_FAILED2)"
  ${EndIf}
!else
  DetailPrint "$(DOWNLOADING) https://aka.ms/vs/17/release/vc_redist.x86.exe"
  inetc::get /silent "https://aka.ms/vs/17/release/vc_redist.x86.exe" "$INSTDIR\redist\vc_redist.x86.exe" /end
  Pop $0

  ${If} $0 == "OK"
    DetailPrint "$(DOWNLOAD_FINISHED)"
    ExecWait "$INSTDIR\redist\vc_redist.x86.exe /q /norestart"
  ${Else}
    DetailPrint "$(VCREDIST_FAILED) https://aka.ms/vs/17/release/vc_redist.x86.exe $(VCREDIST_FAILED2)"
    MessageBox MB_ICONEXCLAMATION "$(VCREDIST_FAILED) https://aka.ms/vs/17/release/vc_redist.x86.exe $(VCREDIST_FAILED2)"
  ${EndIf}
!endif
SectionEnd

Section "$(SECTION_DESKTOP_SHORTCUT)" DESKTOP_SHORTCUT
  SetShellVarContext all
  DetailPrint "$(CREATING_DESKTOP_SHORTCUT)"
  CreateShortCut "$DESKTOP\${SOFTWARE_NAME}.lnk" "$INSTDIR\${SOFTWARE_NAME}.exe"
SectionEnd

Section "$(SECTION_INTEGRATE)" INTEGRATE
  SetShellVarContext all
  DetailPrint "$(INTEGRATING)"
  CreateShortCut "$APPDATA\Microsoft\Windows\SendTo\Winpinator.lnk" "$INSTDIR\${SOFTWARE_NAME}.exe"
SectionEnd

# Uninstaller sections

Section "!un.$(SECTION_WINPINATOR)" UNWINPINATOR
  SetShellVarContext all
  SectionIn RO

  IfFileExists "$INSTDIR\${SOFTWARE_NAME}.exe" uninstall
    MessageBox MB_ICONEXCLAMATION|MB_YESNO "$(UN_WRONG_DIR)" IDYES uninstall
    Abort "$(UNABORT)"

  uninstall:
  Delete "$INSTDIR\Winpinator.exe"
  Delete "$INSTDIR\Languages.xml"
  Delete "$INSTDIR\LICENSE.html"
  Delete "$INSTDIR\LICENSE.rtf"
  Delete "$INSTDIR\RELEASE NOTES.txt"
  Delete "$INSTDIR\*.dll"

  RMDir /r "$INSTDIR\flags"
  RMDir /r "$INSTDIR\locales"
  RMDIR /r "$INSTDIR\redist"

  Delete "$INSTDIR\uninstall.exe"

  RMDir $INSTDIR

  DeleteRegKey HKLM ${REG_KEY}
SectionEnd

Section -un.StartMenu
  SetShellVarContext all
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuDir
  Delete "$SMPROGRAMS\$StartMenuDir\*(${ARCH}).lnk"
  RMDir "$SMPROGRAMS\$StartMenuDir"
SectionEnd

Section -un.Integrate
  SetShellVarContext all
  Delete "$APPDATA\Microsoft\Windows\SendTo\Winpinator.lnk"
SectionEnd

Section -un.AddOrRemovePrograms
  SetShellVarContext all
  # Ensure that we're uninstalling from the same INSTDIR
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}" "InstallLocation"

  ${If} $0 == "$\"$INSTDIR$\""
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFTWARE_NAME}_${ARCH}"
  ${Else}
    DetailPrint "$(UNWRONG_INSTDIR)"
  ${EndIf}
SectionEnd

Section "un.$(UNSECTION_USERDATA)" UNUSERDATA
  SetShellVarContext current
  # Delete user config
  DeleteRegKey HKCU "Software\Winpinator"

  # Delete APPDATA folder
  RMDIR /r "$APPDATA\Winpinator"
SectionEnd


# View language selector on start
Function .onInit
    !insertmacro MUI_LANGDLL_DISPLAY

    # Check if appropriate MSVCP version is installed
    ReadRegDword $ins64_1 HKLM "SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
    ReadRegDword $ins64_1_maj HKLM "SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Major"
    ReadRegDword $ins64_1_min HKLM "SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Minor"

    ReadRegDword $ins64_2 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
    ReadRegDword $ins64_2_maj HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Major"
    ReadRegDword $ins64_2_min HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Minor"


    ReadRegDword $ins86_1 HKLM "SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" "Installed"
    ReadRegDword $ins86_1_maj HKLM "SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" "Major"
    ReadRegDword $ins86_1_min HKLM "SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" "Minor"

    ReadRegDword $ins86_2 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" "Installed"
    ReadRegDword $ins86_2_maj HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" "Major"
    ReadRegDword $ins86_2_min HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" "Minor"

!if ${ARCH} == "x64"
    ${If} $ins64_1 == "1"
    ${AndIf} $ins64_1_maj == "14"
    ${AndIf} $ins64_1_min >= "30"
      !insertmacro UnselectSection ${VCREDIST}
    ${EndIf}

    ${If} $ins64_2 == "1"
    ${AndIf} $ins64_2_maj == "14"
    ${AndIf} $ins64_2_min >= "30"
      !insertmacro UnselectSection ${VCREDIST}
    ${EndIf}
!else
    ${If} $ins86_1 == "1"
    ${AndIf} $ins86_1_maj == "14"
    ${AndIf} $ins86_1_min >= "30"
      !insertmacro UnselectSection ${VCREDIST}
    ${EndIf}
    
    ${If} $ins86_2 == "1"
    ${AndIf} $ins86_2_maj == "14"
    ${AndIf} $ins86_2_min >= "30"
      !insertmacro UnselectSection ${VCREDIST}
    ${EndIf}
!endif

  !insertmacro UnselectSection ${DESKTOP_SHORTCUT}
FunctionEnd

Function un.onInit
  !insertmacro MUI_UNGETLANGUAGE

  !insertmacro UnselectSection ${UNUSERDATA}
FunctionEnd

# Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${WINPINATOR} $(DESC_WINPINATOR)
  !insertmacro MUI_DESCRIPTION_TEXT ${VCREDIST} $(DESC_VCREDIST)
  !insertmacro MUI_DESCRIPTION_TEXT ${DESKTOP_SHORTCUT} $(DESC_DESKTOP_SHORTCUT)
  !insertmacro MUI_DESCRIPTION_TEXT ${INTEGRATE} $(DESC_INTEGRATE)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

!insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${UNWINPINATOR} $(UNDESC_WINPINATOR)
  !insertmacro MUI_DESCRIPTION_TEXT ${UNUSERDATA} $(UNDESC_USERDATA)
!insertmacro MUI_UNFUNCTION_DESCRIPTION_END


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
