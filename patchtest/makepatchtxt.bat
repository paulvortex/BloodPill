REM script to generate patch list for folder
@echo off
IF EXIST patch.txt del /Q patch.txt
for %%i IN (files2\*) DO (
 echo RAW #%%~ni %%i >> patch.txt
)