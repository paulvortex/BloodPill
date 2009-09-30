@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=1

:LOOP
IF "%W%"=="2" ( GOTO EXIT )
bpill -nc -raw 74681A26.dat 74681A26_%W%.tga -type 0 -width 32 -height 1000 -offset 0 -bytes 1
bpill -nc -raw 74681A26.dat 74681A26_%W%_4.tga -type 0 -width 32 -height 1000 -offset 4 -bytes 3
set /a W=W+1
GOTO LOOP

:EXIT
pause
