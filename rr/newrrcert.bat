@echo off
set CERTFILE="%USERPROFILE%\vglcert.pem"
if /i "%1"=="root" set CERTFILE=vglcert.pem
openssl req -new -x509 -days 365 -nodes -config vglcert.cnf -out %CERTFILE% -keyout %CERTFILE%
openssl x509 -subject -dates -fingerprint -noout -in %CERTFILE%
pause
