@echo off
REM --- paths ---
set install_path=testinstallation
set destination_path=..\..\kain
set extract=bpill -f -bigfile pill.big -extract
set spr_tool=bpill -f -spr32
set legacy_nsx=testinstallation\script\legacy.nsx
set colormaps_nsx=testinstallation\script\colormaps.nsx
set sprext=spr32
set scale2x=0

REM --- using 2x mode ---
IF "%scale2x%"=="1" (
 set scale_parms=-nearest2x
 set scale_player=0.41
 set scale_monster=0.46
 set scale_death=0.5
 set scale_bigger=0.6
) ELSE (
 set scale_parms=
 set scale_player=0.82
 set scale_monster=0.92
 set scale_death=1.0
 set scale_bigger=1.2
)

REM --- prepare folders --
rmdir testinstallation /Q /S
mkdir %install_path%\
mkdir %install_path%\script
mkdir %install_path%\models
mkdir %install_path%\models\legacy
mkdir %install_path%\sound
mkdir %install_path%\sound\legacy
mkdir %install_path%\sound\cdtracks
set path_models=%install_path%\models\legacy\
set path_sounds=%install_path%\sound\legacy\
set path_music=%install_path%\sound\cdtracks\

REM --- parse install script ---
for /F "tokens=1,2,3,4,5,6 delims=	 eol=#" %%a in (testinstallationscript.txt) do (
	REM -- begin state --
	IF "%%a"=="BEGIN" (
		IF "%%b"=="MODELCONFIG" (
			@echo // Legacy stuff script file >> %legacy_nsx%
			@echo [models]name={type,scale,paletteindex} >> %legacy_nsx%
		)
		IF "%%b"=="COLORMAPCONFIG" (
			@echo // Particle colormaps  file >> %colormaps_nsx%
			@echo [colormaps]index={colormap} >> %colormaps_nsx%
			@echo # colormaps 0-31 are system ones >> %colormaps_nsx%
		)
		@echo %%c... 
	)
	REM -- end state --
	IF "%%a"=="END" (
		IF "%%b"=="COLORMAPCONFIG" (
			@echo # misc colormaps >> %colormaps_nsx%
		)
		@echo %%c... 
	)
	REM -- colormap state command --
	IF "%%a"=="COLORMAP" (
		set colormap_%%c=%%b
		echo %%c;%%b>>%install_path%\colormaps.temp
	)
	REM -- modelconfig state command --
	IF "%%a"=="MDLCFG" (
		for /F "tokens=1,2 delims=;" %%m in (%install_path%\colormaps.temp) do (
			if "%%m"=="%%e" (
				IF "%%b"=="player" (
					@echo %%d=%%c,%scale_player%,%%n >> %legacy_nsx%
				)
				IF "%%b"=="bigger" (
					@echo %%d=%%c,%scale_bigger%,%%n >> %legacy_nsx%
				)
				IF "%%b"=="monster" (
					@echo %%d=%%c,%scale_monster%,%%n >> %legacy_nsx%
				)
				IF "%%b"=="death" (
					@echo %%d=%%c,%scale_death%,%%n >> %legacy_nsx%
				)
			)
		)
	)
	REM -- colormapconfig state command --
	IF "%%a"=="CMPCFG" (
		for /F "tokens=1,2 delims=;" %%m in (%install_path%\colormaps.temp) do (
			if "%%m"=="%%b" (
				@echo %%n=%%c >> %colormaps_nsx%
			)
		)
	)
	REM -- bein model --
	IF "%%a"=="MODEL" (
		@echo  %%b
	)
	REM -- export model --
	IF "%%a"=="XMODEL" (
		IF "%%e"=="XCOLORMAP" (
			for /F "tokens=1,2 delims=;" %%m in (%install_path%\colormaps.temp) do (
				if "%%m"=="%%f" (
					REM - with colormap exporting -
					%extract% %%b %path_models%%%c.%sprext%	%%d -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 %scale_parms% -colormap2nsx %%g %%n
				)
			)
		) ELSE (
			REM - no colormap exporting -
			%extract% %%b %path_models%%%c.%sprext%	%%d -colormapscale 0.8 -bgcolor 080808 -shadowcolor 000000 -shadowalpha 220 %scale_parms%
		)
	)
	REM -- merge model --
	IF "%%a"=="XMERGE" (
		%spr_tool% %path_models%%%b.%sprext% -merge  %path_models%%%c.%sprext%
	)
	REM -- export sound --
	IF "%%a"=="XSOUND" (
		%extract% %%b %path_sounds%%%c	%%d
	)
	REM -- export music --
	IF "%%a"=="XMUSIC" (
		@echo  %%c
		%extract% %%b %path_music%%%c	%%d
	)
	REM -- stop --
	IF "%%a"=="END" (
		GOTO EXIT
	)
)
del %install_path%\colormaps.temp /Q

:EXIT

IF NOT EXIST winrar.exe (
	echo Copy files...
	xcopy %install_path%\* %destination_path%\ /S /E /Q /Y
) ELSE (
	echo Create LEGACY.PK3...
	call winrar.exe a -r -m4 -ep1 "LEGACY.PK3" -- "LEGACY.PK3" %install_path%\*
	@echo on
	move /Y LEGACY.PK3 %destination_path%
)
rmdir %install_path% /Q /S
pause