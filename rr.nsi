Name ${APPNAME}
OutFile ${APPNAME}.exe
InstallDir $PROGRAMFILES\${APPNAME}-${VERSION}-${BUILD}

SetCompressor bzip2

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "${APPNAME}-${VERSION}-${BUILD} (required)"
	SectionIn RO
	SetOutPath $INSTDIR
	File "${EDIR}\rrxclient.exe"
	File "${EDIR}\hpjpeg.dll"
	File "rr\newrrcert.bat"
	File "rr\rrcert.cnf"
	File "${EDIR}\openssl.exe"

	WriteRegStr HKLM "SOFTWARE\${APPNAME}-${VERSION}-${BUILD}" "Install_Dir" "$INSTDIR"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "DisplayName" "${APPNAME} Client v${VERSION} (${BUILD})"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}" "NoRepair" 1
	WriteUninstaller "uninstall.exe"
SectionEnd

Section "Start Menu Shortcuts (required)"

	CreateDirectory "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Uninstall ${APPNAME} Client.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Install ${APPNAME} Client as a Service.lnk" "$INSTDIR\rrxclient.exe" "-install"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Remove ${APPNAME} Client Service.lnk" "$INSTDIR\rrxclient.exe" "-remove"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Install ${APPNAME} Secure Client as a Service.lnk" "$INSTDIR\rrxclient.exe" "-s -install"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Remove ${APPNAME} Secure Client Service.lnk" "$INSTDIR\rrxclient.exe" "-s -remove"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Start ${APPNAME} Client.lnk" "$INSTDIR\rrxclient.exe"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Start ${APPNAME} Secure Client.lnk" "$INSTDIR\rrxclient.exe" "-s"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Create Client SSL Certificate (this user only).lnk" "$INSTDIR\newrrcert.bat"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\Create Client SSL Certificate (all users).lnk" "$INSTDIR\newrrcert.bat" "root"

SectionEnd

Section "Uninstall"

	ExecWait "$INSTDIR\rrxclient.exe -q -remove"
	ExecWait "$INSTDIR\rrxclient.exe -q -s -remove"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${VERSION}-${BUILD}"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME}-${VERSION}-${BUILD}"

	Delete $INSTDIR\rrxclient.exe
	Delete $INSTDIR\hpjpeg.dll
	Delete $INSTDIR\newrrcert.bat
	Delete $INSTDIR\rrcert.cnf
	Delete $INSTDIR\openssl.exe
	Delete $INSTDIR\uninstall.exe
	Delete $INSTDIR\stunnel.rnd
	Delete $INSTDIR\rrcert.pem

	Delete "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})\*.*"
	RMDir "$SMPROGRAMS\${APPNAME} Client v${VERSION} (${BUILD})"
	RMDir "$INSTDIR"

SectionEnd
