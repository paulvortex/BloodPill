@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=2

:LOOP
IF "%W%"=="100" ( GOTO EXIT )
bpill -nc -raw 786C1839.dat 786C1839_%W%.tga -type 0 -width 128 -height 400 -offset %W% -bytes 2
set /a W=W+1
GOTO LOOP

:EXIT
pause
