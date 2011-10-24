@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

del *.tga
del *.map

set file=m0000126.cmp
bpill -nc -bigfile ../pill.big -extract %file% %file%.tga -t

pause
