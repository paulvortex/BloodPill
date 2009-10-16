@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 75681834_typeZ.dat 75681834_typeZ.tga -type 7
bpill -nc -raw 75681834_typeZ.dat 75681834_typeZ_2.tga -type 0 -width 72 -height 1400 -offset 0 -bytes 1

set /a W=W+1
GOTO LOOP

:EXIT
pause
