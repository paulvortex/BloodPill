@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 1D1B653E.dat 1D1B653E_%W%.tga -type 0 -width 29 -height 420 -colormapoffset 8 -colormapbytes 3 -offset 780 -bytes 1
bpill -nc -raw 1D1B653E.dat 1D1B653E.tga -type 3

set /a W=W+1
GOTO LOOP

:EXIT
pause
