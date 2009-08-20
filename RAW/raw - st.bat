@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

bpill -nc -raw st.dat st.tga

pause
