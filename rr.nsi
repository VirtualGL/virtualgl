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
	File "${BLDDIR}\bin\vglclient.exe"
	File "${BLDDIR}\bin\hpjpeg.dll"
	File /oname=newvglcert.bat "rr\newrrcert.bat"
	File /oname=vglcert.cnf "rr\rrcert.cnf"
	File "${BLDDIR}\bin\openssl.exe"
	File "${BLDDIR}\bin\tcbench.exe"
	File "${BLDDIR}\bin\nettest.exe"
	File "util\nettest.pem"

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
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Install ${APPNAME} Secure Client as a Service.lnk" "$INSTDIR\vglclient.exe" "-s -install"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Remove ${APPNAME} Secure Client Service.lnk" "$INSTDIR\vglclient.exe" "-s -remove"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Start ${APPNAME} Client.lnk" "$INSTDIR\vglclient.exe"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Start ${APPNAME} Secure Client.lnk" "$INSTDIR\vglclient.exe" "-s"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Create Client SSL Certificate (this user only).lnk" "$INSTDIR\newvglcert.bat"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Create Client SSL Certificate (all users).lnk" "$INSTDIR\newvglcert.bat" "root"

SectionEnd

Section "Uninstall"

	SetShellVarContext all
	ExecWait "$INSTDIR\vglclient.exe -q -remove"
	ExecWait "$INSTDIR\vglclient.exe -q -s -remove"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME}-${VERSION}-${BUILD}"

	Delete $INSTDIR\vglclient.exe
	Delete $INSTDIR\hpjpeg.dll
	Delete $INSTDIR\newvglcert.bat
	Delete $INSTDIR\vglcert.cnf
	Delete $INSTDIR\openssl.exe
	Delete $INSTDIR\uninstall.exe
	Delete $INSTDIR\stunnel.rnd
	Delete $INSTDIR\vglcert.pem
	Delete $INSTDIR\tcbench.exe
	Delete $INSTDIR\nettest.exe
	Delete $INSTDIR\nettest.pem

	Delete "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\*.*"
	RMDir "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})"
	RMDir "$INSTDIR"

SectionEnd
