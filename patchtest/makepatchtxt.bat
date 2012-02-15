REM script to generate patch list for folder
@echo off
IF EXIST patch.txt del /Q patch.txt
for %%i IN (files\*) DO (
 echo RAW %%~ni%%~xi %%i >> patch.txt
)