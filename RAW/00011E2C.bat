@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 00011E2C.dat 00011E2C.tga -type 3

set /a W=W+1
GOTO LOOP

:EXIT
pause
