@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

bpill -nc -raw 0A0A122D.dat 0A0A122D_bytes1.tga -type 0 -width 20 -height 600 -offset 332 -bytes 1
bpill -nc -raw 0A0A122D.dat 0A0A122D_bytes2.tga -type 0 -width 10 -height 600 -offset 332 -bytes 2
bpill -nc -raw 0A0A122D.dat 0A0A122D_bytes3.tga -type 0 -width 7 -height 600 -offset 332 -bytes 3


pause
