#include "bloodpill.h"
#include "cmdlib.h"
#include "mem.h"

int BigFile_Main(int argc, char **argv);
int Tim2Targa_Main(int argc, char **argv);
int Targa2Tim_Main(int argc, char **argv);

#include <windows.h>

//
// Help section
//

int Help_Main()
{
	printf(
	"usage: bpill [-w] [-mem] [-nc] action\n"
	" -w : wait for a key press before exit\n"
	" -mem : print a memory usage stats\n"
	" -nc : disable printing of caption\n"
	"\n"
	"=== ACTIONS ===\n"
	"-bigfile [bigfilename] -list [-to filename] [-klist filename]\n"
	"  -to: outputs to external file\n"
	"  -klist: make use of known-files-list\n"
	"   tip: some filetypes like VAG couldn't be detected automatically\n"
	"   so known-files-list is only way to determine them\n"
	"\n"
	"-bigfile [bigfilename] -unpack [-dstdir dir] [-klist filename] [-tim2tga] [-16to24]\n"
	"  unpacks all entries of bigfile and saves listfile\n"
	"    -dstdir: outputs to other folder instead of 'bigfile'\n"
	"    -tim2tga: converts all TIM files to Targa images\n"
	"    -16to24: when write TGA, convert 16 bit colors 24 bit\n"
	"     including colormaps, quite useful because not many tools has support\n"
	"     for 16 Bit TGA's\n"
	"\n"
	"-bigfile [bigfilename] -pack [-srcdir dir]\n"
	"  creates a bigfile from a folder containing all files and listfile\n"
	"    -srcdir: inputs from other folder instead of 'bigfile'\n"
	"\n"
	"-tim2tga timfile [tgafile] [-16to24]\n"
	"  convert TIM image to Truevision TGA\n"
	"  -16to24: convert 16 bit to 24 (including colormap)\n"
	"\n"
	"-tga2tim tgafile [timfile] [-type X]\n"
	"  convert Truevision TGA image to TIM\n"
	"  -type: manually set type of TIM, could be 8, 16, 24\n"
	"\n"
	);
	return 0;
}

//
// Program main()
//

int main(int argc, char **argv)
{
	int i, returncode = 0;
	qboolean printcap;

	// get program name
	ExtractFileBase(argv[0], progname);

	// check command line flags
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
		break;
	}

	// print caption
	if (printcap)
	{
		printf(BLOODPILL_WELCOME);
		if (memstats)
			printf("memstats = true\n");
		if (waitforkey)
			printf("waitforkey = true\n");
		printf( "\n" );
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
	else if (!strcmp (argv[i], "-help"))
		returncode = Help_Main();
	else
		Error("unknown action %s, try %s -help", argv[i], progname);

	printf("\n");

	// print memory stats
	if (memstats)
	{
		printf("=== MemStats ===\n");
		Q_PrintMem();
		printf("\n");
	}

#if _MSC_VER
	if (waitforkey)
	{
		printf("press any key\n");
		getchar();
	}
#endif

	// free allocated memory
	Q_ShutdownMem();

	return returncode;
}