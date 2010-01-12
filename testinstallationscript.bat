@echo off
set B=bpill -f -bigfile pill.big -extract
set S=bpill -f -spr32
set EXT=spr32

echo Character models...

REM --- kain alive ----
echo  kain (alive)...
%B% 5728197C kain0al0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-31 -ofs -40 45
%B% 5728197C kain0al1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 32-63 -ofs -63 50
%B% 5728197C kain0al2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 64-95 -ofs -69 44
%B% 5728197C kain0al3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 96-127 -ofs -71 36
%B% 5728197C kain0al4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 128-159 -ofs -54 34
%B% 5728197C kain0al5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 96-127 -ofs -30 36 -flip
%B% 5728197C kain0al6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 64-95 -ofs -22 44 -flip
%B% 5728197C kain0al7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 32-63 -ofs -25 50 -flip
%B% 5728197C kain0alD.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 160-183 -ofs -31 62 -flip

REM --- kain iron armor + sword ---
echo  kain (iron armor + sword)...
%B% 080C020C kain1sw0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-67 -ofs -52 52
%B% 080C020C kain1sw1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -73 59
%B% 080C020C kain1sw2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -81 55
%B% 080C020C kain1sw3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -80 45
%B% 080C020C kain1sw4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 272-339 -ofs -64 45
%B% 080C020C kain1sw5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -29 45 -flip
%B% 080C020C kain1sw6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -24 55 -flip
%B% 080C020C kain1sw7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -36 58 -flip
REM --- kain iron armor + sword spelling ---
%B% 6E017348 kain1ss0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-11 -ofs -27 37
%B% 6E017348 kain1ss1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -36 34
%B% 6E017348 kain1ss2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -41 34
%B% 6E017348 kain1ss3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -39 35
%B% 6E017348 kain1ss4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 48-59 -ofs -24 37
%B% 6E017348 kain1ss5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -16 35 -flip
%B% 6E017348 kain1ss6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -25 34 -flip
%B% 6E017348 kain1ss7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -31 34 -flip
%B% 6E016248 kain1spl.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-16 -ofs -39 41
REM --- kain iron armor + sword - build single model ---
%S% kain1sw0.%EXT% -merge kain1ss0.%EXT%
%S% kain1sw1.%EXT% -merge kain1ss1.%EXT%
%S% kain1sw2.%EXT% -merge kain1ss2.%EXT%
%S% kain1sw3.%EXT% -merge kain1ss3.%EXT%
%S% kain1sw4.%EXT% -merge kain1ss4.%EXT%
%S% kain1sw5.%EXT% -merge kain1ss5.%EXT%
%S% kain1sw6.%EXT% -merge kain1ss6.%EXT%
%S% kain1sw7.%EXT% -merge kain1ss7.%EXT%
REM --- kain spell effects ---
%B% 011E0F07 kaingain.%EXT% -overhead -i 0-20 -ofs -44 48
%B% 0303683A kaincure.%EXT% -overhead -i 0-16 -ofs -45 44
%B% 17056235 kainreds.%EXT% -overhead -i 0-20 -ofs -39 41
%B% 0D1F7A28 kaingrns.%EXT% -overhead -i 0-15 -ofs -39 41

REM --- green skeleton ---
echo  Green skeleton...
%B% 79671C58 skel0wr0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-11 -ofs -26 31
%B% 79671C58 skel0wr1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -30 31
%B% 79671C58 skel0wr2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -33 22
%B% 79671C58 skel0wr3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -33 26
%B% 79671C58 skel0wr4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 48-59 -ofs -24 35
%B% 79671C58 skel0wr5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -27 26 -flip
%B% 79671C58 skel0wr6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -31 22 -flip
%B% 79671C58 skel0wr7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -28 32 -flip
%B% 79671C58 skel0wrD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 60-78 -ofs -58 41

REM --- gravedigger ---
echo  Gravedigger...
%B% 74671C5C grvdigr0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-25 -ofs -44 67
%B% 74671C5C grvdigr1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 26-51 -ofs -52 61
%B% 74671C5C grvdigr2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 52-77 -ofs -83 52
%B% 74671C5C grvdigr3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 78-103 -ofs -68 53
%B% 74671C5C grvdigr4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 104-129 -ofs -35 56
%B% 74671C5C grvdigr5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 78-103 -ofs -46 53 -flip
%B% 74671C5C grvdigr6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 52-77 -ofs -55 52 -flip
%B% 74671C5C grvdigr7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 26-51 -ofs -48 63 -flip
%B% 74671C5C grvdigrA.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 130-143 -ofs -16 32
%B% 74671C5C grvdigrD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 144-165 -ofs -75 61

REM --- ghoul ---
echo  Ghoul...
%B% 7C661C57 ghoulwk0.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-39 -ofs -24 48
%B% 7C661C57 ghoulwk1.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -35 45
%B% 7C661C57 ghoulwk2.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -42 31
%B% 7C661C57 ghoulwk3.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -50 36
%B% 7C661C57 ghoulwk4.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 160-199 -ofs -29 42
%B% 7C661C57 ghoulwk5.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -23 35 -flip
%B% 7C661C57 ghoulwk6.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -27 31 -flip
%B% 7C661C57 ghoulwk7.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -21 43 -flip
%B% 7C661C57 ghoulwkD.%EXT% -replacecolormap ghoul_palette.tga -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 199-207 -ofs -25 29

REM --- ghoul (strong) ---
echo  Ghoul (strong)...
%B% 7A661C5D ghoulst0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-39 -ofs -24 48
%B% 7A661C5D ghoulst1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -35 45
%B% 7A661C5D ghoulst2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -42 31
%B% 7A661C5D ghoulst3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -50 36
%B% 7A661C5D ghoulst4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 160-199 -ofs -29 42
%B% 7A661C5D ghoulst5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -23 35 -flip
%B% 7A661C5D ghoulst6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -27 31 -flip
%B% 7A661C5D ghoulst7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -21 43 -flip
%B% 7A661C5D ghoulstD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 199-207 -ofs -25 29

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

echo Sounds...
REM --- sounds ---
%B% 12046C45 firesword1.wav
%B% 1413141E firesword2.wav
%B% 564E2D74 firesword3.wav
%B% 14091300 magicspike.wav
%B% 141C0A12 power.wav
%B% 17101D1F explode.wav
%B% 19136C46 kainpain1.wav
%B% 19136C47 kainpain2.wav
%B% 1F04141D kainpain3.wav
%B% 1C136A4D hpain1.wav
%B% 1C136A4E hpain2.wav
%B% 1C1B6736 hdeath1.wav
%B% 45572963 hdeath2.wav
%B% 5A2B7268 hfdeath1.wav
%B% 5A2B7168 hfdeath2.wav
%B% 5A2F0E7F button.wav
%B% 1D15111C bloodgain.wav -speed 1.1
%B% 24690B7C wolfpain.wav
%B% 246A0B7C wolfjump.wav
%B% 246F1779 hit1.wav -trimstart 0.02 -speed 1.1
%B% 246E1779 hit2.wav -trimstart 0.013 -speed 1.1
%B% 24691779 hit3.wav -trimstart 0.013 -speed 0.95
%B% 24701162 choir.wav -speed 1.1
%B% 270F1664 switch.wav -trimstart 0.27
%B% 2A0D1C79 heartbeat.wav
%B% 2A0E0C7E kainlaugh.wav
%B% 2E06196A undead.wav
%B% 39741C68 captivem1.wav
%B% 39771C68 captivem2.wav
%B% 7C101201 captivef1.wav
%B% 3C707867 fullmoon.wav
%B% 474D4153 vaevictis.wav
%B% 24716379 beast1.wav
%B% 474E3C71 beast2.wav
%B% 75000E12 beast3.wav
%B% 475A227F ambient1.wav
%B% 7D060411 ambient2.wav
%B% 485E3B7F transform.wav
%B% 495E2B68 beastdeath1.wav
%B% 7D17131B beastdeath2.wav
%B% 505A2D69 bats.wav
%B% 51543C76 ambient2.wav
%B% 56286765 mechanic.wav
%B% 5A341272 slash.wav

echo Music...
REM --- music ---
%B% 2574636C track01.ogg -ir 22050
%B% 2577636C track02.ogg -ir 22050
%B% 2570636C track03.ogg -ir 22050
%B% 257C636C track04.ogg -ir 22050
%B% 2572636C track05.ogg -ir 22050
%B% 2575626C track06.ogg -ir 22050
%B% 2575636C track07.ogg -ir 22050
%B% 2571636C track08.ogg -ir 22050
%B% 257D636C track09.ogg -ir 22050
%B% 2573636C track10.ogg -ir 22050
%B% 2576636C track11.ogg -ir 22050

echo Copy files...
REM --- copy models ---
set P=..\..\
mkdir %P%kain\models\legacy
del %P%kain\models\legacy\*.spr32 /Q
move *.spr32 %P%kain\models\legacy\ >> garbage

REM --- copy sounds ---
mkdir %P%kain\sound\legacy
del %P%kain\sound\legacy\*.wav /Q
move *.wav %P%kain\sound\legacy\ >> garbage

REM --- copy cdtracks ---
mkdir %P%kain\sound\cdtracks
del %P%kain\sound\legacy\*.ogg /Q
move *.ogg %P%kain\sound\cdtracks\ >> garbage

:QUIT
del garbage /Q
pause
exit

:EXIT

echo Copy files...
REM --- copy models ---
set P=..\..\
mkdir %P%kain\models\legacy
del %P%kain\models\legacy\*.spr32 /Q
move *.spr32 %P%kain\models\legacy\ >> garbage

GOTO QUIT