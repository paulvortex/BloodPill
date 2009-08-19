@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

for %%i in (*.dat) do bpill -nc -raw %%i %%i.tga -bpp 1

pause
