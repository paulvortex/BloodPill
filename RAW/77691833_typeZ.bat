@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=1

:LOOP
IF "%W%"=="100" ( GOTO EXIT )
REM bpill -nc -raw 75681834_typeZ.dat 75681834_typeZ.tga -type 7
bpill -nc -raw 75681834_typeZ.dat 75681834_typeZ_2_%W%.tga -type 0 -width %W% -height 500 -offset 0 -bytes 1

set /a W=W+1
GOTO LOOP

:EXIT
pause
