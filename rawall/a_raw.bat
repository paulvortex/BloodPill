@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

for %%i in (*.dat) do bpill -nc -raw %%i %%i.tga -type 0 -width 32 -height 1000 -cmpr1 0
pause
