@echo off
set CERTFILE="%USERPROFILE%\rrcert.pem"
if /i "%1"=="root" set CERTFILE=rrcert.pem
openssl req -new -x509 -days 365 -nodes -config rrcert.cnf -out %CERTFILE% -keyout %CERTFILE%
openssl x509 -subject -dates -fingerprint -noout -in %CERTFILE%
