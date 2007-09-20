@echo off
setlocal EnableDelayedExpansion

for %%i in ("ssh.exe") do set SSHPATH=%%~$PATH:i
if not "%SSHPATH%"=="" goto sshfound
echo.
echo [VGL] ERROR: Could not find an SSh client.  vglconnect requires an OpenSSh-
echo [VGL]    compatible version of SSh, such as Cygwin or SSHWindows.  The
echo [VGL]    directory containing ssh.exe must be in your PATH.
goto done

:sshfound

set VGLTUNNEL=0
set X11TUNNEL=1
set FORCE=0

:getargs
if "%1"=="" goto argsdone
if "%1"=="-display" (
	set DISPLAY=%2
	shift /1 & shift /1 & goto getargs
) else (
	if "%1"=="-force" (
		set FORCE=1
		shift /1 & goto getargs
	) else (
		if "%1"=="-s" (
			set VGLTUNNEL=1
			shift /1 & goto getargs
		) else (
			if "%1"=="-x" (
				set X11TUNNEL=0
				shift /1 & goto getargs
			) else (
				goto argsdone
			)
		)
	)
)
:argsdone

if "%1"=="" goto usage

if "%DISPLAY%"=="" (
	echo [VGL] ERROR: An X display must be specified, either by using the -display
	echo [VGL]    argument to vglconnect or by setting the DISPLAY environment variable
	goto done
)
if "%DISPLAY:~0,1%"==":" set DISPLAY=localhost%DISPLAY%

set SSHARG=-X
for /f "tokens=1,*" %%i in ('""%SSHPATH%"" -V 2^>^&1') do (
	for /f "delims=_, tokens=1,2,*" %%x in ("%%i") do (
		if /i "%%y" geq "3.8" set SSHARG=-Y
	)
)

set LOGDIR=%USERPROFILE%\.vgl
if not exist "%LOGDIR%" md "%LOGDIR%"
set LOGFILE=%LOGDIR%\vglconnect-%COMPUTERNAME%-%DISPLAY::=%%.log

set VGLARGS=-l "%LOGFILE%" -d %DISPLAY% -detach
if "%FORCE%"=="1" set VGLARGS=%VGLARGS% -force
set VGLCLIENT=%~d0%~p0vglclient.exe
if not exist "%VGLCLIENT%" set VGLCLIENT=vglclient.exe

set VGLCLIENT="%VGLCLIENT%" %VGLARGS%
for /f %%i in ('"%VGLCLIENT%"') do set PORT=%%i

set /a PORTM1=PORT-1
if "%PORT%"=="" (
	echo [VGL] ERROR: vglclient failed to execute.
	goto done
)
if "%PORTM1%"=="-1" (
	echo [VGL] ERROR: vglclient failed to execute.
	goto done
)

if "%VGLTUNNEL%"=="1" goto vgltunnel

if "%X11TUNNEL%"=="1" goto x11tunnel

set XAUTH=%~d0%~p0xauth.exe
if not exist "%XAUTH%" set XAUTH=xauth
:mktemp
if not "%TEMP%"=="" set TMPDIR=%TEMP%
if "%TMPDIR%"=="" if not "%TMP%"=="" set TMPDIR=%TMP%
if "%TMPDIR%"=="" TMPDIR=%SystemRoot%
set XAUTHFILE=%TMPDIR%\vglconnect_%RANDOM%%RANDOM%%RANDOM%.tmp
if exist "%XAUTHFILE%" goto mktemp
"%XAUTH%" -f "%XAUTHFILE%" generate %DISPLAY% . trusted timeout 0
set XAUTHCMD="%XAUTH%" -f "%XAUTHFILE%" list
for /f "tokens=1,2,3,*" %%i in ('"%XAUTHCMD%"') do set XAUTHCOOKIE=%%k
del /q "%XAUTHFILE%"
"%SSHPATH%" -t -x %1 %2 %3 %4 %5 %6 %7 %8 %9 'exec /opt/VirtualGL/bin/vgllogin -x '%DISPLAY%' '%XAUTHCOOKIE%

goto done

:vgltunnel
for /f "usebackq tokens=1,*" %%i in (`""%SSHPATH%"" %1 %2 %3 %4 %5 %6 %7 %8 %9 '/opt/VirtualGL/bin/nettest -findport'`) do set REMOTEPORT=%%i
set /a REMOTEPORTM1=REMOTEPORT-1
if "%REMOTEPORT%"=="" (
	echo [VGL] ERROR: Could not obtain a free port on the server.  Does the server have
	echo [VGL]    VirtualGL 2.1 or later installed?
	goto done
)
if "%REMOTEPORTM1%"=="-1" (
	echo [VGL] ERROR: Could not obtain a free port on the server.  Does the server have
	echo [VGL]    VirtualGL 2.1 or later installed?
	goto done
)
"%SSHPATH%" -t %SSHARG% -R%REMOTEPORT%:localhost:%PORT% %1 %2 %3 %4 %5 %6 %7 %8 %9 '/opt/VirtualGL/bin/vgllogin -s '%REMOTEPORT%

goto done

:x11tunnel
"%SSHPATH%" %SSHARG% %1 %2 %3 %4 %5 %6 %7 %8 %9

goto done

:usage
echo.
echo USAGE: %0
echo        [-display ^<d^>] [-s] [-x] [-force] [user@]hostname
echo        [Additional SSh options]
echo.
echo -display ^<d^> = Local X display to use when drawing VirtualGL's images
echo                (default: read from DISPLAY environment variable)
echo -s = Tunnel VGL image transport over SSh
echo -x = Do not tunnel X11 traffic over SSh
echo -force = Force a new vglclient instance (use with caution)
echo.

:done

endlocal
