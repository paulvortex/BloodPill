@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=64

:LOOP
IF "%W%"=="65" ( GOTO EXIT )
bpill -nc -raw 78661C5A.dat 78661C5A_%W%.tga -type 0 -width %w% -height 6000 -offset 0 -bytes 1
set /a W=W+1
GOTO LOOP

:EXIT
pause
