Unicode True
RequestExecutionLevel user

!include "MUI2.nsh"

!define APP_NAME "JesterStep"
!define APP_EXE "JesterStep.exe"
!define APP_VERSION "1.3.0"
!define APP_PUBLISHER "Jesterace"

Name "${APP_NAME}"
OutFile "..\dist\JesterStep-Setup.exe"
InstallDir "$LOCALAPPDATA\JesterStep"
InstallDirRegKey HKCU "Software\JesterStep" "InstallDir"

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch JesterStep"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "JesterStep" SEC_MAIN
    SectionIn RO

    SetOutPath "$INSTDIR"

    File "..\build\Release\JesterStep.exe"

    WriteRegStr HKCU "Software\JesterStep" "InstallDir" "$INSTDIR"

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    CreateDirectory "$SMPROGRAMS\JesterStep"
    CreateShortcut "$SMPROGRAMS\JesterStep\JesterStep.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortcut "$SMPROGRAMS\JesterStep\Uninstall JesterStep.lnk" "$INSTDIR\Uninstall.exe"

    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep" "DisplayName" "${APP_NAME}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep" "DisplayVersion" "${APP_VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep" "Publisher" "${APP_PUBLISHER}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep" "DisplayIcon" "$INSTDIR\${APP_EXE}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep" "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep" "NoRepair" 1
SectionEnd

Section "Desktop shortcut" SEC_DESKTOP
    CreateShortcut "$DESKTOP\JesterStep.lnk" "$INSTDIR\${APP_EXE}"
SectionEnd

Section "Uninstall"
    Delete "$SMPROGRAMS\JesterStep\JesterStep.lnk"
    Delete "$SMPROGRAMS\JesterStep\Uninstall JesterStep.lnk"
    RMDir "$SMPROGRAMS\JesterStep"

    Delete "$DESKTOP\JesterStep.lnk"

    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JesterStep"
    DeleteRegKey /ifempty HKCU "Software\JesterStep"

    ; Intentionally leave %APPDATA%\JesterStep alone.
    ; That is where user settings.ini lives.
SectionEnd