#include "bloodpill.h"
#include "cmdlib.h"
#include "mem.h"

int BigFile_Main(int argc, char **argv);

//
// Help section
//

int Help_Main()
{
	printf(
	"usage: bpill [-w] [-mem] action\n"
	" -w : wait for a key press before exit\n"
	" -mem : print a memory usage stats\n"
	"\n"
	"=== ACTIONS ===\n"
	"-bigfile [bigfilename] -list [-to filename] [-klist filename]\n"
	"  -to: outputs to external file\n"
	"  -klist: make use of known-files-list\n"
	"   tip: some filetypes like VAG couldn't be detected\n"
	"   automatically by a tool, so known-files-list is only way\n"
	"   to determine them\n"
	"\n"
	"-bigfile [bigfilename] -unpack [-dstdir dir] [-klist filename]\n"
	"  unpacks all entries of bigfile and saves listfile\n"
	"    -dstdir: outputs to other folder instead of 'bigfile'\n"
	"\n"
	"-bigfile [bigfilename] -pack [-srcdir dir]\n"
	"  creates a bigfile from a folder containing all files and listfile\n"
	"    -srcdir: inputs from other folder instead of 'bigfile'\n"
	);
	return 0;
}

//
// Program main()
//

int main(int argc, char **argv)
{
	int i, returncode = 0;

	// print welcome string
	printf(BLOODPILL_WELCOME);

	// get program name
	ExtractFileBase(argv[0], progname);

	// check command line flags
	waitforkey = false;
	memstats = false;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i],"-mem"))
		{
			printf("memstats = true\n");
			memstats = true;
			continue;
		}
		if (!strcmp(argv[i], "-w"))
		{
			printf("waitforkey = true\n");
			waitforkey = true;
			continue;
		}
		break;
	}
	printf( "\n" );

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