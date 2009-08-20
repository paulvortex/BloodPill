@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

bpill -nc -raw t1.dat t1.tga

pause
