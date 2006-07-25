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
	SetOutPath $INSTDIR\doc
	File "doc\*.gif"
	File "doc\*.png"
	File "doc\*.html"
	SetOutPath $INSTDIR
	File "${BLDDIR}\bin\vglclient.exe"
	File "${BLDDIR}\bin\turbojpeg.dll"
	File "${BLDDIR}\bin\tcbench.exe"
	File "${BLDDIR}\bin\nettest.exe"
	File "${MINGWDIR}\bin\mingwm10.dll"
	File "$%systemroot%\system32\libeay32.dll"
	File "$%systemroot%\system32\ssleay32.dll"
	File "/oname=doc\LGPL.txt" "LGPL.txt"
	File "/oname=doc\LICENSE.txt" "LICENSE.txt"
	File "/oname=doc\LICENSE-OpenSSL.txt" "LICENSE-OpenSSL.txt"

	WriteRegStr HKLM "SOFTWARE\${APPNAME}-${VERSION}-${BUILD}" "Install_Dir" "$INSTDIR"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "DisplayName" "${APPNAME} Client v${VERSION} (${BUILD})"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "NoRepair" 1
	WriteUninstaller "uninstall.exe"
SectionEnd

Section "Start Menu Shortcuts (required)"

	SetShellVarContext all
	CreateDirectory "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Uninstall ${APPNAME} Client.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Install ${APPNAME} Client as a Service.lnk" "$INSTDIR\vglclient.exe" "-install"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Remove ${APPNAME} Client Service.lnk" "$INSTDIR\vglclient.exe" "-remove"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Start ${APPNAME} Client.lnk" "$INSTDIR\vglclient.exe"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\${APPNAME} User's Guide.lnk" "$INSTDIR\doc\index.html"

SectionEnd

Section "Uninstall"

	SetShellVarContext all
	ExecWait "$INSTDIR\vglclient.exe -q -remove"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME}-${VERSION}-${BUILD}"

	Delete $INSTDIR\vglclient.exe
	Delete $INSTDIR\turbojpeg.dll
	Delete $INSTDIR\uninstall.exe
	Delete $INSTDIR\stunnel.rnd
	Delete $INSTDIR\tcbench.exe
	Delete $INSTDIR\nettest.exe
	Delete $INSTDIR\mingwm10.dll
	Delete $INSTDIR\libeay32.dll
	Delete $INSTDIR\ssleay32.dll
	RMDir /r $INSTDIR\doc

	Delete "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\*.*"
	RMDir "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})"
	RMDir "$INSTDIR"

SectionEnd
