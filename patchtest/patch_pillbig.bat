@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

bpill.exe -bigfile PILL.big -patch patch.txt
pause
