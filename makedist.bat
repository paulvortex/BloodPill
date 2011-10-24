rmdir /S /Q release
mkdir release
mkdir release\samples
# main
copy bpill.exe release\bpill.exe
copy bpill.txt release\bpill.txt
copy bpill_ru.txt release\bpill_ru.txt
copy klist.txt release\klist.txt
copy font.tim release\font.tim
# sox
copy sox\sox.exe release\sox.exe
copy sox\cyggomp-1.dll release\cyggomp-1.dll
copy sox\cygwin1.dll release\cygwin1.dll
copy zlib1.dll release\zlib1.dll
# samples
copy bo_filetypes.txt release\bo_filetypes.txt
copy samples.txt release\samples\samples.txt
copy samples_ru.txt release\samples\samples_ru.txt
copy bigfile_extract_file.bat release\samples\extract_file.bat
copy bigfile_list.bat release\samples\list.bat
copy bigfile_pack.bat release\samples\pack.bat
copy bigfile_unpack.bat release\samples\unpack.bat
copy tga2tim(test.tga).bat release\samples\tga2tim(test.tga).bat
copy jam2tga.bat release\samples\jam2tga(act2.jam).bat
copy tim2tga(test.tim).bat release\samples\tim2tga(test.tim).bat
copy tim2tga(test.tim)_16to24.bat release\samples\tim2tga(test.tim)_16to24.bat
copy bigfile_unpack_raw.bat release\samples\unpack_raw.bat
copy bigfile_unpack_fortranslating.bat release\samples\unpack_fortranslating.bat
copy bigfile_unpack_oggsounds.bat release\samples\unpack_oggsounds.bat
copy bigfile_unpack_mapsonly_solids.bat release\samples\unpack_mapsonly_solids.bat
copy bigfile_unpack_mapsonly_plain.bat release\samples\unpack_mapsonly_plain.bat
# patch sample
mkdir release\patchtest
mkdir release\patchtest\files
copy patchtest\* release\patchtest\*
copy patchtest\files\* release\patchtest\files\*
del release\patchtest\*.psd
# sourcecode distrib
mkdir release\bloodpillsource\
copy src\*.c release\bloodpillsource\*.c
copy src\*.h release\bloodpillsource\*.h
copy src\*.dsp release\bloodpillsource\*.dsp
copy src\*.vcproj release\bloodpillsource\*.vcproj




