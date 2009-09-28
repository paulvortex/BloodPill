@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=1

:LOOP
IF "%W%"=="2" ( GOTO EXIT )
bpill -nc -raw 0E031007.dat 0E031007_%W%.tga -type 0 -width 72 -height 4000 -offset 6 -bytes 1
bpill -nc -raw 0E031007.dat 0E031007_2.tga -type 0 -width 32 -height 4000 -offset 150000 -bytes 1
bpill -nc -raw 0E031007.dat 0E031007_3.tga -type 0 -width 72 -height 4000 -offset 4 -bytes 3
set /a W=W+1
GOTO LOOP

:EXIT
pause
