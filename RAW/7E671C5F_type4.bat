@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

SET W=24

:LOOP
IF "%W%"=="25" ( GOTO EXIT )
bpill -nc -raw 7E671C5F_type4.dat 7E671C5F_type4.tga -type 0 -width 32 -height 2500
bpill -nc -raw 7E671C5F_type4.dat 7E671C5F_type3.tga -type 4

set /a W=W+1
GOTO LOOP

:EXIT
pause
