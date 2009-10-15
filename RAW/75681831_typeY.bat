@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
REM bpill -nc -raw 75681831_typeY.dat 75681831_typeY.tga -type 0 -width 72 -height 512 -offset 26 -bytes 2
REM bpill -nc -raw 75681831_typeY.dat 75681831_typeY_2.tga -type 0 -width 72 -height 512 -offset 27 -bytes 2
REM bpill -nc -raw 75681831_typeY.dat 75681831_typeY_3.tga -type 0 -width 72 -height 512 -offset 27 -bytes 1
bpill -nc -raw 75681831_typeY.dat 75681831_typeY_4.tga -type 0 -width 72 -height 512 -offset 26
bpill -nc -raw 75681831_typeY.dat 75681831_type7.tga -type 7

set /a W=W+1
GOTO LOOP

:EXIT
pause
