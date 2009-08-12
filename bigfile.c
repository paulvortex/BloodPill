#include "bloodpill.h"
#include "bigfile.h"
#include "cmdlib.h"
#include "mem.h"

#define DEFAULT_BIGFILENAME	"pill.big"
#define DEFAULT_PACKPATH	"bigfile"

char bigfile[MAX_BLOODPATH];

/*
==========================================================================================

  BigFile subs

==========================================================================================
*/

void BigfileSeekContents(FILE *f, byte *contents, bigfileentry_t *entry)
{
	if (fseek(f, entry->offset, SEEK_SET))
		Error( "error seeking for data on file %.8X", entry->hash);

	if (fread(contents, entry->size, 1, f) < 1)
		Error( "error reading data on file %.8X (%s)", entry->hash, strerror(errno));
}

void BigfileWriteListfile(FILE *f, bigfileheader_t *data)
{
	bigfileentry_t *entry;
	int i;

	if (f != stdout)
		fprintf(f, "numentries %i\n", data->numentries);
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];
		fprintf(f, "%i size %i offset %i type %i name %s\n", entry->hash, entry->size, entry->offset, entry->type, entry->name);
	}
}

bigfileheader_t *ReadBigfileHeaderFromListfile(FILE *f)
{
	bigfileheader_t *data;
	bigfileentry_t *entry;
	int i;

	fseek(f, 0, SEEK_SET);

	// read number of entries
	data = qmalloc(sizeof(bigfileheader_t));
	if (fscanf(f, "numentries %i\n", &data->numentries) != 1)
		Error("broken numentries record");
	printf("%i entries\n", data->numentries);

	// read all entries
	data->entries = qmalloc(data->numentries * sizeof(bigfileentry_t));
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		printf("\rreading entry %i of %i", i + 1, data->numentries);
		fflush(stdout);

		if (fscanf(f, "%i size %i offset %i type %i name %s\n", &entry->hash, &entry->size, &entry->offset, &entry->type, &entry->name) != 5)
			Error("broken entry record %i", i+1);
	}
	printf("\n");

	return data;
}

bigfileheader_t *ReadBigfileHeader(FILE *f, qboolean scanforfiletypes)
{	
	bigfileheader_t *data;
	bigfileentry_t *entry;
	fpos_t fpos;
	byte *contents;
	unsigned int read[3];
	int i;

	data = qmalloc(sizeof(bigfileheader_t));

	// read header
	if (fread(&data->numentries, sizeof(unsigned int), 1, f) < 1)
		Error("BigfileHeader: wrong of broken file\n");
	if (!data->numentries)
		Error("BigfileHeader: funny entries count, perhaps file is broken\n");
	printf("%i entries\n", data->numentries);

	// read entries
	data->entries = qmalloc(data->numentries * sizeof(bigfileentry_t));
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		printf("\rreading entry %i of %i", i + 1, data->numentries);
		fflush(stdout);

		if (fread(&read, 12, 1, f) < 1)
			Error("BigfileHeader: error on entry %i (%s)\n", i, strerror(errno));
		entry->hash = read[0];
		entry->size = read[1];
		entry->offset = read[2];
		entry->type = BIGENTRY_UNKNOWN;
		sprintf(entry->name, "%.8X%s", read[0], bigentryext[BIGENTRY_UNKNOWN]);
		if (!entry->hash || !entry->offset)
			Error("BigfileHeader: entry %i is broken\n", i);
	}

	printf("\n");

	if (!scanforfiletypes)
		return data;

	fgetpos(f, &fpos);

	// scan for filetypes
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		printf("\rscanning type for entry %i of %i", i + 1, data->numentries);
		fflush(stdout);

		// seek file contents
		contents = (byte *)qmalloc(entry->size);
		BigfileSeekContents(f, contents, entry);
		qfree(contents);
	}

	fsetpos(f, &fpos);
	printf("\n");

	return data;
}


/*
==========================================================================================

  Actions

==========================================================================================
*/

int BigFile_List(int argc, char **argv, char *listfile)
{
	FILE *f;
	bigfileheader_t *data;

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	printf("%s opened\n", bigfile);
	data = ReadBigfileHeader(f, true);
	fclose (f);

	// print or...
	if (listfile[0] == '-')
		BigfileWriteListfile(stdout, data);
	else // output to file
	{
		f = SafeOpen(listfile, "w");
		BigfileWriteListfile(f, data);
		printf("wrote %s\n", listfile);
		fclose(f);
	}
	printf("done.\n");

	return 0;
}

int BigFile_Unpack(int argc, char **argv, char *dstdir)
{
	FILE *f, *f2;
	char savefile[MAX_BLOODPATH];
	bigfileheader_t *data;
	bigfileentry_t *entry;
	byte *contents;
	int i;

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	printf("%s opened\n", bigfile);
	data = ReadBigfileHeader(f, true);

	// make directory
	printf("%s folder created\n", dstdir);
	Q_mkdir(dstdir);

	// export all files
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		printf("\runpacking file %i of %i", i + 1, data->numentries);
		fflush(stdout);

		// open
		sprintf(savefile, "%s/%.8X%s", dstdir, entry->hash, bigentryext[entry->type]);
		f2 = SafeOpen(savefile, "wb");

		// read contents
		contents = (byte *)qmalloc(entry->size);
		BigfileSeekContents(f, contents, entry);

		// write contents
		fwrite(contents, 1, entry->size, f2);
		fclose(f2);
		qfree(contents);
	}
	printf("\n");

	// write listfile
	sprintf(savefile, "%s/listfile.txt", dstdir);
	f2 = SafeOpen(savefile, "w");
	BigfileWriteListfile(f2, data);
	fclose(f2);
	printf("wrote %s\n", savefile);
	printf("done.\n");

	fclose (f);
	return 0;
}

int BigFile_Pack(int argc, char **argv, char *srcdir)
{
	FILE *f, *f2;
	bigfileheader_t *data;
	bigfileentry_t *entry;
	char openpath[MAX_BLOODPATH], savefile[MAX_BLOODPATH];
	byte *contents;
	int filelen, i;

	// open list file
	sprintf(openpath, "%s/listfile.txt", srcdir);
	f = SafeOpen(openpath, "r");
	printf("%s opened\n", openpath);

	// read list file to header
	data = ReadBigfileHeaderFromListfile(f);
	fclose(f);

	// check header
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		printf("\rchecking file %i of %i", i + 1, data->numentries);
		fflush(stdout);

		// open file
		sprintf(savefile, "%s/%s", srcdir, entry->name);
		f2 = SafeOpen(savefile, "r");

		// check length
		filelen = Q_filelength(f2);
		if (filelen != (int)entry->size)
		{
			printf("notice: file %s differs in size with listfile (+%i bytes), fixing\n", savefile, (filelen - (int)entry->size));
			data->entries[i].size = (unsigned int)filelen;
		}

		// offset gaps not yet supported
		if (i > 0)
		{
			if ((data->entries[i-1].size + data->entries[i-1].offset) != entry->offset)
				Error("offset gap detected on entry %i", i);
		}
	
		fclose(f2);
	}
	printf("\n");

	// open bigfile
	f = fopen(bigfile, "rb");
	if (f)
	{
		printf("%s already exists, overwriting\n", bigfile);
		fclose(f);
	}
	f = SafeOpen(bigfile, "wb");

	// write header
	fwrite(&data->numentries, 4, 1, f);
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		printf("\rwriting entry header %i of %i", i + 1, data->numentries);
		fflush(stdout);
	
		fwrite(&entry->hash, 4, 1, f);
		fwrite(&entry->size, 4, 1, f);
		fwrite(&entry->offset, 4, 1, f);
	}
	printf("\n");

	// write files
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		printf("\rwriting entry %i of %i", i + 1, data->numentries);
		fflush(stdout);

		contents = (byte *)qmalloc(entry->size);
		sprintf(savefile, "%s/%s", srcdir, entry->name);
		f2 = SafeOpen(savefile, "rb");
		fread(contents, entry->size, 1, f2);
		fclose(f2);
		fwrite(contents, entry->size, 1, f);
		qfree(contents);
	}
	printf("\n");

	fclose (f);
	printf("done.\n");

	return 0;
}


/*
==========================================================================================

  Main

==========================================================================================
*/

int BigFile_Main(int argc, char **argv)
{
	int i = 1, k, returncode = 0;
	char *tofile, *srcdir, *dstdir;
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

	// args check
	if (argc < i + 1)
		Error("no action specified, try %s -help", progname);

	// parse cmdline
	tofile = qmalloc(MAX_BLOODPATH);
	srcdir = qmalloc(MAX_BLOODPATH);
	dstdir = qmalloc(MAX_BLOODPATH);
	strcpy(tofile, "-");
	strcpy(dstdir, DEFAULT_PACKPATH);
	strcpy(srcdir, DEFAULT_PACKPATH);
	for (k = 2; k < argc; k++)
	{
		if (!strcmp(argv[k],"-to"))
		{
			k++;
			if (k < argc)
				strcpy(tofile, argv[k]);
		}
		else if (!strcmp(argv[k],"-dstdir"))
		{
			k++;
			if (k < argc)
				strcpy(dstdir, argv[k]);
		}
		else if (!strcmp(argv[k],"-srcdir"))
		{
			k++;
			if (k < argc)
				strcpy(srcdir, argv[k]);
		}
	}

	// action
	if (!strcmp(argv[i], "-list"))
		returncode = BigFile_List(argc-i, argv+i, tofile);
	else if (!strcmp(argv[i], "-unpack"))
		returncode = BigFile_Unpack(argc-i, argv+i, dstdir);
	else if (!strcmp(argv[i], "-pack"))
		returncode = BigFile_Pack(argc-i, argv+i, srcdir);
	else
		printf("unknown option %s", argv[i]);

	return returncode;
}