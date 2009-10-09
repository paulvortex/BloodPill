@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 0B081424.dat 0B081424.tga -type 3 -chunk 0

set /a W=W+1
GOTO LOOP

:EXIT
pause
