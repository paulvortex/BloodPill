@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

bpill -nc -raw 1A137337.dat 1A137337.tga -type 2

pause
