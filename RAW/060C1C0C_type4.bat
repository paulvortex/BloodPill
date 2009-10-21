@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 060C1C0C_type4.dat 060C1C0C_type4.tga -type 4
rem bpill -nc -raw 060C1C0C_type4.dat 060C1C0C_type4.tga -type 0 -width 24 -height 2500 -offset 2400 -cmpr1 0 -cmpr2 255

set /a W=W+1
GOTO LOOP

:EXIT
pause
