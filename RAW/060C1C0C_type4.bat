@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 060C1C0C_type4.dat 060C1C0C_type4.tga -type 4

set /a W=W+1
GOTO LOOP

:EXIT
pause
