#include "bloodpill.h"
#include "bigfile.h"
#include "cmdlib.h"
#include "mem.h"

char bigfile[1024];

int BigFile_List(int argc, char **argv)
{
	FILE *f;
	bigfileheader_t *data;

	printf("Opening %s...\n", bigfile);
	f = fopen(bigfile, "r");
	if (!f)
		Error("BigFile: couldn't read %s\n", bigfile);

	// load header
	data = qmalloc(sizeof(bigfileheader_t));
	if (fscanf (f,"%u\n", data->numentries) != 1)
		Error("BigFile: failed to read headers\n");
	if (!data->numentries)
		Error("BigFile: funny entries count, perharps file is broken\n");
	printf("%i entries\n", data->numentries);

	// load enties
	data->entries = qmalloc(data->numentries * sizeof(bigfileentry_t));

	fclose (f);
	return 0;
}

int BigFile_Main(int argc, char **argv)
{
	int i = 1, returncode = 0;
	char *c;

	printf("=== BigFile ===\n");

	// get input file
	c = argv[i];
	if (c[0] != '-')
	{
		strcpy(bigfile, c);
		i++;
	}
	else
		strcpy(bigfile, "pill.big");

	// no args check
	if (argc < i + 1)
		Error("no action specified, try %s -help", progname);

	// action
	if (!strcmp(argv[i], "-list"))
		returncode = BigFile_List(argc-i, argv+i);
	else
		printf("unknown action %s", argv[i]);

	return returncode;
}