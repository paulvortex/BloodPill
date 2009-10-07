@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 77681A2F.dat 77681A2F_%W%.tga -type 0 -width %W% -height 120 -colormapoffset 8 -colormapbytes 3 -offset 860 -bytes 1
bpill -nc -raw 77681A2F.dat 77681A2F.tga -type 3

set /a W=W+1
GOTO LOOP

:EXIT
pause
