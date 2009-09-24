@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

bpill -nc -raw 1E177028.dat 1E177028.tga -type 2 -doubleres 0

REM bpill -nc -raw 1E177028.dat 1E177028.tga -type 0 -width 37 -height 400 -offset 36000 -bytes 1

pause
