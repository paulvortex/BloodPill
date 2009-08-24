mkdir release
mkdir release\samples
# main
copy bpill.exe release\bpill.exe
copy bpill.txt release\bpill.txt
copy bpill_ru.txt release\bpill_ru.txt
copy klist.txt release\klist.txt
# sox
copy sox\sox.exe release\sox.exe
copy sox\cyggomp-1.dll release\cyggomp-1.dll
copy sox\cygwin1.dll release\cygwin1.dll
# samples
copy bigfile_list.bat release\samples\bigfile_list.bat
copy bigfile_pack.bat release\samples\bigfile_pack.bat
copy bigfile_pack_lowmem.bat release\sample\bigfile_pack_lowmem.bat
copy bigfile_unpack.bat release\samples\bigfile_unpack.bat
copy bigfile_unpack_tim2tga.bat release\samples\bigfile_unpack_tim2tga.bat
copy tga2tim(test.tga).bat release\samples\tga2tim(test.tga).bat
copy tim2tga(test.tim).bat release\samples\tim2tga(test.tim).bat
copy tim2tga(test.tim)_16to24.bat release\samples\tim2tga(test.tim)_16to24.bat

