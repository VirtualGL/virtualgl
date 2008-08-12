@echo off
setlocal EnableDelayedExpansion

set WITHDLL=%~d0%~p0withdll.exe
if not exist "%WITHDLL%" set WITHDLL=withdll.exe
set FAKER=%~d0%~p0faker.dll
if not exist "%FAKER%" set FAKER=faker.dll

:getargs
set ARG=%~1
if "%ARG%"=="" goto argsdone
if /i "%ARG:~0,2%"=="-f" (
	set VGL_FPS=%2
	shift /1 & shift /1 & goto getargs
) else (
	if /i "%ARG:~0,3%"=="-ga" (
		set VGL_GAMMA=%2
		shift /1 & shift /1 & goto getargs
	) else (
		if /i "%ARG:~0,2%"=="-g" (
			set VGL_GAMMA=0
			shift /1 & goto getargs
		) else (
			if /i "%ARG:~0,2%"=="+g" (
				set VGL_GAMMA=1
				shift /1 & goto getargs
			) else (
				if /i "%ARG:~0,3%"=="+pr" (
					set VGL_PROFILE=1
					shift /1 & goto getargs
				) else (
					if /i "%ARG:~0,3%"=="-pr" (
						set VGL_PROFILE=0
						shift /1 & goto getargs
					) else (
						if /i "%ARG:~0,3%"=="+se" (
							set VGL_SERIAL=1
							shift /1 & goto getargs
						) else (
							if /i "%ARG:~0,3%"=="-se" (
								set VGL_SERIAL=0
								shift /1 & goto getargs
							) else (
								if /i "%ARG:~0,3%"=="+sp" (
									set VGL_SPOIL=1
									shift /1 & goto getargs
								) else (
									if /i "%ARG:~0,3%"=="-sp" (
										set VGL_SPOIL=0
										shift /1 & goto getargs
									) else (
										if /i "%ARG:~0,3%"=="-st" (
											set VGL_STEREO=%2
											shift /1 & shift /1 % goto getargs
										) else (
											if /i "%ARG:~0,3%"=="+tr" (
												set VGL_TRACE=1
												shift /1 & goto getargs
											) else (
												if /i "%ARG:~0,3%"=="-tr" (
													set VGL_TRACE=0
													shift /1 & goto getargs
												) else (
													if /i "%ARG:~0,2%"=="+v" (
														set VGL_VERBOSE=1
														shift /1 & goto getargs
													) else (
														if /i "%ARG:~0,2%"=="-v" (
															set VGL_VERBOSE=0
															shift /1 & goto getargs
														) else (
															goto argsdone
														)
													)
												)
											)
										)
									)
								)
							)
						)
					)
				)
			)
		)
	)
)
:argsdone

set ARG=%~1
if "%ARG%"=="" goto usage

"%WITHDLL%" /d:"%FAKER%" %1 %2 %3 %4 %5 %6 %7 %8 %9

goto done

:usage

echo.
echo USAGE: %0 [options] ^<OpenGL app^> [OpenGL app arguments]
echo.
echo -fps ^<f^>  : Limit client/server frame rate to ^<f^> frames/sec
echo +/-g      : Enable/disable gamma correction
echo -gamma ^<g^>: Set gamma correction factor to ^<g^> (see docs)
echo +/-pr     : Enable/disable performance profiling output (default: off)
echo +/-se     : Turn on/off serialization (default: on)
echo             Turning off serialization causes the compress thread and
echo             readback/render thread to run in parallel, similarly to
echo             VirtualGL for Unix.  This can create resource contention
echo             issues on slower systems.
echo +/-sp     : Turn on/off frame spoiling (default: on)
echo             Note that frame spoiling has no effect if serialization is
echo             also enabled.
echo -st ^<s^>   : left = Send only left eye buffer
echo             right = Send only right eye buffer
echo             rc = Use red/cyan (anaglyphic) stereo [default]
echo +/-tr     : Enable/disable function call tracing (generates a lot of output)
echo +/-v      : Enable/disable verbose VirtualGL messages

:done

endlocal
