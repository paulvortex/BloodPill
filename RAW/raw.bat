@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

del *.tga
del *.map

set file=m0000812.cmp
bpill -nc -bigfile ../pill.big -extract %file% %file%.tga

pause
