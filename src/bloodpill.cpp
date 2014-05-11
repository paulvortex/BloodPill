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
	"    HELP PAGE\n"
	"----------------------------------------\n"
	"1.1 Usage:\n"
	"----------------------------------------\n"
	"    bpill options action\n"
	"\n"
	"1.2 Options:\n"
	"----------------------------------------\n"
	"    -w : wait for a key press before exit\n"
	"    -ew : wait for a key press once got error (obsoleted by -w)\n"
	"    -mem : print a memory usage stats\n"
	"    -nc : disable printing of caption\n"
	"    -c : compact mode, no verbose messages\n"
	"    -f : function mode, only error and warnings get printed\n"
	"    -cd x: change current dir to this\n"
	"    -sp : print percentage pacifier as newlines, used by installers\n"
	"    -errlog: write berror.txt on error\n"
	"\n"
	"1.3 Action list:\n"
	"----------------------------------------\n"
	"    -bigfile: operate with bigfile (see ch.2)\n"
	"    -jam: convert JAM movie to Truevision Targa (TGA) files (see ch.3)\n"
	"    -tim2tga: convert TIM image to TGA file (see ch.4)\n"
	"    -tga2tim: convert TGA image (or several images) to TIM (see ch.5)\n"
	"    -adpcmconvert: convert a Blood Omen raw ADPCM file to WAV/OGG (see ch.6)\n"
	"    -mapconvert: convert a Blood Omen map to TGA picture (see ch.7)\n"
	"    -raw: convert other Blood Omen internal format images (see ch.8)\n"
	"\n"
	"2.1 Operating with bigfile:\n"
	"----------------------------------------\n"
	"    Usage: bpill -bigfile bigfilename parameters command\n"
	"    Bigfilename: optional name of bigfile (default is pill.big)\n"
	"    Command: one of bigfile commands (see ch.2.3)\n"
	"    Parameters:\n"
	"      -klist file: optional, path to known-file-list file\n"
	"\n"
	"2.2 Known-file-list:\n"
	"----------------------------------------\n"
	"    Can override a default klist.txt with -klist file switch\n"
	"    The reason known-file-list exist, is that some filetypes like RAW ADPCM\n"
	"    couldn't be detected automatically. So known-files-list is only way\n"
	"    to determine them.\n"
	"    Example known-file-list file is stored with blood pill.\n"
	"    Possible file types is listed in ch.8.2\n"
	"\n"
	"2.3 Bigfile commands:\n"
	"----------------------------------------\n"
	"    -list filename: print bigfile contents\n"
	"    -unpack dir: unpacks all entries of bigfilen\n"
	"    -pack dir: packs all entries back to a new bigfile\n"
	"    -patch script: patch bigfile\n"
	"    -extract filename: extract single entry from bigfile\n"
	"\n"
	"2.3.1 List bigfile contents:\n"
	"----------------------------------------\n"
	"    Usage: bpill -bigfile bigfilename -list filename parameters\n"
	"    Filename: if set, a new text file with listing will be created\n"
	"    Parameters:\n"
	"      -hashasnames: use hashed names instead of trying to unhash them\n"
	"\n"
	"2.3.2 Unpack bigfile\n"
	"----------------------------------------\n"
	"    Usage: bpill -bigfile bigfilename -unpack dir parameters\n"
	"    Dir: optional output directory (default is bigfile)\n"
	"    Parameters:\n"
	"      -hashasnames: use hashed names instead of trying to unhash them\n"
	"      -tim2tga: converts all TIM files to Targa images\n"
	"      -16to24: when write TGA, convert 16 bit colors 24 bit\n"
	"               including colormaps, useful because not many tools\n"
	"               supports support for 16 Bit TGA's\n"
	"      -adpcm2wav: convert raw ADPCM to native ADPCM wave\n"
	"      -adpcm2pcm: convert raw ADPCM to PCM wave\n"
	"      -adpcm2ogg: convert raw ADPCM to Ogg Vorbis (quality 5)\n"
	"      -vag2pcm: convert VAG to PCM wave\n"
	"      -vag2ogg: convert VAG to Ogg Vorbis (quality 5)\n"
	"      -raw2tga: convert raw images to readable TGA\n"
	"                WARNING: a lot of files will be extracted! (about 45000)\n"
	"                TIP: unlike TIM->TGA, this is one way raw conversion\n"
	"                     backward conversion on pill.big re-pack is not supported\n"
	"      -rawnoalign: disables width/height aligning of all chunks in RAW images\n"
	"                   this makes all chunks of RAW image to be different size,\n"
	"                   yet smaller but harder to use in external programs\n"
	"      -map2tga: convert Blood Omen maps to TGA images\n"
	"      -mapcontents: use with -map2tga, make map image with content blocks\n"
	"      -maptriggers: use with -map2tga, make map image with script info:\n"
	"                    triggers, speech, activation points, exits etc.\n"
	"      -mapsaveid: show save identifiers for buttons, enemies etc.)\n"
	"      -maptoggled: use with -map2tga, make map image with all switches toggled\n"
	"\n"
	"2.3.3 Pack bigfile\n"
	"----------------------------------------\n"
	"    Usage: bpill -bigfile bigfilename -pack dir\n"
	"    Bigfilename: this bigfile will be created/overwritten (optional, see 2.1)\n"
	"    Dir: optional input directory, default is bigfile\n"
	"\n"
	"2.3.4 Patch bigfile\n"
	"----------------------------------------\n"
	"    Usage: bpill -bigfile bigfilename -patch scriptname parameters\n"
	"    Scriptname: a text file that must consist of such lines:\n"
	"       RAW filename_or_hash path_to_input_file : replace a raw file\n"
	"       WAV2ADPCM filename_or_hash path_to_input_file : replace a speech\n"
	"       DEL filename_or_hash : delete file\n"
	"       path_to_input_file : replace a raw file (shoul matchunhashed file name);\n"
	"       See samples/patch/ folder for an example of patch script\n"
	"    Parameters:\n"
	"      -outfile filename: generate a new bigfile insted of modifying existing\n"
	"\n"
	"2.3.5 Extract entry from bigfile\n"
	"----------------------------------------\n"
	"    Usage: bpill -bigfile bigfilename -extract filename outfile parameters\n"
	"    Filename: #hash or file name of entry inside bigfile\n"
	"    Outfile: path of output file\n"
	"    Parameters:\n"
	"      -f format: the format what output file you want, default is estimated\n"
	"               from outfile extension. If outfile has no extension,\n"
	"               format will be determined automatically by autoscanner.\n"
	"               Possible formats:"
	"               raw - extract file as is\n"
	"               wav - WAVE files (used with sound entries)\n"
	"               ogg   - Ogg Vorbis files (used with sound entries)\n"
	"               tga   - Targa (used with picture entries)\n"
	"               tga24 - Targa 24 bit\n"
	"               tga8_32 - 8-bit with 32b palette\n"
	"               spr32 - Darkplaces SPR32 format (used with sprites)\n"
	"    Parameters for sound files:\n"
	"      -ir X: set input rate in Hz (11025, 22050 etc.)\n"
	"      -trimstart X: trim X seconds at the beginning\n"
	"      -speed X: change speed (like -r but X is multiplier)\n"
	"      -tempo X R: change tempo, X is multiplier, R is inverval of search\n"
	"                  typical value is 0.9 40\n"
	"      -pitch X R: change pitch\n"
	"      -gain X: volume gain\n"
	"      -bass X: bass gain\n"
	"      -treble X: treble gain\n"
	"      -normalize: normalize volume\n"
	"      -reverb: reverbance effect\n"
	"    Parameters for sprites:\n"
	"      -i x-y: extract this chunks, may be used several times\n"
	"      -nearest2x: scale to 2x size using nearest filter\n"
	"      -replacecolormap tgafile: replace a chunk colormap with this tga colormap\n"
	"      -ofs x y: offset a center of sprite (spr32 only)\n"
	"      -flip: flip horizontally\n"
	"      -nocrop: disable autocropping to reduce memore usage (spr32 only)\n"
    "      -noalign: chunks alightning (mostly matter for spr32)\n"
	"      -alpha X: set images alpha (spr32 only)\n"
	"      -shadowalpha X: set alpha for shadow pixels (spr32 only)\n"
	"      -shadowcolor XXXXXX: customize shadow pixel color (color is HEX)\n"
	"      -bgcolor XXXXXX: customize background color (color is HEX)\n"
	"    Parameters for sprites and TIM images/tilemaps:\n"
	"      -colorscale X: scale colors at 0-1 range\n"
	"      -colorsub X :subtract color values at 0-255 range\n"
	"    Parameters for maps:\n"
	"      -t: show triggers (buttons, paths, misc info)\n"
	"      -s: show save identifiers for buttons, enemies etc.\n"
	"      -c: show contents (solid places, special zones)\n"
	"      -a: animated objects (effects, tiles etc.) shown in toggled state\n"
	"    Parameters for TIM images and tilemaps:\n"
	"      -scale2x: scale covnerted TGA images with Scale2X filter\n"
	"      -scale4x: scale covnerted TGA images with Scale4X filter\n"
	"\n"
	"3.1 Convert JAM movie to TGA files\n"
	"----------------------------------------\n"
	"    Usage: bpill -jam jamfile outputdir\n"
	"    Jamfile: input .JAM file\n"
	"    Outputdir: path to output directory\n"
	"\n"
	"4.1 Convert TIM image to TGA file\n"
	"----------------------------------------\n"
	"    Usage: bpill -tim2tga timfile tgafile [parameters]\n"
	"    Timfile: input TIM file\n"
	"    Tgafile: optional path to output TGA file\n"
	"    Parameters:\n"
	"      -16to24: convert 16 bit to 24 (including palette)\n"
	"      -colorscale X: scales color values (0-1 range)\n"
	"      -colorsub X: subtract color values at 0-255 range\n"
	"\n"
	"5.1 Convert TGA image (or several images) to TIM\n"
	"----------------------------------------\n"
	"    Usage: bpill -tga2tim tgafile timfile [parameters]\n"
	"    Tgafile: path to TGA file\n"
	"    Timfile: optional output TIM file name\n"
	"    Parameters:\n"
	"      -bpp X: manually set type of TIM, could be 8 (paletted), 16, 24\n"
	"      -ofs X Y: override TIM image offset\n"
	"      -mask X: use a custom maskfile (default is filename_mask.tga)\n"
	"\n"
	"6.1 Convert a Blood Omen raw ADPCM file to WAV/OGG\n"
	"----------------------------------------\n"
	"    Usage: bpill -adpcmconvert adpcmfile outfile [parameters]\n"
	"    Adpcmfile: path to source file\n"
	"    Outfile: optional name of output file\n"
	"    Parameters:\n"
	"      -rate X: ADPCM sampling rate, defaults to 22050\n"
	"      -pcm: make 16-bit PCM wavefile\n"
	"      -oggvorbis: make Ogg Vorbis files (Quality 5)\n"
	"      -custom: custom SoX output options (see SoX docs)\n"
	"\n"
	"7.1 Convert a Blood Omen map to TGA picture\n"
	"----------------------------------------\n"
	"    Usage: bpill -mapconvert mapfile outfile parameters\n"
	"    Mapfile: path to source file\n"
	"    Outfile: optional path for output file\n"
	"    Parameters:\n"
	"      -t: show triggers (buttons, paths, misc info)\n"
	"      -s: show save identifiers for buttons, enemies etc.\n"
	"      -c: show contents (solid places, special zones)\n"
	"      -a: animated objects (effects, tiles etc.) shown in toggled state\n"
	"      -tilespath path: path to grp/effect original files\n"
	"\n"
	"8.1 Convert other Blood Omen internal format images\n"
	"-----\n"
	"    Usage: bpill -raw infile outfile parameters\n"
	"    Infile: path to source file\n"
	"    Outfile: path to destination file\n"
	"    Parameters:\n"
	"      -type X: raw type (see 8.3)\n"
	"      -forcetype: same as -type but with lax error checks\n"
	"      -noswap: don't swap RGB->BGR when writing Targa\n"
	"      -noclut: disable colormap, write indexes instead\n"
	"    Parameters for type 0:\n"
	"      -width: image width\n"
	"      -height: image height\n"
	"      -offset: file start offset\n"
	"      -colormapoffset: colormap start position in file\n"
	"      -colormapbytes: colormap BPP, could be 2 or 3\n"
    "      -cmprX Y: run-length decode index Y (0-255), X is 1,2,3 or 4\n" 
	"    Parameters for SDR/SHD files:\n"
	"      -chunk X: number of chunk to extract (for types 3,4,5,6)\n"
	"      -doubleres: threat file as double-resolution, type 2 only\n"
	"      -noalign: disables width/height aligning of all chunks in RAW images\n"
	"                this makes all chunks of RAW image to be different size,\n"
	"                yet smaller but harder to use in external apps\n"
	"\n"
	"8.2 Raw file types\n"
	"----------------------------------------\n"
	"    raw: unknown file type (should be autoscanned)\n"
	"    tim: PS1 texture file\n"
	"    adpcm: raw IMA ADPCM, voice data\n"
	"    wav: PCM Riff Wave files\n"
	"    rsp: sprite files (SDR/SHD)"
	"    vag: PS1 'Very Audio Good' compressed sound\n"
	"    ctm: Compressed TIM (tiles)\n"
	"    cmp: Compressed map\n"
	"\n"
	"8.3 Sprite file types\n"
	"----------------------------------------\n"
	"    type0: special type for debugging, should have -width, -height, \n"
	"           -bytes offset and colormap parms set.\n"
	"    type1: item cards, single-object files\n"
	"    type2: multiobject file with per-object colormaps\n"
	"    type3: view-parallel sprites (multiobjects, shared colormap)\n"
	"    type4: mostly oriented sprites (monsters)\n"
	"    type5: misc sprites\n"
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