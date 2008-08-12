Name ${APPNAME}
OutFile ${BLDDIR}\${APPNAME}.exe
InstallDir $PROGRAMFILES\${APPNAME}-${VERSION}-${BUILD}

SetCompressor bzip2

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "${APPNAME}-${VERSION}-${BUILD} (required)"
	SectionIn RO
	SetOutPath $INSTDIR
	File "${BLDDIR}\bin\faker.dll"
	File "${BLDDIR}\bin\wglspheres.exe"
	File "${BLDDIR}\bin\nettest.exe"
	File "${DETOURSDIR}\bin\detoured.dll"
	File "${DETOURSDIR}\bin\withdll.exe"
	File ".\vglrun.bat"
	File "..\LICENSE.txt"
	File "..\LGPL.txt"
	File "LICENSE-DetoursExpress.txt"
	File "${DETOURSDIR}\README.TXT"
	File "${DETOURSDIR}\REDIST.TXT"

	WriteRegStr HKLM "SOFTWARE\${APPNAME}-${VERSION}-${BUILD}" "Install_Dir" "$INSTDIR"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "DisplayName" "${APPNAME} v${VERSION} (${BUILD})"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "NoRepair" 1
	WriteUninstaller "uninstall.exe"
SectionEnd

Section "Uninstall"

	SetShellVarContext all

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME}-${VERSION}-${BUILD}"

	Delete $INSTDIR\faker.dll
	Delete $INSTDIR\wglspheres.exe
	Delete $INSTDIR\nettest.exe
	Delete $INSTDIR\detoured.dll
	Delete $INSTDIR\withdll.exe
	Delete $INSTDIR\vglrun.bat
	Delete $INSTDIR\LICENSE.txt
	Delete $INSTDIR\LGPL.txt
	Delete $INSTDIR\LICENSE-DetoursExpress.txt
	Delete $INSTDIR\README.TXT
	Delete $INSTDIR\REDIST.TXT
	Delete $INSTDIR\uninstall.exe

	RMDir "$INSTDIR"

SectionEnd
