@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

bpill -nc -raw 1E177028.dat 1E177028.tga -type 2 -doubleres 0

pause
