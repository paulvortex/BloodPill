@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y
del *.tga
for %%i in (*.cmp) do (
   echo %%i...
   echo ----- EXTRACTING %%i ----- >> log.txt
   bpill -nc -map %%i -tilespath ../bigfile_pc/original -c -t
)

pause
