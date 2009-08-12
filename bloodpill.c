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
	"usage: %s [-w] [-mem] [action]\n"
	" -w : wait for a key press before exit\n"
	" -mem : print a memory usage stats\n"
	"\nactions:\n"
	" -bigfile [bigfilename] -list [-to filename]\n"
	"    list contents of bigfile\n"
	"    if bigfilename is not set, it will be default 'pill.big'\n"
	"    [-to filename] outputs to external file\n",
	progname);
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
		if (!strcmp(argv[i],"-w"))
		{
			printf("waitforkey = true\n");
			waitforkey = true;
			continue;
		}
		break;
	}
	printf("\n");

	// no args check
	if (argc == 1)
	{
		waitforkey = true;
		Help_Main ();
		Error("bad commandline", progname);
	}

	// init memory
	Q_InitMem();

	// do the action
	if (!strcmp(argv[i], "-bigfile"))
		returncode = BigFile_Main (argc-i, argv+i);
	else if (!strcmp (argv[i], "-help"))
		returncode = Help_Main ();
	else
		Error("unknown action %s, try %s -help", argv[i], progname);

	printf("\n");

	// print memory stats
	if (memstats)
		Q_PrintMem();

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