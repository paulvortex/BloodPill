@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 77691832_typeX.dat 77691832.tga -type 7

set /a W=W+1
GOTO LOOP

:EXIT
pause
