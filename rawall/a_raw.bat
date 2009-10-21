@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

REM for %%i in (*.dat) do bpill -nc -raw %%i %%i.tga -type 0 -width 32 -height 2500 -bytes 1
REM for %%i in (*.dat) do bpill -nc -raw %%i %%i.tga -type 0 -width 72 -height 256 -offset 26 -bytes 2
for %%i in (*.dat) do bpill -nc -raw %%i %%i.tga -type 7
pause
