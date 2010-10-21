!include x64.nsh
!ifdef WIN64
!define PROGNAME "${APPNAME} 64-bit Client"
OutFile ${BLDDIR}\${APPNAME}64.exe
InstallDir $PROGRAMFILES64\${APPNAME}-${VERSION}-${BUILD}
!else
!define PROGNAME "${APPNAME} Client"
OutFile ${BLDDIR}\${APPNAME}.exe
InstallDir $PROGRAMFILES\${APPNAME}-${VERSION}-${BUILD}
!endif
Name "${PROGNAME}"

SetCompressor bzip2

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "${APPNAME}-${VERSION}-${BUILD} (required)"

!ifdef WIN64
	${If} ${RunningX64}
	${DisableX64FSRedirection}
	${Endif}
!endif

	SectionIn RO
	SetOutPath $INSTDIR\doc
	File "doc\*.gif"
	File "doc\*.png"
	File "doc\*.css"
	File "doc\*.html"
	SetOutPath $INSTDIR
	File "${BLDDIR}\bin\vglclient.exe"
	File "${BLDDIR}\bin\vglconnect.bat"
	File "${BLDDIR}\bin\xauth.exe"
	File "${BLDDIR}\bin\putty.exe"
	File "${BLDDIR}\bin\plink.exe"
	File "${BLDDIR}\bin\tcbench.exe"
	File "${BLDDIR}\bin\nettest.exe"
	File "/oname=doc\LGPL.txt" "LGPL.txt"
	File "/oname=doc\LICENSE.txt" "LICENSE.txt"
	File "/oname=doc\LICENSE-FLTK.txt" "fltk\COPYING"
	File "/oname=doc\LICENSE-PuTTY.txt" "putty\LICENCE"
	File "/oname=doc\LICENSE-xauth.txt" "x11windows\xauth.license"
	File "/oname=doc\ChangeLog.txt" "ChangeLog.txt"

	WriteRegStr HKLM "SOFTWARE\${APPNAME}-${VERSION}-${BUILD}" "Install_Dir" "$INSTDIR"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "DisplayName" "${PROGNAME} v${VERSION} (${BUILD})"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "NoRepair" 1
	WriteUninstaller "uninstall.exe"
SectionEnd

Section "Start Menu Shortcuts (required)"

	SetShellVarContext all
	CreateDirectory "$SMPROGRAMS\${PROGNAME} v${VERSION} (${BUILD})"
	CreateShortCut "$SMPROGRAMS\${PROGNAME} v${VERSION} (${BUILD})\Uninstall ${APPNAME} Client.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	CreateShortCut "$SMPROGRAMS\${PROGNAME} v${VERSION} (${BUILD})\${APPNAME} User's Guide.lnk" "$INSTDIR\doc\index.html"

SectionEnd

Section "Uninstall"

!ifdef WIN64
	${If} ${RunningX64}
	${DisableX64FSRedirection}
	${Endif}
!endif

	SetShellVarContext all
	ExecWait "$INSTDIR\vglclient.exe -killall"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME}-${VERSION}-${BUILD}"

	Delete $INSTDIR\vglclient.exe
	Delete $INSTDIR\vglconnect.bat
	Delete $INSTDIR\xauth.exe
	Delete $INSTDIR\putty.exe
	Delete $INSTDIR\plink.exe
	Delete $INSTDIR\uninstall.exe
	Delete $INSTDIR\stunnel.rnd
	Delete $INSTDIR\tcbench.exe
	Delete $INSTDIR\nettest.exe
	RMDir /r $INSTDIR\doc

	Delete "$SMPROGRAMS\${PROGNAME} v${VERSION} (${BUILD})\*.*"
	RMDir "$SMPROGRAMS\${PROGNAME} v${VERSION} (${BUILD})"
	RMDir "$INSTDIR"

SectionEnd
