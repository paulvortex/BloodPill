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

char progname[128];

// bigfile.c
int BigFile_Main(int argc, char **argv);

// timfile.c
int Targa2Tim_Main(int argc, char **argv);
int Tim2Targa_Main(int argc, char **argv);

// rawfile.c
int Raw_Main(int argc, char **argv);

// soxsupp.c
int VagConvert_Main(int argc, char **argv);

void Print(char *str, ...)
{
	va_list argptr;

	va_start(argptr, str);
	vprintf(str, argptr);
	va_end(argptr);
}

void Verbose(char *str, ...)
{
	va_list argptr;

	if (!verbose)
		return;
	va_start(argptr, str);
	vprintf(str, argptr);
	va_end(argptr);
}

// flush out after that string 
void Pacifier(char *str, ...)
{
	va_list argptr;

	va_start(argptr, str);
	printf("\r");
	vprintf(str, argptr);
	printf("\r");
	va_end(argptr);
	fflush(stdout);
}

void PacifierEnd() 
{
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
	"usage: bpill [-w] [-mem] [-nc] action\n"
	" -w : wait for a key press before exit\n"
	" -mem : print a memory usage stats\n"
	" -nc : disable printing of caption\n"
	" -c : compact mode, no verbose messages\n"
	"\n"
	"=== ACTIONS ===\n"
	"-bigfile [bigfilename] -list [-to filename] [optional_switches]\n"
	"  list bigfile contents\n"
	"  OPTIONAL SWITCHES:\n"
	"    -to: outputs to external file\n"
	"    -klist: use custom known-files-list (default is klist.txt)\n"
	"     TIP: some filetypes like RAW ADPCM couldn't be detected automatically\n"
	"     so known-files-list is only way to determine them\n"
	"    -scanraw: extensively scan to detect RAW filetypes (sprites, tiles etc.)\n"
	"    -csv filename: export CSV listfile which could be used by Wheel Of Doom\n"
	"\n"
	"-bigfile [bigfilename] -unpack [-dstdir dir] [optional_switches]\n"
	"  unpacks all entries of bigfile and saves listfile\n" 
    "  OPTIONAL SWITCHES:\n"
	"    -klist filename: use custom known-files-list\n"
	"    -dstdir: outputs to other folder instead of 'bigfile'\n"
	"    -tim2tga: converts all TIM files to Targa images\n"
	"    -16to24: when write TGA, convert 16 bit colors 24 bit\n"
	"     including colormaps, quite useful because not many tools has support\n"
	"     for 16 Bit TGA's\n"
	"    -vagconvert: enables VAG->WAV conversion, wav will use ADPCM encoding\n"
	"    -pcm: use with -vagconvert to make 16-bit PCM wavefiles\n"
	"    -oggvorbis: use with -vagconvert to make Ogg Vorbis files (Q7)\n"
	"    -scanraw: extensively scan to detect RAW filetypes (sprites, tiles etc.)\n"
	"    -rawconvert: convert raw images to readable TGA\n"
	"     WARNING: a lot of files will be extracted! (about 45000)\n"
	"     TIP: unlike TIM->TGA, this is one way raw conversion, backward conversion\n"
	"     on pill.big re-pack is not supported (original raw file will be used)\n"
	"    -forcerawtype [rawtype]: threat all raw images to this type\n"
	"     also this disables extra safe checks for autoscanner/exporter\n"
	"     so it can find more of this types (but they tend be wrong detected\n"
	"     and buggy at all). Helps to find out obfuscated/funky files\n"
	"     that shall be identificated with known-files-list.\n"
	"     TIP: For list of possible raw types see APPENDIX II\n"
	"    -noalign: disables width/height aligning of all chunks in RAW images\n"
	"     this makes all chunks of RAW image to be different size, yet smaller\n"
	"     but harder to use in any way\n"
	"\n"
	"-bigfile [bigfilename] -pack [-srcdir dir] [-lowmem]\n"
	"  create a bigfile from a folder containing all files and listfile\n"
	"    -srcdir: inputs from other folder instead of 'bigfile'\n"
	"    -lowmem: use smaller amount of memory but a bit slower\n"
	"\n"
	"-tim2tga timfile [tgafile] [-16to24] \n"
	"  convert TIM image to Truevision TGA\n"
	"  -16to24: convert 16 bit to 24 (including colormap)\n"
	"\n"
	"-tga2tim tgafile [timfile] [-bpp X] [-ofs X Y] [-mask X]\n"
	"  convert Truevision TGA image (or several images) to TIM\n"
	"  -bpp: manually set type of TIM, could be 8 (paletted), 16, 24\n"
	"  -ofs: override TIM image offset\n"
	"  -mask: use a custom maskfile (default is filename_mask.tga)\n"
	"\n"
	"-vagconvert vagfile [outfile] [-rate X] [-pcm]/[-oggvorbis]/[-custom]\n"
	" convert a Blood Omen headerless VAG file to someting that WinAmp could play\n"
	" -rate: VAG sampling rate, defaults to 22050 if not presented\n"
	" -pcm: make 16-bit PCM wavefile\n"
	" -oggvorbis: make Ogg Vorbis files (Quality 7)\n"
	" -custom: custom SoX output options (see SoX docs)\n"
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
	"   -bytes: image bits-per-pixel, could be 1, 2, 3\n"
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
	" dat - unknown file type (should be autoscanned)\n"
	" tim - PSX texture file\n"
	" adpcm - raw IMA ADPCM, voice data\n"
	" wav - PCM Riff Wave files\n"
	" raw - raw image files (tiles, sprites etc.)\n"
	" vag - PSX 'Very Audio Good' compressed sound\n"
	"\n"
	"=== APPENDIX II: raw image types ===\n"
	" type0 - special type for debugging, should have width, height, \n"
	"         bytes offset and color bytes. Well suited for 'data massaging'\n"
	" type1 - item cards, single-object files\n"
	" type2 - multiobject file with per-object colormaps\n"
	" type3 - view-parallel sprites (multiobjects, shared colormap)\n"
	" type4 - mostly oriented sprites (monsters)\n"
	" type5 - misc sprites\n"
	" type6 - hybrid type4/type5 type (oriented sprites)\n"
	" type7 - tiles (not yet completed)\n"
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
	int i, returncode = 0;
	char customsoxpath[MAX_BLOODPATH];
	qboolean printcap;

	// get program name
	memset(progname, 0, 128);
	ExtractFileBase(argv[0], progname);

	// check command line flags
	sprintf(customsoxpath, "sox.exe");
	verbose = true;
	printcap = true;
	waitforkey = false;
	memstats = false;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i],"-nc")) // disable caption
		{
			printcap = false;
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
		if (!strcmp(argv[i], "-c"))
		{
			verbose = false;
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
		Print( "\n" );
	}

	// no args check
	if (argc == 1)
	{
		waitforkey = true;
		Help_Main ();
		Error  ("bad commandline" , progname );
	}

	// init memory
	Q_InitMem();

	// do the action
	if (!strcmp(argv[i], "-bigfile"))
		returncode = BigFile_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-tim2tga"))
		returncode = Tim2Targa_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-tga2tim"))
		returncode = Targa2Tim_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-raw"))
		returncode = Raw_Main(argc-i, argv+i);
	else if (!strcmp(argv[i], "-vagconvert"))
		returncode = VagConvert_Main(argc-i, argv+i);
	else if (!strcmp (argv[i], "-help"))
		returncode = Help_Main();
	else
		Error("unknown action %s, try %s -help", argv[i], progname);

	Print("\n");

	// print memory stats
	if (memstats)
	{
		Print("=== MemStats ===\n");
		Q_PrintMem();
		Print("\n");
	}

#if _MSC_VER
	if (waitforkey)
	{
		Print("press any key\n");
		getchar();
	}
#endif

	// free allocated memory
	Q_ShutdownMem();

	return returncode;
}