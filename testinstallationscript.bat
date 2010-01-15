@echo off
set B=bpill -f -bigfile pill.big -extract
set S=bpill -f -spr32
set N=testinstallation\script\legacy.nsx
set EXT=spr32
set scale2x=1

REM --- using 2x mode ---
IF "%scale2x%"=="1" (
 set BE=-nearest2x
 set MS=0.4
 set MDS=0.9
) ELSE (
 set BE=
 set MS=0.8
 set MDS=0.9
)

REM --- INIT ----
echo Initializing...
mkdir testinstallation
mkdir testinstallation\script

REM --- CHARACTER SCRIPTS ----
echo Creating script file...
echo // Legacy stuff script file>>%N%
echo [models]name={type,scale}>>%N%
echo kain0al=oriented,%MS% >> %N%
echo kain1sw=oriented,%MS% >> %N%
echo kain1spl=flat,%MS% >> %N%
echo kaingain=flat,%MS% >> %N%
echo kaincure=flat,%MS% >> %N%
echo kainreds=flat,%MS% >> %N%
echo kaingrns=flat,%MS% >> %N%
echo skel0wr=oriented,%MS% >> %N%
echo skel0wrD=decal,%MDS% >> %N%
echo grvdigr=oriented,%MS% >> %N%
echo grvdigrA=flat,%MS% >> %N%
echo grvdigrD=decal,%MDS% >> %N%
echo skel0wr=oriented,%MS% >> %N%
echo skel0wrD=decal,%MDS% >> %N%
echo ghast=oriented,%MS% >> %N%
echo ghastD=decal,%MDS% >> %N%

REM --- CHARACTERS ----
echo Character models...
set D=testinstallation/models/legacy

REM --- kain alive ----
echo  kain (alive)
%B% 5728197C %D%/kain0al0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-31 -ofs -40 45 %BE%
%B% 5728197C %D%/kain0al1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 32-63 -ofs -63 50 %BE%
%B% 5728197C %D%/kain0al2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 64-95 -ofs -69 44 %BE%
%B% 5728197C %D%/kain0al3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 96-127 -ofs -71 36 %BE%
%B% 5728197C %D%/kain0al4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 128-159 -ofs -54 34 %BE%
%B% 5728197C %D%/kain0al5.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 96-127 -ofs -30 36 -flip %BE%
%B% 5728197C %D%/kain0al6.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 64-95 -ofs -22 44 -flip %BE%
%B% 5728197C %D%/kain0al7.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 32-63 -ofs -25 50 -flip %BE%
%B% 5728197C %D%/kain0alD.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 160-183 -ofs -31 62 -flip %BE%

REM --- kain iron armor + sword ---
echo  kain (iron armor + sword)
%B% 080C020C %D%/kain1sw0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-67 -ofs -52 52 %BE%
%B% 080C020C %D%/kain1sw1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -73 59 %BE%
%B% 080C020C %D%/kain1sw2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -81 55 %BE%
%B% 080C020C %D%/kain1sw3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -80 45 %BE%
%B% 080C020C %D%/kain1sw4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 272-339 -ofs -64 45 %BE%
%B% 080C020C %D%/kain1sw5.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -29 45 -flip %BE%
%B% 080C020C %D%/kain1sw6.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -24 55 -flip %BE%
%B% 080C020C %D%/kain1sw7.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -36 58 -flip %BE%
REM --- kain iron armor + sword spelling ---
%B% 6E017348 %D%/kain1ss0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-11 -ofs -27 37 %BE%
%B% 6E017348 %D%/kain1ss1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -36 34 %BE%
%B% 6E017348 %D%/kain1ss2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -41 34 %BE%
%B% 6E017348 %D%/kain1ss3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -39 35 %BE%
%B% 6E017348 %D%/kain1ss4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 48-59 -ofs -24 37 %BE%
%B% 6E017348 %D%/kain1ss5.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -16 35 -flip %BE%
%B% 6E017348 %D%/kain1ss6.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -25 34 -flip %BE%
%B% 6E017348 %D%/kain1ss7.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -31 34 -flip %BE%
%B% 6E016248 %D%/kain1spl.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-16 -ofs -39 41 %BE%
REM --- kain iron armor + sword - build single model ---
%S% %D%/kain1sw0.%EXT% -merge %D%/kain1ss0.%EXT%
%S% %D%/kain1sw1.%EXT% -merge %D%/kain1ss1.%EXT%
%S% %D%/kain1sw2.%EXT% -merge %D%/kain1ss2.%EXT%
%S% %D%/kain1sw3.%EXT% -merge %D%/kain1ss3.%EXT%
%S% %D%/kain1sw4.%EXT% -merge %D%/kain1ss4.%EXT%
%S% %D%/kain1sw5.%EXT% -merge %D%/kain1ss5.%EXT%
%S% %D%/kain1sw6.%EXT% -merge %D%/kain1ss6.%EXT%
%S% %D%/kain1sw7.%EXT% -merge %D%/kain1ss7.%EXT%
REM --- kain spell effects ---
%B% 011E0F07 %D%/kaingain.%EXT% -overhead -i 0-20 -ofs -44 48 %BE%
%B% 0303683A %D%/kaincure.%EXT% -overhead -i 0-16 -ofs -45 44 %BE%
%B% 17056235 %D%/kainreds.%EXT% -overhead -i 0-20 -ofs -39 41 %BE%
%B% 0D1F7A28 %D%/kaingrns.%EXT% -overhead -i 0-15 -ofs -39 41 %BE%

REM --- green skeleton ---
echo  Green skeleton
%B% 79671C58 %D%/skel0wr0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-11 -ofs -26 31 %BE%
%B% 79671C58 %D%/skel0wr1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -30 31 %BE%
%B% 79671C58 %D%/skel0wr2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -33 22 %BE%
%B% 79671C58 %D%/skel0wr3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -33 26 %BE%
%B% 79671C58 %D%/skel0wr4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 48-59 -ofs -24 35 %BE%
%B% 79671C58 %D%/skel0wr5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -27 26 -flip %BE%
%B% 79671C58 %D%/skel0wr6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -31 22 -flip %BE%
%B% 79671C58 %D%/skel0wr7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -28 32 -flip %BE%
%B% 79671C58 %D%/skel0wrD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 60-78 -ofs -58 41 %BE%

REM --- gravedigger ---
echo  Gravedigger
%B% 74671C5C %D%/grvdigr0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-25 -ofs -44 67 %BE%
%B% 74671C5C %D%/grvdigr1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 26-51 -ofs -52 61 %BE%
%B% 74671C5C %D%/grvdigr2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 52-77 -ofs -83 52 %BE%
%B% 74671C5C %D%/grvdigr3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 78-103 -ofs -68 53 %BE%
%B% 74671C5C %D%/grvdigr4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 104-129 -ofs -35 56 %BE%
%B% 74671C5C %D%/grvdigr5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 78-103 -ofs -46 53 -flip %BE%
%B% 74671C5C %D%/grvdigr6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 52-77 -ofs -55 52 -flip %BE%
%B% 74671C5C %D%/grvdigr7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 26-51 -ofs -48 63 -flip %BE%
%B% 74671C5C %D%/grvdigrA.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 144-151 -i 130-143 -ofs -17 31 %BE%
%B% 74671C5C %D%/grvdigrD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 152-165 -ofs -75 61 %BE%

REM --- ghoul ---
echo  Ghoul
%B% 7C661C57 %D%/ghoul0.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-39 -ofs -24 48 %BE%
%B% 7C661C57 %D%/ghoul1.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -35 45 %BE%
%B% 7C661C57 %D%/ghoul2.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -42 31 %BE%
%B% 7C661C57 %D%/ghoul3.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -50 36 %BE%
%B% 7C661C57 %D%/ghoul4.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 160-199 -ofs -29 42 %BE%
%B% 7C661C57 %D%/ghoul5.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -23 35 -flip %BE%
%B% 7C661C57 %D%/ghoul6.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -27 31 -flip %BE%
%B% 7C661C57 %D%/ghoul7.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -21 43 -flip %BE%
%B% 7C661C57 %D%/ghoulD.%EXT% -replacecolormap ghoul_palette.tga -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 199-207 -ofs -25 29 %BE%

REM --- ghoul (strong) ---
echo  Ghast
%B% 7A661C5D %D%/ghast0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-39 -ofs -24 48 %BE%
%B% 7A661C5D %D%/ghast1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -35 45 %BE%
%B% 7A661C5D %D%/ghast2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -42 31 %BE%
%B% 7A661C5D %D%/ghast3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -50 36 %BE%
%B% 7A661C5D %D%/ghast4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 160-199 -ofs -29 42 %BE%
%B% 7A661C5D %D%/ghast5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -23 35 -flip %BE%
%B% 7A661C5D %D%/ghast6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -27 31 -flip %BE%
%B% 7A661C5D %D%/ghast7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -21 43 -flip %BE%
%B% 7A661C5D %D%/ghastD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 199-207 -ofs -25 29 %BE%

REM --- rogue swordsman ---
REM 7C661C5D

REM --- rogue commander ---
REM 75671C56

REM --- rogue (knifethrower) ---
REM 79671C5D

REM --- ariel ---
REM 79661C56

REM --- gog ---
REM 79671C5A

GOTO EXIT

REM --- SOUNDS ---
echo Sounds...
set D=testinstallation/sound/legacy
%B% 12046C45 %D%/firesword1.wav
%B% 1413141E %D%/firesword2.wav
%B% 564E2D74 %D%/firesword3.wav
%B% 14091300 %D%/magicspike.wav
%B% 141C0A12 %D%/power.wav
%B% 17101D1F %D%/explode.wav
%B% 19136C46 %D%/kainpain1.wav
%B% 19136C47 %D%/kainpain2.wav
%B% 1F04141D %D%/kainpain3.wav
%B% 1C136A4D %D%/hpain1.wav
%B% 1C136A4E %D%/hpain2.wav
%B% 1C1B6736 %D%/hdeath1.wav
%B% 45572963 %D%/hdeath2.wav
%B% 5A2B7268 %D%/hfdeath1.wav
%B% 5A2B7168 %D%/hfdeath2.wav
%B% 5A2F0E7F %D%/button.wav
%B% 1D15111C %D%/bloodgain.wav -speed 1.1
%B% 24690B7C %D%/wolfpain.wav
%B% 246A0B7C %D%/wolfjump.wav
%B% 246F1779 %D%/hit1.wav -trimstart 0.02 -speed 1.1
%B% 246E1779 %D%/hit2.wav -trimstart 0.013 -speed 1.1
%B% 24691779 %D%/hit3.wav -trimstart 0.013 -speed 0.95
%B% 24701162 %D%/choir.wav -speed 1.1
%B% 270F1664 %D%/switch.wav -trimstart 0.27
%B% 2A0D1C79 %D%/heartbeat.wav
%B% 2A0E0C7E %D%/kainlaugh.wav
%B% 2E06196A %D%/undead.wav
%B% 39741C68 %D%/captivem1.wav
%B% 39771C68 %D%/captivem2.wav
%B% 7C101201 %D%/captivef1.wav
%B% 3C707867 %D%/fullmoon.wav
%B% 474D4153 %D%/vaevictis.wav
%B% 24716379 %D%/beast1.wav
%B% 474E3C71 %D%/beast2.wav
%B% 75000E12 %D%/beast3.wav
%B% 475A227F %D%/ambient1.wav
%B% 7D060411 %D%/ambient2.wav
%B% 485E3B7F %D%/transform.wav
%B% 495E2B68 %D%/beastdeath1.wav
%B% 7D17131B %D%/beastdeath2.wav
%B% 505A2D69 %D%/bats.wav
%B% 51543C76 %D%/ambient2.wav
%B% 56286765 %D%/mechanic.wav
%B% 5A341272 %D%/slash.wav

REM --- MUSIC ---
echo Music...
set D=testinstallation/sound/cdtracks
%B% 2574636C %D%/track01.ogg -ir 22050
%B% 2577636C %D%/track02.ogg -ir 22050
%B% 2570636C %D%/track03.ogg -ir 22050
%B% 257C636C %D%/track04.ogg -ir 22050
%B% 2572636C %D%/track05.ogg -ir 22050
%B% 2575626C %D%/track06.ogg -ir 22050
%B% 2575636C %D%/track07.ogg -ir 22050
%B% 2571636C %D%/track08.ogg -ir 22050
%B% 257D636C %D%/track09.ogg -ir 22050
%B% 2573636C %D%/track10.ogg -ir 22050
%B% 2576636C %D%/track11.ogg -ir 22050

:EXIT

IF NOT EXIST winrar.exe (
 echo Copy files...
 xcopy testinstallation\* ..\..\kain\ /S /E /Q /Y
) ELSE (
 echo Create LEGACY.PK3...
 call winrar.exe a -r -m4 -ep1 "LEGACY.pk3" -- "LEGACY.pk3" testinstallation\*
 @echo on
 move /Y LEGACY.PK3 ..\..\kain
)
rmdir testinstallation /Q /S
pause
