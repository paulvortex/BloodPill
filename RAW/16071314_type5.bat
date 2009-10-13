@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 16071314_type5.dat 16071314_type5.tga -type 5

set /a W=W+1
GOTO LOOP

:EXIT
pause
