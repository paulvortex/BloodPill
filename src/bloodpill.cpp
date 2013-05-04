////////////////////////////////////////////////////////////////
//
// Blood Pill - utility main
// coded by Pavel [VorteX] Timofeyev and placed to public domain
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////

#include "bloodpill.h"
#include "cmdlib.h"
#include "mem.h"
#include "soxsupp.h"
#include "zlib.h"

// global switches
bool waitforkey;
bool error_waitforkey;
bool memstats;
bool verbose;
bool noprint;
bool solidpacifier;
bool errorlog;

char progname[MAX_OSPATH];
char progpath[MAX_OSPATH];

// bigfile.c
int BigFile_Main(int argc, char **argv);

// timfile.c
int Targa2Tim_Main(int argc, char **argv);
int Tim2Targa_Main(int argc, char **argv);

// rawfile.c
int Raw_Main(int argc, char **argv);

// sprfile.c
int Spr32_Main(int argc, char **argv);

// jamfile.c
int Jam_Main(int argc, char **argv);

// soxsupp.c
int AdpcmConvert_Main(int argc, char **argv);

// script.c
int Script_Main(int argc, char **argv);

// mapfile.c
int MapConvert_Main(int argc, char **argv);

void Print(char *str, ...)
{
	va_list argptr;

	if (noprint)
		return;
	va_start(argptr, str);
	vprintf(str, argptr);
	va_end(argptr);
}

void Verbose(char *str, ...)
{
	va_list argptr;

	if (!verbose || noprint)
		return;
	va_start(argptr, str);
	vprintf(str, argptr);
	va_end(argptr);
}

// flush out after that string 
void PercentPacifier(char *str, ...)
{
	va_list argptr;

	va_start(argptr, str);
	if (solidpacifier)
	{
		vprintf(str, argptr);
		printf("\n");
		va_end(argptr);
		Sleep(20);
	}
	else
	{
		printf("\r");
		vprintf(str, argptr);
		printf("%\r");
		va_end(argptr);
	}
	fflush(stdout);
}

void Pacifier(char *str, ...)
{
	va_list argptr;

	if (noprint)
		return;
	va_start(argptr, str);
	printf("\r");
	vprintf(str, argptr);
	printf("\r");
	va_end(argptr);
	fflush(stdout);
}

void PacifierEnd() 
{
	if (noprint)
		return;
	printf("\n");
}

void Warning(char *str, ...)
{
	va_list argptr;

	va_start(argptr, str);
	printf("Warning: ");
	vprintf(str, argptr);
	va_end(argptr);
	printf("\n");
}


/*
==========================================================================================

  Help section

==========================================================================================
*/

int Help_Main()
{
	Print(
	"usage: bpill [prefixes] action\n"
	"=== PREFIXES ===\n"
	" -w : wait for a key press before exit\n"
	" -ew : wait for a key press once got error (obsoleted by -w)\n"
	" -mem : print a memory usage stats\n"
	" -nc : disable printing of caption\n"
	" -c : compact mode, no verbose messages\n"
	" -f : function mode, only error and warnings get printed\n"
	" -cd x: change current dir to this\n"
	" -sp : print percentage pacifier as newlines, used by installers\n"
	" -errlog: once got error, make berror.txt with it's contents\n"
	"\n"
	"=== ACTIONS ===\n"
	"-bigfile [bigfilename] [-klist file] -list [filename] [optional_switches]\n"
	"  list bigfile contents\n"
	"  OPTIONAL SWITCHES:\n"
	"    -hashasnames: use hashed names instead of trying to unhash them\n"
	"    -klist: use custom known-files-list (default is klist.txt)\n"
	"     TIP: some filetypes like RAW ADPCM couldn't be detected automatically\n"
	"     so known-files-list is only way to determine them\n"
	"\n"
	"-bigfile [bigfilename] [-klist file] -unpack [dir] [optional_switches]\n"
	"  unpacks all entries of bigfile and saves listfile\n" 
    "  OPTIONAL SWITCHES:\n"
	"    -hashasnames: use hashed names instead of trying to unhash them \n"
	"    -tim2tga: converts all TIM files to Targa images\n"
	"    -16to24: when write TGA, convert 16 bit colors 24 bit\n"
	"     including colormaps, quite useful because not many tools has support\n"
	"     for 16 Bit TGA's\n"
	"    -adpcm2wav: convert raw ADPCM to native ADPCM wave\n"
	"    -adpcm2pcm: convert raw ADPCM to PCM wave\n"
	"    -adpcm2ogg: convert raw ADPCM to Ogg Vorbis (quality 5)\n"
	"    -vag2pcm: convert VAG to PCM wave\n"
	"    -vag2ogg: convert VAG to Ogg Vorbis (quality 5)\n"
	"    -raw2tga: convert raw images to readable TGA\n"
	"     WARNING: a lot of files will be extracted! (about 45000)\n"
	"     TIP: unlike TIM->TGA, this is one way raw conversion, backward conversion\n"
	"     on pill.big re-pack is not supported (original raw file will be used)\n"
	"    -rawnoalign: disables width/height aligning of all chunks in RAW images\n"
	"     this makes all chunks of RAW image to be different size, yet smaller\n"
	"     but harder to use in any way\n"
	"    -map2tga: convert Blood Omen maps to TGA images\n"
	"    -mapcontents: use with -map2tga, make map image with content blocks\n"
	"    -maptriggers: use with -map2tga, make map image with script info:\n"
	"                  triggers, speech, activation points, exits etc.\n"
	"    -mapsaveid: show save identifiers for buttons, enemies etc.)\n"
	"    -maptoggled: use with -map2tga, make map image with all switches toggled\n"
	"\n"
	"-bigfile [bigfilename] -pack [dir]\n"
	"  create a bigfile from a folder containing all files and listfile\n"
	"\n"
	"-bigfile [bigfilename] -patch <patchscript> -outfile outfile\n"
	"  patch bigfile with external fiels, patchscript should consist of lines:\n"
	"  RAW/DEL/WAV2ADPCM <filename_or_hash> <path_to_file>\n"
	"   or\n"
	"  <path_to_file>\n"
	"  see patchexample folder\n"
	"\n"
	"\n"
	"-bigfile [bigfilename] -extract filename outfile [-f format] [optional_switches]\n"
	"  extract a specific entry from bigfile\n"
	"  PARMS:\n"
	"   filename: #hash or filename inside pill.big\n"
	"   outfile: output file\n"
	"   format: the format what output file you want, if not set, it will be\n" 
	"           picked from outfile extension. If outfile has no extension,\n"
	"			format will be determined automatically by autoscanner.\n"
	"           A 'raw' format is supported on all filetypes\n"
	"           it tells util to extract file as is."
	"			other formats are:"
	"     wav   - WAVE files (used with sound entries)\n"
	"     ogg   - Ogg Vorbis files (used with sound entries)\n"
	"     tga   - Targa (used with picture entries)\n"
	"     tga24 - Targa 24 bit\n"
	"     tga8_32 - 8-bit with 32 bit palette (requires 8-bit original image)\n"
	"     spr32 - Darkplaces SPR32 format (used with sprites)\n"
	"  OPTIONAL SWITCHES:\n"
	"   for sounds:\n"
	"    -ir X : set input rate in Hz (11025, 22050 etc.)\n"
	"    -trimstart X : trim X seconds at the beginning\n"
	"    -speed X : change speed (like -r but X is multiplier)\n"
	"    -tempo X R : change tempo, X is multiplier, R is inverval of search\n"
	"                 typical value is 0.9 40\n"
	"    -pitch X R : change pitch\n"
	"    -gain X : volume gain\n"
	"    -bass X : bass gain\n"
	"    -treble X : treble gain\n"
	"    -normalize : normalize volume\n"
	"    -reverb : reverbance effect\n"
	"   for sprites:\n"
	"    -i x-y : extract this chunks, may be used several times\n"
	"    -nearest2x : scale to 2x size using nearest filter\n"
	"    -replacecolormap tgafile : replace a chunk colormap with this tga colormap\n"
	"    -ofs x y : offset a center of sprite (spr32 only)\n"
	"    -flip : flip horizontally\n"
	"    -nocrop : disable autocropping to reduce memore usage (spr32 only)\n"
    "    -noalign : chunks alightning (mostly matter for spr32)\n"
	"    -alpha X : set images alpha (spr32 only)\n"
	"    -shadowalpha X : set alpha for shadow pixels (spr32 only)\n"
	"    -shadowcolor XXXXXX : customize shadow pixel color (color is HEX)\n"
	"    -bgcolor XXXXXX : customize background color (color is HEX)\n"
	"   for sprites and TIM images/tilemaps:\n"
	"    -colorscale X : scale colors at 0-1 range\n"
	"    -colorsub X : subtract color values at 0-255 range\n"
	"   for maps:\n"
	"    -t: show triggers (buttons, paths, misc info)\n"
	"    -s: show save identifiers for buttons, enemies etc.\n"
	"    -c: show contents (solid places, special zones)\n"
	"    -a: animated objects (effects, tiles etc.) shown in toggled state\n"
	"   for TIM images and tilemaps:\n"
	"    -scale2x: scale covnerted TGA images with Scale2X filter\n"
	"    -scale4x: scale covnerted TGA images with Scale4X filter\n"
	"\n"
	"-jam jamfile outputdir\n"
	"  Convert JAM movie to number of TGA files\n"
	"\n"
	"-tim2tga timfile [tgafile] [-16to24] \n"
	"  convert TIM image to Truevision TGA\n"
	"  -16to24: convert 16 bit to 24 (including colormap)\n"
	"  -colorscale X : scales color values (0-1 range)\n"
	"  -colorsub X : subtract color values at 0-255 range\n"
	"\n"
	"-tga2tim tgafile [timfile] [-bpp X] [-ofs X Y] [-mask X]\n"
	"  convert Truevision TGA image (or several images) to TIM\n"
	"  -bpp: manually set type of TIM, could be 8 (paletted), 16, 24\n"
	"  -ofs: override TIM image offset\n"
	"  -mask: use a custom maskfile (default is filename_mask.tga)\n"
	"\n"
	"-adpcmconvert adpcmfile [outfile] [-rate X] [-pcm]/[-oggvorbis]/[-custom]\n"
	" convert a Blood Omen raw ADPCM file to WAV/OGG\n"
	" -rate: ADPCM sampling rate, defaults to 22050 if not presented\n"
	" -pcm: make 16-bit PCM wavefile\n"
	" -oggvorbis: make Ogg Vorbis files (Quality 7)\n"
	" -custom: custom SoX output options (see SoX docs)\n"
	"\n"
	"-mapconvert mapfile [outfile] [-t] [-c] [-a] [-tilespath]\n"
	" convert a Blood Omen map to TGA picture\n"
	" -t: show triggers (buttons, paths, misc info)\n"
	" -s: show save identifiers for buttons, enemies etc.\n"
	" -c: show contents (solid places, special zones)\n"
	" -a: animated objects (effects, tiles etc.) shown in toggled state\n"
	" -tilespath path: path to grp/effect original files\n"
	"\n"
	"-raw infile outfile [options]\n"
	" convert Blood Omen internal format images to something viewable\n"
	" GENERAL OPTIONS: \n"
	"   -type: raw type (see Appendix II)\n"
	"   -forcetype: same as -type but with lax error checks\n"
	"   -noswap: don't swap RGB->BGR when writing Targa\n"
	"   -noclut: disable colormap, write indexes instead\n"
	" TYPE 0 OPTIONS: \n"
	"   -width: image width\n"
	"   -height: image height\n"
	"   -offset: file start offset\n"
	"   -colormapoffset: colormap start position in file\n"
	"   -colormapbytes: colormap BPP, could be 2 or 3\n"
    "   -cmprX Y: run-length decode index Y (0-255), X is 1,2,3 or 4\n" 
	" OTHER OPTIONS:\n"
	"   -chunk X: number of chunk to extract (for types 3,4,5,6)\n"
	"   -doubleres: threat file as double-resolution, type 2 only\n"
	"   -noalign: disables width/height aligning of all chunks in RAW images\n"
	"    this makes all chunks of RAW image to be different size, yet smaller\n"
	"    but harder to use in any way\n"
	"\n"
	"=== APPENDIX I: entry types ===\n"
	"here is the list of entry types that could be used in klist:\n"
	" raw - unknown file type (should be autoscanned)\n"
	" tim - PSX texture file\n"
	" adpcm - raw IMA ADPCM, voice data\n"
	" wav - PCM Riff Wave files\n"
	" rsp - sprite files"
	" vag - PSX 'Very Audio Good' compressed sound\n"
	" ctm - Compressed TIM (tiles)\n"
	" cmp - Compressed map\n"
	"\n"
	"=== APPENDIX II: sprites ===\n"
	" type0 - special type for debugging, should have width, height, \n"
	"         bytes offset and color bytes. Well suited for 'data massaging'\n"
	" type1 - item cards, single-object files\n"
	" type2 - multiobject file with per-object colormaps\n"
	" type3 - view-parallel sprites (multiobjects, shared colormap)\n"
	" type4 - mostly oriented sprites (monsters)\n"
	" type5 - misc sprites\n"
	"\n");
	return 0;
}

/*
==========================================================================================

  Program main

==========================================================================================
*/

int main(int argc, char **argv)
{
	int i, j, returncode = 0;
	char customsoxpath[MAX_OSPATH];
	bool printcap;

	// get program name
	memset(progname, 0, MAX_OSPATH);
	memset(progpath, 0, MAX_OSPATH);
	ExtractFileBase(argv[0], progname);
	ExtractFilePath(argv[0], progpath);

	// check command line flags
	sprintf(customsoxpath, "sox.exe");
	verbose = true;
	printcap = true;
	waitforkey = false;
	memstats = false;
	noprint = false;
	solidpacifier = false;
	errorlog = false;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i],"-nc")) // disable caption
		{
			printcap = false;
			continue;
		}
		if (!strcmp(argv[i], "-c")) // disable partial printings
		{
			verbose = false;
			printcap = false;
			continue;
		}
		if (!strcmp(argv[i],"-f")) // disable all printings
		{
			verbose = false;
			printcap = false;
			noprint = true;
			continue;
		}
		if (!strcmp(argv[i],"-mem"))
		{
			memstats = true;
			continue;
		}
		if (!strcmp(argv[i], "-w"))
		{
			waitforkey = true;
			continue;
		}
		if (!strcmp(argv[i], "-ew"))
		{
			error_waitforkey = true;
			continue;
		}
		if (!strcmp(argv[i], "-cd"))
		{
			i++;
			if (i < argc)
				ChangeDirectory(argv[i]);
			continue;
		}
		if (!strcmp(argv[i], "-sp"))
		{
			solidpacifier = true;
			continue;
		}
		if (!strcmp(argv[i], "-errlog"))
		{
			errorlog = true;
			continue;
		}
		if (!strcmp(argv[i],"-testcmd"))
		{
			printf("Commandline parms test:\n");
			for (j = 0; j < argc; j++)
				printf("%i : '%s'\n", j, argv[j]);
			printf("---\n");
			continue;
		}
		if (!strcmp(argv[i], "-soxpath"))
		{
			i++;
			if (i < argc)
				sprintf(customsoxpath, "%s", argv[i]);
			continue;
		}
		break;
	}

	// init SoX library
	SoX_Init(customsoxpath);
	PK3_OpenLibrary(false);

	// print caption
	if (printcap)
	{
		Print(BLOODPILL_WELCOME, BLOODPILL_VERSION);
		if (memstats)
			Print("memstats = true\n");
		if (waitforkey)
			Print("waitforkey = true\n");
		if (soxfound)
			Print("SoX found\n");
		else
			Print("SoX not found\n");
		if (PK3_Enabled())
			Print("Zlib found\n");
		else
			Print("Zlib not found\n");
		Print( "\n" );
	}

	// no args check
	if (i >= argc)
	{
		waitforkey = true;
		Help_Main ();
		Error  ("bad commandline" , progname );
	}

	// init memory
	Mem_Init();

	// do the action
	if (!strcmp(argv[i], "-bigfile"))
		returncode = BigFile_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-script"))
		returncode = Script_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-tim2tga"))
		returncode = Tim2Targa_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-tga2tim"))
		returncode = Targa2Tim_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-raw"))
		returncode = Raw_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-adpcmconvert"))
		returncode = AdpcmConvert_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-spr32"))
		returncode = Spr32_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-jam"))
		returncode = Jam_Main(argc-i, argv+i);
	else if (!strcmp (argv[i], "-mapconvert"))
		returncode = MapConvert_Main(argc-i, argv+i);
	else if (!strcmp (argv[i], "-help"))
		returncode = Help_Main();
	else
		Error("unknown action %s, try %s -help", argv[i], progname);
	Print("\n");

	// free allocated memory
	Mem_Shutdown();
	PK3_CloseLibrary();

#if _MSC_VER
	if (waitforkey)
	{
		Print("press any key\n");
		getchar();
	}
#endif

	return returncode;
}