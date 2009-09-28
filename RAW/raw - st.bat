@echo off
del bpill.exe /q
copy .\..\bpill.exe bpill.exe /y

bpill -nc -raw st.dat st.tga -type 1


bpill -nc -raw st.dat st2222.tga -type 0 -width 31 -height 400 -offset 0 -bytes 1


pause
