Name ${APPNAME}
OutFile ${APPNAME}.exe
InstallDir $PROGRAMFILES\${APPNAME}-${MAJVER}.${MINVER}

SetCompressor bzip2

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "${APPNAME}-${MAJVER}.${MINVER} (required)"
	SectionIn RO
	SetOutPath $INSTDIR
	File "${EDIR}\rrxclient.exe"
	File "${EDIR}\hpjpeg.dll"
	File "rr\newrrcert.bat"
	File "rr\rrcert.cnf"
	File "${EDIR}\openssl.exe"
	File "${EDIR}\pthreadVC.dll"

	WriteRegStr HKLM "SOFTWARE\${APPNAME} Client (${MAJVER}.${MINVER})" "Install_Dir" "$INSTDIR"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} Client (${MAJVER}.${MINVER})" "DisplayName" "${APPNAME} Client (${MAJVER}.${MINVER})"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} Client (${MAJVER}.${MINVER})" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} Client (${MAJVER}.${MINVER})" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} Client (${MAJVER}.${MINVER})" "NoRepair" 1
	WriteUninstaller "uninstall.exe"
SectionEnd

Section "Start Menu Shortcuts (required)"

	CreateDirectory "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})\Uninstall ${APPNAME} Client.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})\Install ${APPNAME} Client as a Service.lnk" "$INSTDIR\rrxclient.exe" "-install"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})\Remove ${APPNAME} Client Service.lnk" "$INSTDIR\rrxclient.exe" "-remove"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})\Install ${APPNAME} Secure Client as a Service.lnk" "$INSTDIR\rrxclient.exe" "-s -install"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})\Remove ${APPNAME} Secure Client Service.lnk" "$INSTDIR\rrxclient.exe" "-s -remove"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})\Start ${APPNAME} Client.lnk" "$INSTDIR\rrxclient.exe"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})\Create Client SSL Certificate (this user only).lnk" "$INSTDIR\newrrcert.bat"
	CreateShortCut "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})\Create Client SSL Certificate (all users).lnk" "$INSTDIR\newrrcert.bat" "root"

SectionEnd

Section "Uninstall"

	ExecWait "$INSTDIR\rrxclient.exe -q -remove"
	ExecWait "$INSTDIR\rrxclient.exe -q -s -remove"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} Client (${MAJVER}.${MINVER})"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME} Client (${MAJVER}.${MINVER})"

	Delete $INSTDIR\rrxclient.exe
	Delete $INSTDIR\hpjpeg.dll
	Delete $INSTDIR\newrrcert.bat
	Delete $INSTDIR\rrcert.cnf
	Delete $INSTDIR\openssl.exe
	Delete $INSTDIR\pthreadVC.dll
	Delete $INSTDIR\uninstall.exe
	Delete $INSTDIR\stunnel.rnd
	Delete $INSTDIR\rrcert.pem

	Delete "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})\*.*"
	RMDir "$SMPROGRAMS\${APPNAME} Client (${MAJVER}.${MINVER})"
	RMDir "$INSTDIR"

SectionEnd
