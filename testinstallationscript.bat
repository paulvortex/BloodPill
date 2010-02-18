@echo off
set B=bpill -f -bigfile pill.big -extract
set S=bpill -f -spr32
set N=testinstallation\script\legacy.nsx
set CM=testinstallation\script\colormaps.nsx
set EXT=spr32
set scale2x=0

REM --- using 2x mode ---
IF "%scale2x%"=="1" (
 set BE=-nearest2x
 set PS=0.41
 set MS=0.46
 set MDS=0.5
 set BS=0.6
) ELSE (
 set BE=
 set PS=0.82
 set MS=0.92
 set MDS=1.0
 set BS=1.2
)

REM --- INIT ----
echo Initializing...
mkdir testinstallation
mkdir testinstallation\script

REM --- COLORMAPS ---
set colormap_null=0
set colormap_blood_red=1
set colormap_blood_green=2
set colormap_blood_black=3
set colormap_blood_blue=4
set colormap_gray=5
set colormap_waterpuff=6
set colormap_kain_alive=32
set colormap_kain_armor1=33
set colormap_kain_armor2=34
set colormap_kain_armor3=35
set colormap_kain_armor4=36
set colormap_kain_armor5=37
set colormap_flayed_skeleton=38
set colormap_gravedigger=39
set colormap_green_skeleton=40
set colormap_ghoul=41
set colormap_ghast=42
set colormap_brigand=43
set colormap_brigandmerc=44
set colormap_brigandmace=45
set colormap_macemerc=46

REM --- CHARACTER SCRIPTS ----
echo Creating script file...
echo // Legacy stuff script file >> %N%
echo [models]name={type,scale,paletteindex} >> %N%
echo kain0al=oriented,%PS%,%colormap_kain_alive% >> %N%
echo kain1sw=oriented,%PS%,%colormap_kain_armor1% >> %N%
echo kain1spl=flat,%PS%,%colormap_kain_armor1% >> %N%
echo kain2sw=oriented,%PS%,%colormap_kain_armor2% >> %N%
echo kain2spl=flat,%PS%,%colormap_kain_armor2% >> %N%
echo kain3sw=oriented,%PS%,%colormap_kain_armor3% >> %N%
echo kain3spl=flat,%PS%,%colormap_kain_armor3% >> %N%
echo kain4sw=oriented,%PS%,%colormap_kain_armor4% >> %N%
echo kain4spl=flat,%PS%,%colormap_kain_armor4% >> %N%
echo kain5sw=oriented,%PS%,%colormap_kain_armor5% >> %N%
echo kain5spl=flat,%PS%,%colormap_kain_armor5% >> %N%
echo kaingain=flat,%PS%,%colormap_null% >> %N%
echo kaincure=flat,%PS%,%colormap_null% >> %N%
echo kainreds=flat,%PS%,%colormap_null% >> %N%
echo kaingrns=flat,%PS%,%colormap_null% >> %N%
echo kaintele=flat,%PS%,%colormap_null% >> %N%
echo kainmlit=flat,%PS%,%colormap_null% >> %N%
echo flayed=decal,%BS%,%colormap_flayed_skeleton% >> %N%
echo skel0wr=oriented,%MS%,%colormap_green_skeleton% >> %N%
echo skel0wrD=decal,%MDS%,%colormap_green_skeleton% >> %N%
echo grvdigr=oriented,%MS%,%colormap_gravedigger% >> %N%
echo grvdigrA=flat,%MS%,%colormap_gravedigger% >> %N%
echo grvdigrD=decal,%MDS%,%colormap_gravedigger% >> %N%
echo ghoul=oriented,%MS%,%colormap_ghoul% >> %N%
echo ghoulD=decal,%MDS%,%colormap_ghoul% >> %N%
echo ghast=oriented,%MS%,%colormap_ghast% >> %N%
echo ghastD=decal,%MDS%,%colormap_ghast% >> %N%
echo brigand=oriented,%MS%,%colormap_brigand% >> %N%
echo brigandA=flat,%MS%,%colormap_brigand% >> %N%
echo brigandD=decal,%MDS%,%colormap_brigand% >> %N%
echo brgmerc=oriented,%MS%,%colormap_brigandmerc% >> %N%
echo brgmercA=flat,%MS%,%colormap_brigandmerc% >> %N%
echo brgmercD=decal,%MDS%,%colormap_brigandmerc% >> %N%
echo brgmace=oriented,%MS%,%colormap_brigandmace% >> %N%
echo brgmaceA=flat,%MS%,%colormap_brigandmace% >> %N%
echo brgmaceD=decal,%MDS%,%colormap_brigandmace% >> %N%
echo macemrc=oriented,%MS%,%colormap_macemerc% >> %N%
echo macemrcA=flat,%MS%,%colormap_macemerc% >> %N%
echo macemrcD=decal,%MDS%,%colormap_macemerc% >> %N%

echo [colormaps]index={colormap} >> %CM%
echo # colormaps 0-31 are system ones >> %CM%
echo %colormap_blood_red%='28 0 0''37 0 0''42 0 0''56 0 0''69 0 0''74 0 0''79 0 0''84 0 0' >> %CM%
echo %colormap_blood_green%='40 90 38''42 90 31''42 72 28''28 44 12''20 33 7' >> %CM%
echo %colormap_blood_black%='7 7 7''8 7 8''12 12 12''13 12 13''15 15 15''17 15 17''20 20 20''23 20 23''24 24 24''27 24 27' >> %CM%
echo %colormap_blood_blue%='20 19 90''21 16 90''21 17 72''14 6 44''10 4 33' >> %CM%
echo %colormap_gray%='16 16 16''48 48 48''64 64 64''96 96 96''128 128 128''160 160 160' >> %CM%
echo %colormap_waterpuff%='16 16 21''48 48 53''64 64 70''96 96 102''128 128 138''160 160 173' >> %CM%
echo # misc colormaps >> %CM%

REM --- CHARACTERS ----
echo Character models...
set D=testinstallation/models/legacy

REM GOTO FAST

REM --- kain alive ----
echo  kain (alive)
%B% 5728197C %D%/kain0al0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-31 -ofs -40 45 %BE% -colormap2nsx 1 14 %colormap_kain_alive% %CM%
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
%B% 080C020C %D%/kain1sw0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-67 -ofs -52 52 %BE% -colormap2nsx 1 14 %colormap_kain_armor1% %CM%
%B% 080C020C %D%/kain1sw1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -73 59 %BE%
%B% 080C020C %D%/kain1sw2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -81 55 %BE%
%B% 080C020C %D%/kain1sw3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -80 45 %BE%
%B% 080C020C %D%/kain1sw4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 272-339 -ofs -66 45 %BE%
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
%B% 6E016248 %D%/kain1spl.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-16 -ofs -40 41 %BE%
REM --- kain iron armor + sword - build single model ---
%S% %D%/kain1sw0.%EXT% -merge %D%/kain1ss0.%EXT%
%S% %D%/kain1sw1.%EXT% -merge %D%/kain1ss1.%EXT%
%S% %D%/kain1sw2.%EXT% -merge %D%/kain1ss2.%EXT%
%S% %D%/kain1sw3.%EXT% -merge %D%/kain1ss3.%EXT%
%S% %D%/kain1sw4.%EXT% -merge %D%/kain1ss4.%EXT%
%S% %D%/kain1sw5.%EXT% -merge %D%/kain1ss5.%EXT%
%S% %D%/kain1sw6.%EXT% -merge %D%/kain1ss6.%EXT%
%S% %D%/kain1sw7.%EXT% -merge %D%/kain1ss7.%EXT%

REM --- kain bone armor + sword ---
echo  kain (bone armor + sword)
%B% 0D0F020C %D%/kain2sw0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-67 -ofs -52 56 %BE% -colormap2nsx 1 14 %colormap_kain_armor2% %CM%
%B% 0D0F020C %D%/kain2sw1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -73 63 %BE%
%B% 0D0F020C %D%/kain2sw2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -81 59 %BE%
%B% 0D0F020C %D%/kain2sw3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -80 49 %BE%
%B% 0D0F020C %D%/kain2sw4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 272-339 -ofs -64 49 %BE%
%B% 0D0F020C %D%/kain2sw5.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -29 49 -flip %BE%
%B% 0D0F020C %D%/kain2sw6.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -24 59 -flip %BE%
%B% 0D0F020C %D%/kain2sw7.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -36 62 -flip %BE%
REM --- kain bone armor + sword spelling ---
%B% 6C01734D %D%/kain2ss0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-11 -ofs -27 41 %BE%
%B% 6C01734D %D%/kain2ss1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -36 38 %BE%
%B% 6C01734D %D%/kain2ss2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -41 38 %BE%
%B% 6C01734D %D%/kain2ss3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -39 39 %BE%
%B% 6C01734D %D%/kain2ss4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 48-59 -ofs -24 41 %BE%
%B% 6C01734D %D%/kain2ss5.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -16 39 -flip %BE%
%B% 6C01734D %D%/kain2ss6.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -25 38 -flip %BE%
%B% 6C01734D %D%/kain2ss7.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -31 38 -flip %BE%
%B% 6C01624D %D%/kain2spl.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-16 -ofs -39 41 %BE%
REM --- kain bone armor + sword - build single model ---
%S% %D%/kain2sw0.%EXT% -merge %D%/kain2ss0.%EXT%
%S% %D%/kain2sw1.%EXT% -merge %D%/kain2ss1.%EXT%
%S% %D%/kain2sw2.%EXT% -merge %D%/kain2ss2.%EXT%
%S% %D%/kain2sw3.%EXT% -merge %D%/kain2ss3.%EXT%
%S% %D%/kain2sw4.%EXT% -merge %D%/kain2ss4.%EXT%
%S% %D%/kain2sw5.%EXT% -merge %D%/kain2ss5.%EXT%
%S% %D%/kain2sw6.%EXT% -merge %D%/kain2ss6.%EXT%
%S% %D%/kain2sw7.%EXT% -merge %D%/kain2ss7.%EXT%

REM --- kain chaos armor + sword ---
echo  kain (chaos armor + sword)
%B% 0208030C %D%/kain3sw0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-67 -ofs -52 52 %BE% -colormap2nsx 1 14 %colormap_kain_armor3% %CM%
%B% 0208030C %D%/kain3sw1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -73 59 %BE%
%B% 0208030C %D%/kain3sw2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -81 55 %BE%
%B% 0208030C %D%/kain3sw3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -80 45 %BE%
%B% 0208030C %D%/kain3sw4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 272-339 -ofs -64 45 %BE%
%B% 0208030C %D%/kain3sw5.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -29 45 -flip %BE%
%B% 0208030C %D%/kain3sw6.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -24 55 -flip %BE%
%B% 0208030C %D%/kain3sw7.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -36 58 -flip %BE%
REM --- kain chaos armor + sword spelling ---
%B% 6D01734E %D%/kain3ss0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-11 -ofs -27 37 %BE%
%B% 6D01734E %D%/kain3ss1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -36 34 %BE%
%B% 6D01734E %D%/kain3ss2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -41 34 %BE%
%B% 6D01734E %D%/kain3ss3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -39 35 %BE%
%B% 6D01734E %D%/kain3ss4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 48-59 -ofs -24 37 %BE%
%B% 6D01734E %D%/kain3ss5.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -16 35 -flip %BE%
%B% 6D01734E %D%/kain3ss6.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -25 34 -flip %BE%
%B% 6D01734E %D%/kain3ss7.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -31 34 -flip %BE%
%B% 6D01624E %D%/kain3spl.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-16 -ofs -39 41 %BE%
REM --- kain chaos armor + sword - build single model ---
%S% %D%/kain3sw0.%EXT% -merge %D%/kain3ss0.%EXT%
%S% %D%/kain3sw1.%EXT% -merge %D%/kain3ss1.%EXT%
%S% %D%/kain3sw2.%EXT% -merge %D%/kain3ss2.%EXT%
%S% %D%/kain3sw3.%EXT% -merge %D%/kain3ss3.%EXT%
%S% %D%/kain3sw4.%EXT% -merge %D%/kain3ss4.%EXT%
%S% %D%/kain3sw5.%EXT% -merge %D%/kain3ss5.%EXT%
%S% %D%/kain3sw6.%EXT% -merge %D%/kain3ss6.%EXT%
%S% %D%/kain3sw7.%EXT% -merge %D%/kain3ss7.%EXT%

REM --- kain flesh armor + sword ---
echo  kain (flesh armor + sword)
%B% 0D0B130C %D%/kain4sw0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-67 -ofs -52 52 %BE% -colormap2nsx 1 14 %colormap_kain_armor4% %CM%
%B% 0D0B130C %D%/kain4sw1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -73 59 %BE%
%B% 0D0B130C %D%/kain4sw2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -81 55 %BE%
%B% 0D0B130C %D%/kain4sw3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -80 45 %BE%
%B% 0D0B130C %D%/kain4sw4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 272-339 -ofs -64 45 %BE%
%B% 0D0B130C %D%/kain4sw5.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 204-271 -ofs -29 45 -flip %BE%
%B% 0D0B130C %D%/kain4sw6.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-203 -ofs -24 55 -flip %BE%
%B% 0D0B130C %D%/kain4sw7.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-135 -ofs -36 58 -flip %BE%
REM --- kain flesh armor + sword spelling ---
%B% 6D01734F %D%/kain4ss0.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-11 -ofs -27 37 %BE%
%B% 6D01734F %D%/kain4ss1.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -36 34 %BE%
%B% 6D01734F %D%/kain4ss2.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -41 34 %BE%
%B% 6D01734F %D%/kain4ss3.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -39 35 %BE%
%B% 6D01734F %D%/kain4ss4.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 48-59 -ofs -24 37 %BE%
%B% 6D01734F %D%/kain4ss5.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -16 35 -flip %BE%
%B% 6D01734F %D%/kain4ss6.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -25 34 -flip %BE%
%B% 6D01734F %D%/kain4ss7.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -31 34 -flip %BE%
%B% 6D01624F %D%/kain4spl.%EXT% -overhead -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-16 -ofs -39 41 %BE%
REM --- kain flesh armor + sword - build single model ---
%S% %D%/kain4sw0.%EXT% -merge %D%/kain4ss0.%EXT%
%S% %D%/kain4sw1.%EXT% -merge %D%/kain4ss1.%EXT%
%S% %D%/kain4sw2.%EXT% -merge %D%/kain4ss2.%EXT%
%S% %D%/kain4sw3.%EXT% -merge %D%/kain4ss3.%EXT%
%S% %D%/kain4sw4.%EXT% -merge %D%/kain4ss4.%EXT%
%S% %D%/kain4sw5.%EXT% -merge %D%/kain4ss5.%EXT%
%S% %D%/kain4sw6.%EXT% -merge %D%/kain4ss6.%EXT%
%S% %D%/kain4sw7.%EXT% -merge %D%/kain4ss7.%EXT%

REM --- kain spell effects ---
echo  kain spell effects
%B% 011E0F07 %D%/kaingain.%EXT% -overhead -i 0-20 -ofs -45 48 %BE%
%B% 0303683A %D%/kaincure.%EXT% -overhead -i 0-16 -ofs -46 44 %BE%
%B% 17056235 %D%/kainreds.%EXT% -overhead -i 0-20 -ofs -40 41 %BE%
%B% 0D1F7A28 %D%/kaingrns.%EXT% -overhead -i 0-15 -ofs -40 41 %BE%
%B% 1F146B3E %D%/kaintele.%EXT% -overhead -i 0-20 -ofs -21 36 %BE%
%B% 512A0B71 %D%/kainmlit.%EXT% -overhead -i 0-13 -ofs -43 47 %BE%
%B% 1D1B653E %D%/flayed.%EXT% -oriented -i 30-48 -ofs -21 20 -replacecolormap flayed_palette.tga %BE% -colormap2nsx 1 14 %colormap_flayed_skeleton% %CM%
%B% 1D1B653E %D%/repel.%EXT% -overhead -i 49-49 -ofs -49 49 %BE%

REM --- gravedigger ---
echo  Gravedigger
%B% 74671C5C %D%/grvdigr0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-25 -ofs -44 67 %BE% -colormap2nsx 1 14 %colormap_gravedigger% %CM%
%B% 74671C5C %D%/grvdigr1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 26-51 -ofs -52 61 %BE%
%B% 74671C5C %D%/grvdigr2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 52-77 -ofs -83 52 %BE%
%B% 74671C5C %D%/grvdigr3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 78-103 -ofs -68 53 %BE%
%B% 74671C5C %D%/grvdigr4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 104-129 -ofs -35 56 %BE%
%B% 74671C5C %D%/grvdigr5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 78-103 -ofs -46 53 -flip %BE%
%B% 74671C5C %D%/grvdigr6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 52-77 -ofs -55 52 -flip %BE%
%B% 74671C5C %D%/grvdigr7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 26-51 -ofs -48 63 -flip %BE%
%B% 74671C5C %D%/grvdigrA.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 144-151 -i 130-143 -ofs -17 31 %BE%
%B% 74671C5C %D%/grvdigrD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 152-165 -ofs -75 61 %BE%

REM --- green skeleton ---
echo  Green skeleton
%B% 79671C58 %D%/skel0wr0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-11 -ofs -26 31 %BE% -colormap2nsx 1 14 %colormap_green_skeleton% %CM%
%B% 79671C58 %D%/skel0wr1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -30 31 %BE%
%B% 79671C58 %D%/skel0wr2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -33 22 %BE%
%B% 79671C58 %D%/skel0wr3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -33 26 %BE%
%B% 79671C58 %D%/skel0wr4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 48-59 -ofs -24 35 %BE%
%B% 79671C58 %D%/skel0wr5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 36-47 -ofs -27 26 -flip %BE%
%B% 79671C58 %D%/skel0wr6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 24-35 -ofs -31 22 -flip %BE%
%B% 79671C58 %D%/skel0wr7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 12-23 -ofs -28 32 -flip %BE%
%B% 79671C58 %D%/skel0wrD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 60-78 -ofs -58 41 %BE%

REM --- ghoul ---
echo  Ghoul
%B% 7C661C57 %D%/ghoul0.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-39 -ofs -24 48 %BE% -colormap2nsx 1 14 %colormap_ghoul% %CM%
%B% 7C661C57 %D%/ghoul1.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -35 45 %BE%
%B% 7C661C57 %D%/ghoul2.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -42 31 %BE%
%B% 7C661C57 %D%/ghoul3.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -50 36 %BE%
%B% 7C661C57 %D%/ghoul4.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 160-199 -ofs -29 42 %BE%
%B% 7C661C57 %D%/ghoul5.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -23 35 -flip %BE%
%B% 7C661C57 %D%/ghoul6.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -27 31 -flip %BE%
%B% 7C661C57 %D%/ghoul7.%EXT% -replacecolormap ghoul_palette.tga -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -21 43 -flip %BE%
%B% 7C661C57 %D%/ghoulD.%EXT% -replacecolormap ghoul_palette.tga -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 199-207 -ofs -25 29 %BE%

REM --- ghast ---
echo  Ghast
%B% 7A661C5D %D%/ghast0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-39 -ofs -24 48 %BE% -colormap2nsx 1 14 %colormap_ghast% %CM%
%B% 7A661C5D %D%/ghast1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -35 45 %BE%
%B% 7A661C5D %D%/ghast2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -42 31 %BE%
%B% 7A661C5D %D%/ghast3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -50 36 %BE%
%B% 7A661C5D %D%/ghast4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 160-199 -ofs -29 42 %BE%
%B% 7A661C5D %D%/ghast5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 120-159 -ofs -23 35 -flip %BE%
%B% 7A661C5D %D%/ghast6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 80-119 -ofs -27 31 -flip %BE%
%B% 7A661C5D %D%/ghast7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 40-79 -ofs -21 43 -flip %BE%
%B% 7A661C5D %D%/ghastD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 199-207 -ofs -25 29 %BE%

REM --- brigand ---
echo  Brigand
%B% 7C661C5D %D%/brigand0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-33 -ofs -33 43 %BE% -colormap2nsx 1 14 %colormap_brigand% %CM%
%B% 7C661C5D %D%/brigand1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 34-67 -ofs -51 48 %BE%
%B% 7C661C5D %D%/brigand2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-101 -ofs -54 44 %BE%
%B% 7C661C5D %D%/brigand3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 102-135 -ofs -46 49 %BE%
%B% 7C661C5D %D%/brigand4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-169 -ofs -36 42 %BE%
%B% 7C661C5D %D%/brigand5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 102-135 -ofs -28 49 -flip %BE%
%B% 7C661C5D %D%/brigand6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-101 -ofs -41 44 -flip %BE%
%B% 7C661C5D %D%/brigand7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 34-67 -ofs -38 48 -flip %BE%
%B% 7C661C5D %D%/brigandA.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 170-178 -ofs -15 19 %BE%
%B% 7C661C5D %D%/brigandD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 179-200 -ofs -31 28 %BE%

REM --- brigand merc ---
echo  Brigand mercenary
%B% 7F661C59 %D%/brgmerc0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-33 -ofs -33 43 %BE% -colormap2nsx 1 14 %colormap_brigandmerc% %CM%
%B% 7F661C59 %D%/brgmerc1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 34-67 -ofs -51 48 %BE%
%B% 7F661C59 %D%/brgmerc2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-101 -ofs -54 44 %BE%
%B% 7F661C59 %D%/brgmerc3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 102-135 -ofs -46 49 %BE%
%B% 7F661C59 %D%/brgmerc4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-169 -ofs -36 42 %BE%
%B% 7F661C59 %D%/brgmerc5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 102-135 -ofs -28 49 -flip %BE%
%B% 7F661C59 %D%/brgmerc6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-101 -ofs -41 44 -flip %BE%
%B% 7F661C59 %D%/brgmerc7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 34-67 -ofs -38 48 -flip %BE%
%B% 7F661C59 %D%/brgmercA.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 170-178 -ofs -15 19 %BE%
%B% 7F661C59 %D%/brgmercD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 179-200 -ofs -31 28 %BE%

REM --- brigand with mace ---
echo  Brigand with mace
%B% 7C661C5E %D%/brgmace0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-33 -ofs -30 41 %BE% -colormap2nsx 1 14 %colormap_brigandmace% %CM%
%B% 7C661C5E %D%/brgmace1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 34-67 -ofs -44 45 %BE%
%B% 7C661C5E %D%/brgmace2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-101 -ofs -50 43 %BE%
%B% 7C661C5E %D%/brgmace3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 102-135 -ofs -46 45 %BE%
%B% 7C661C5E %D%/brgmace4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-169 -ofs -35 39 %BE%
%B% 7C661C5E %D%/brgmace5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 102-135 -ofs -24 45 -flip %BE%
%B% 7C661C5E %D%/brgmace6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-101 -ofs -37 43 -flip %BE%
%B% 7C661C5E %D%/brgmace7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 34-67 -ofs -33 45 -flip %BE%
%B% 7C661C5E %D%/brgmaceA.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 170-177 -ofs -19 19 %BE%
%B% 7C661C5E %D%/brgmaceD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 178-200 -ofs -101 68 %BE%

REM --- brigand with mace ---
echo  Mercenary with mace
%B% 7E661C5A %D%/macemrc0.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 0-33 -ofs -30 41 %BE% -colormap2nsx 1 14 %colormap_macemerc% %CM%
%B% 7E661C5A %D%/macemrc1.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 34-67 -ofs -44 45 %BE%
%B% 7E661C5A %D%/macemrc2.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-101 -ofs -50 43 %BE%
%B% 7E661C5A %D%/macemrc3.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 102-135 -ofs -46 45 %BE%
%B% 7E661C5A %D%/macemrc4.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 136-169 -ofs -35 39 %BE%
%B% 7E661C5A %D%/macemrc5.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 102-135 -ofs -24 45 -flip %BE%
%B% 7E661C5A %D%/macemrc6.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 68-101 -ofs -37 43 -flip %BE%
%B% 7E661C5A %D%/macemrc7.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 34-67 -ofs -33 45 -flip %BE%
%B% 7E661C5A %D%/macemrcA.%EXT% -overhead -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 170-177 -ofs -19 19 %BE%
%B% 7E661C5A %D%/macemrcD.%EXT% -oriented -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 -i 178-192 -ofs -101 68 %BE%

REM GOTO EXIT

REM --- SOUNDS ---
echo Sounds...
set D=testinstallation/sound/legacy
%B% 12046C45 %D%/firesword1.wav
%B% 1413141E %D%/firesword2.wav
%B% 564E2D74 %D%/firesword3.wav
%B% 14091300 %D%/magicspike.wav
%B% 141C0A12 %D%/power.wav -speed 1.1
%B% 17101D1F %D%/explode.wav -speed 1.1
%B% 19136C46 %D%/kainpain1.wav
%B% 19136C47 %D%/kainpain2.wav
%B% 1F04141D %D%/kainpain3.wav
%B% 1C136A4D %D%/hpain1.wav
%B% 1C136A4E %D%/hpain2.wav
%B% 1C1B6736 %D%/hdeath1.wav
%B% 45572963 %D%/hdeath2.wav -trimstart 0.03 -speed 1.05
%B% 5A2B7268 %D%/hfdeath1.wav
%B% 5A2B7168 %D%/hfdeath2.wav
%B% 5A2F0E7F %D%/button.wav
%B% 1D15111C %D%/bloodgain.wav -speed 1.3
%B% 24690B7C %D%/wolfpain.wav
%B% 246A0B7C %D%/wolfjump.wav
%B% 246F1779 %D%/hit1.wav -trimstart 0.02 -speed 1.2
%B% 246E1779 %D%/hit2.wav -trimstart 0.013 -speed 1.2
%B% 24691779 %D%/hit3.wav -trimstart 0.013 -speed 1.1
%B% 24701162 %D%/choir.wav -speed 1.1
%B% 270F1664 %D%/switch.wav -trimstart 0.27
%B% 2A0D1C79 %D%/heartbeat.wav -speed 1.1
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
%B% 475A227F %D%/stoneblock.wav
%B% 7D060411 %D%/raise.wav -speed 0.5
%B% 485E3B7F %D%/transform.wav
%B% 495E2B68 %D%/beastdeath1.wav
%B% 7D17131B %D%/beastdeath2.wav
%B% 505A2D69 %D%/bats.wav
%B% 51543C76 %D%/ambient2.wav
%B% 56286765 %D%/mechanic.wav
%B% 5A341272 %D%/slash.wav -speed 1.1

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
