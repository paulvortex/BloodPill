////////////////////////////////////////////////////////////////
//
// Bigfile operations
// coded by Pavel [VorteX] Timofeyev and placed to public domain
// thanks to XentaX (www.xentax.com) community for providing bigfile specs
//
////////////////////////////////

#include "bloodpill.h"
#include "bigfile.h"
#include "cmdlib.h"
#include "mem.h"

#define DEFAULT_BIGFILENAME	"pill.big"
#define DEFAULT_PACKPATH	"bigfile"

char bigfile[MAX_BLOODPATH];

// knowledge base
typedef struct
{
	unsigned int hash;
	int vagrate;
	bigentrytype_t type;
}
bigkentry_t;

typedef struct
{
	int numentries;
	bigkentry_t *entries;
}
bigklist_t;

bigklist_t *bigklist;

/*
==========================================================================================

  KNOWN-FILES FEATURE

==========================================================================================
*/

bigentrytype_t BigfileTypeForExt(char *ext)
{
	int i, numbytes;

	Q_strlower(ext);
	numbytes = strlen(ext);

	// null-length ext?
	if (!numbytes)
		return BIGENTRY_UNKNOWN;

	// find type
	for (i = 0; i < BIGFILE_NUM_FILETYPES; i++)
		if (!memcmp(bigentryext[i], ext, numbytes))
			return i;

	return BIGENTRY_UNKNOWN;
}

bigklist_t *BigfileLoadKList(char *filename, qboolean stopOnError)
{
	bigklist_t *klist;
	int linenum = 0;
	char line[256], typestr[256];
	unsigned int hash;
	int parm1, parms, i;
	FILE *f;

	// create empty klist
	klist = qmalloc(sizeof(bigklist_t));
	klist->entries = NULL;
	klist->numentries = 0;

	if (stopOnError)
		f = SafeOpen(filename, "r");
	else
	{
		f = fopen(filename, "r");
		if (!f)
			return klist;
	}
	// first pass - scan klist to determine how many strings we should allocate
	while(!feof(f))
	{
		fgets(line, 1024, f);
		if (line[0] == '#')
			continue;
		linenum++;
	}

	// allocate
	klist->entries = qmalloc(linenum * sizeof(bigkentry_t));

	// seconds pass - parse klist
	fseek(f, 0, SEEK_SET);
	linenum = 0;
	while(!feof(f))
	{
		linenum++;
		fgets(line, 1024, f);
		if (line[0] == '#')
			continue;

		// scan
		parms = sscanf(line,"%X=%s %i", &hash, typestr, &parm1);
		if (parms < 2)
		{
			printf("%s: parse error on line %i: %s", filename, linenum, line);
			continue;
		}
		klist->entries[klist->numentries].hash = hash;
		klist->entries[klist->numentries].type = BigfileTypeForExt(typestr);

		// warn for double defienition
		for (i = 0; i < klist->numentries; i++)
			if (klist->entries[i].hash == hash)
				printf("warning: redefenition of hash %.8X on line %i\n", hash, linenum);

		// VAG - sampling rate
		if (klist->entries[klist->numentries].type == BIGENTRY_RAW_VAG)
			klist->entries[klist->numentries].vagrate = (parms < 3) ? 11025 : parm1;

		// parsed
		klist->numentries++;
	}

	printf("loaded known-files list with %i entries\n", klist->numentries);

	return klist;
}

bigkentry_t *BigfileSearchKList(unsigned int hash)
{
	int i;

	for (i = 0; i < bigklist->numentries; i++)
		if (bigklist->entries[i].hash == hash)
			return &bigklist->entries[i];

	return NULL;
}

/*
==========================================================================================

  BigFile subs

==========================================================================================
*/

void BigfileEmptyEntry(bigfileentry_t *entry)
{
	entry->data = NULL;
}

void BigfileSeekFile(FILE *f, bigfileentry_t *entry)
{
	if (fseek(f, (long int)entry->offset, SEEK_SET))
		Error( "error seeking for data on file %.8X", entry->hash);
}

void BigfileSeekContents(FILE *f, byte *contents, bigfileentry_t *entry)
{
	if (fseek(f, (long int)entry->offset, SEEK_SET))
		Error( "error seeking for data on file %.8X", entry->hash);

	if (fread(contents, entry->size, 1, f) < 1)
		Error( "error reading data on file %.8X (%s)", entry->hash, strerror(errno));
}

void BigFileUnpackFile(FILE *f, bigfileentry_t *entry, FILE *dstf)
{
	byte *contents;

	contents = (byte *)qmalloc(entry->size);
	BigfileSeekContents(f, contents, entry);
	fwrite(contents, 1, entry->size, dstf);
	qfree(contents);
}

void BigfileWriteListfile(FILE *f, bigfileheader_t *data)
{
	bigfileentry_t *entry;
	int i, k;

	if (f != stdout)
		fprintf(f, "numentries=%i\n", data->numentries);
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		// write general data
		fprintf(f, "\n", entry->hash);
		fprintf(f, "[%.8X]\n", entry->hash);
		fprintf(f, "type=%i\n", (int)entry->type);
		fprintf(f, "size=%i\n", (int)entry->size);
		fprintf(f, "offset=%i\n", (int)entry->offset);
		fprintf(f, "file=%s\n", entry->name);

		// write specific data for TIM images
		switch(entry->type)
		{
			case BIGENTRY_TIM:
				fprintf(f, "tim.layers=%i\n", entry->timlayers);
				for(k = 0; k < entry->timlayers; k++)
				{
					fprintf(f, "tim[%i].type=%i\n", k, entry->timtype[k]);
					fprintf(f, "tim[%i].xskip=%i\n", k, entry->timxpos[k]);
					fprintf(f, "tim[%i].yskip=%i\n", k, entry->timypos[k]);
				}
				break;
			case BIGENTRY_RAW_VAG:
				fprintf(f, "vag.rate=%i\n", entry->vagrate);
				break;
			default:
				break;
		}
	}
}

bigfileheader_t *ReadBigfileHeader(FILE *f, char *filename, qboolean loadfilecontents)
{	
	bigfileheader_t *data;
	bigfileentry_t *entry;
	unsigned int read[3];
	int i;

	data = qmalloc(sizeof(bigfileheader_t));

	// read header
	if (fread(&data->numentries, sizeof(unsigned int), 1, f) < 1)
		Error("BigfileHeader: wrong of broken file\n");
	if (!data->numentries)
		Error("BigfileHeader: funny entries count, perhaps file is broken\n");
	printf("%s: %i entries\n", filename, data->numentries);

	// read entries
	data->entries = qmalloc(data->numentries * sizeof(bigfileentry_t));
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];
		BigfileEmptyEntry(entry);

		printf("\rreading entry %i of %i", i + 1, data->numentries);
		fflush(stdout);

		if (fread(&read, 12, 1, f) < 1)
			Error("BigfileHeader: error on entry %i (%s)\n", i, strerror(errno));
		entry->hash = read[0];
		entry->size = read[1];
		entry->offset = read[2];
		entry->type = BIGENTRY_UNKNOWN;
		sprintf(entry->name, "%.8X.%s", read[0], bigentryext[BIGENTRY_UNKNOWN]);
		if (!entry->hash || !entry->offset)
			Error("BigfileHeader: entry %i is broken\n", i);
	}
	printf("\n");

	// load contents
	if (loadfilecontents)
	{
		for (i = 0; i < (int)data->numentries; i++)
		{
			entry = &data->entries[i];
			if (entry->size <= 0)
				continue;

			printf("\rloading entry %i of %i", i + 1, data->numentries);
			fflush(stdout);

			entry->data = qmalloc(entry->size);
			BigfileSeekContents(f, entry->data, entry);
		}
		printf("\n");
	}

	// warnings
	/*
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];
		if (entry->size <= 0)
			printf("warning: entry %.8X size = %i bytes\n", entry->hash, entry->size);
	}
	*/

	return data;
}

// recalculate all file offsets
void BigfileHeaderRecalcOffsets(bigfileheader_t *data)
{
	bigfileentry_t *entry;
	int i, offset;

	offset = sizeof(unsigned int) + data->numentries*12;
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];
		entry->offset = (unsigned int)offset;
		offset = offset + entry->size;
	}
}

// check & fix entry that was loaded from listfile
// this functino laso does autoconvert job
void BigfileFixListfileEntry(char *srcdir, bigfileentry_t *entry, qboolean lowmem)
{
	char ext[16], filename[MAX_BLOODPATH], basename[MAX_BLOODPATH];
	tim_image_t *tim;
	int i;
	FILE *f;

	// extract extension
	ExtractFileBase(entry->name, basename);
	ExtractFileExtension(entry->name, ext);
	Q_strlower(ext);

	// standart file
	if (!strcmp(ext, bigentryext[entry->type]))
	{
		sprintf(filename, "%s/%s", srcdir, entry->name);
		f = SafeOpen(filename, "rb");
		entry->data = NULL; // load as-is
		entry->size = (unsigned int)Q_filelength(f);
		fclose(f);
		return;
	}

	// TGA -> TIM autoconversion
	if (!strcmp(ext, "tga") && entry->type == BIGENTRY_TIM)
	{
		// main tim
		entry->size = 0;
		if (!entry->timlayers)
			Error("bad TIM layer info for entry %.8X", entry->hash);
		for (i = 0; i < entry->timlayers; i++)
		{
			if (i == 0)
				sprintf(filename, "%s/%s", srcdir, entry->name);
			else
				sprintf(filename, "%s/%s_layer%i.tga", srcdir, basename, i);
			f = SafeOpen(filename, "rb");
			tim = TIM_LoadFromTarga(f, entry->timtype[i]); 
			if (tim->error)
				Error("conversion error on %s layer %i: %s", entry->name, i, tim->errorstr);

			entry->size += (unsigned int)tim->filelen;
			if (i == 0)
				entry->data = tim;
			if (i != 0 || lowmem) // VorteX: -lomem key support, read that again later
				FreeTIM(tim);
			fclose(f);
		}

		return;
	}

	Error("can't identify file %s", entry->name);

}

// print stats about loaded bigfile entry
void BigfileEmitStats(bigfileheader_t *data)
{
	bigfileentry_t *entry;
	int stats[BIGFILE_NUM_FILETYPES], timstats[4];
	int i;

	// calc stats
	memset(stats, 0, sizeof(int)*BIGFILE_NUM_FILETYPES);
	memset(timstats, 0, sizeof(int)*4);
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		if (entry->type == BIGENTRY_TIM)
		{
			if (entry->timtype[0] == TIM_4Bit)
				timstats[0]++;
			else if (entry->timtype[0] == TIM_8Bit)
				timstats[1]++;
			else if (entry->timtype[0] == TIM_16Bit)
				timstats[2]++;	
			else if (entry->timtype[0] == TIM_24Bit)
				timstats[3]++;
		}
		stats[entry->type]++;
	}

	// print
	printf(" %6i 4-bit TIM\n", timstats[0]);
	printf(" %6i 8-bit TIM\n", timstats[1]);
	printf(" %6i 16-bit TIM\n", timstats[2]);
	printf(" %6i 24-bit TIM\n", timstats[3]);
	printf(" %6i TIM total\n", stats[BIGENTRY_TIM]);
	printf(" %6i RAW VAG\n", stats[BIGENTRY_RAW_VAG]);
	printf(" %6i RIFF WAVE\n", stats[BIGENTRY_RIFF_WAVE]);
	printf(" %6i unknown\n", stats[BIGENTRY_UNKNOWN]);
	printf(" %6i TOTAL\n", data->numentries);
}

// read bigfile header from listfile
bigfileheader_t *BigfileOpenListfile(char *srcdir, qboolean lowmem)
{
	bigfileheader_t *data;
	bigfileentry_t *entry;
	char line[256], temp[128], filename[MAX_BLOODPATH];
	int numentries, linenum, val, num;
	unsigned int uval;
	short valshort;
	FILE *f;

	// open file
	sprintf(filename, "%s/listfile.txt", srcdir);
	f = SafeOpen(filename, "r");

	// read number of entries
	data = qmalloc(sizeof(bigfileheader_t));
	if (fscanf(f, "numentries=%i\n", &numentries) != 1)
		Error("broken numentries record");
	printf("%s: %i entries\n", filename, numentries);

	// read all entries
	linenum = 1;
	entry = NULL;
	data->entries = qmalloc(numentries * sizeof(bigfileentry_t));
	data->numentries = 0;
	while(!feof(f))
	{
		linenum++;
		fgets(line, 256, f);

		// new entry
		if (line[0] == '[')
		{
			if (sscanf(line, "[%X]", &val) < 1)
				Error("bad entry definition on line %i: %s\n", linenum, line);

			// check old entry
			if (entry != NULL)
				BigfileFixListfileEntry(srcdir, entry, lowmem);

			if ((int)data->numentries >= numentries)
				Error("entries overflow, numentries is out of date\n");

			entry = &data->entries[data->numentries];
			BigfileEmptyEntry(entry);
			entry->hash = (unsigned int)val;
			data->numentries++;
			
			printf("\rreading entry %i of %i", data->numentries, numentries);
			fflush(stdout);
			continue;
		}

		// scan parameter
		if (entry == NULL)
			Error("Entry data without actual entry on line %i: %s\n", linenum, line);

		// parse base parms
		if (sscanf(line, "type=%i", &val))
			entry->type = (bigentrytype_t)val;
		else if (sscanf(line, "size=%i", &val))
			entry->size = (int)val;
		else if (sscanf(line, "offset=%i", &val))
			entry->offset = (int)val;
		else if (sscanf(line, "file=%s", &temp))
			strcpy(entry->name, temp);
		// for TIM
		else if (sscanf(line, "tim[%i].type=%i", &num, &uval))
			entry->timtype[num] = uval;
		else if (sscanf(line, "tim[%i].xpos=%f", &num, &valshort))
			entry->timxpos[num] = valshort;
		else if (sscanf(line, "tim[%i].ypos=%f", &num, &valshort))
			entry->timypos[num] = valshort;
		else if (sscanf(line, "tim.layers=%i", &val))
			entry->timlayers = val;
		// for VAG
		else if (sscanf(line, "vag.rate=%i", &val))
			entry->vagrate = val;
	}
	printf("\n");

	// check last entry
	if (entry != NULL)
		BigfileFixListfileEntry(srcdir, entry, lowmem);

	// emit some ststs
	BigfileEmitStats(data);

	// recalc offsets
	BigfileHeaderRecalcOffsets(data);

	return data;
}


/*
==========================================================================================

  BigFile filetype scanner

==========================================================================================
*/

qboolean BigFileScanTIM(FILE *f, bigfileentry_t *entry)
{
	tim_image_t *tim;
	fpos_t fpos;
	unsigned int tag;
	unsigned int bpp;
	int bytes;

	// VorteX: Blood Omen has *weird* TIM files - they could be 2 or more TIM's in one file
	// they could be easily detected however, as second TIM goes right after base TIM 

	BigfileSeekFile(f, entry);

	bytes = 0;
	entry->timlayers = 0;
	while(1)
	{
		fgetpos(f, &fpos);

		// 0x10 should be at beginning of standart TIM
		if (fread(&tag, sizeof(unsigned int), 1, f) < 1)
			return (entry->timlayers != 0) ? true : false;
		if (tag != 0x10)
			return (entry->timlayers != 0) ? true : false;

		// second uint is BPP
		// todo: there are files with TIM header but with nasty BPP
		if (fread(&bpp, sizeof(unsigned int), 1, f) < 1)
			return (entry->timlayers != 0) ? true : false;
		if (bpp != TIM_4Bit && bpp != TIM_8Bit && bpp != TIM_16Bit && bpp != TIM_24Bit) 
			return (entry->timlayers != 0) ? true : false;

		// try load that TIM
		fsetpos(f, &fpos);
		tim = TIM_LoadFromStream(f);
		if (tim->error)
		{
			FreeTIM(tim);
			return false;
		}
		bytes += tim->filelen;

		// fill diminfo section
		if (entry->timlayers >= (MAX_TIM_MASKS + 1))
			Error("TIM layers overflow on entry %.8X", entry->hash);

		entry->timtype[entry->timlayers] = tim->type;
		entry->timxpos[entry->timlayers] = tim->dim.xpos;
		entry->timypos[entry->timlayers] = tim->dim.ypos;
		entry->timlayers++;
		FreeTIM(tim);

		if (bytes >= (int)entry->size)
			break;
	}

	return true;
}

qboolean BigFileScanRiffWave(FILE *f, bigfileentry_t *entry)
{
	byte tag[4];

	BigfileSeekFile(f, entry);

	// first unsigned int - tag
	if (fread(&tag, sizeof(char), 4, f) < 1)
		return false;
	if (tag[0] != 0x52 || tag[1] != 0x49 || tag[2] != 0x46 || tag[3] != 0x46)
		return false;

	// it's a RIFF
	return true;
}

void BigfileScanFiletypes(FILE *f,bigfileheader_t *data)
{
	fpos_t fpos;
	bigfileentry_t *entry;
	bigkentry_t *kentry;
	int i;

	fgetpos(f, &fpos);
	// scan for filetypes
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		printf("\rscanning type for entry %i of %i", i + 1, data->numentries);
		fflush(stdout);
		
		// scan for certain filetype
		kentry = BigfileSearchKList(entry->hash); // check for known filetype
		if (kentry != NULL)
		{
			entry->type = kentry->type;
			entry->vagrate = kentry->vagrate;
		}
		else if (BigFileScanTIM(f, entry))
			entry->type = BIGENTRY_TIM;
		else if (BigFileScanRiffWave(f, entry))
			entry->type = BIGENTRY_RIFF_WAVE;
		else
			entry->type = BIGENTRY_UNKNOWN;

		// make name
		sprintf(entry->name, "%.8X.%s", entry->hash, bigentryext[entry->type]);
	}
	fsetpos(f, &fpos);
	printf("\n");
	
	// emit some stats
	BigfileEmitStats(data);
}


/*
==========================================================================================

  BigFile analyser

==========================================================================================
*/

typedef struct
{
	unsigned int data;
	int occurrences;
}
bigchunk4_t;

typedef struct
{
	unsigned int data;
	int occurrences;
}
bigchunk8_t;

typedef struct
{
	// unsigned int chunks
	bigchunk4_t chunks4[2048];
	byte chunk4;
	int numchunks4;
}
bigchunkstats_t;

int BigFile_Analyse(int argc, char **argv, char *outfile)
{
	FILE *f;
	bigfileheader_t *data;
	bigfileentry_t *entry;
	bigchunkstats_t *chunkstats;
	int i, k;

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	data = ReadBigfileHeader(f, bigfile, false);
	BigfileScanFiletypes(f, data);

	// analyse headers
	chunkstats = (bigchunkstats_t *)qmalloc(sizeof(bigchunkstats_t));
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];
		if (entry->type != BIGENTRY_UNKNOWN)
			continue;
		printf("\ranalysing entry %i of %i", i + 1, data->numentries);
		fflush(stdout);

		// chunk4 stats
		BigfileSeekFile(f, entry);
		if (fread(&chunkstats->chunk4, sizeof(unsigned int), 1, f) > 0)
		{
			// try find chunk
			for (k = 0; k < chunkstats->numchunks4; k++)
			{
				if (chunkstats->chunks4[k].data != chunkstats->chunk4)
					continue;
				chunkstats->chunks4[k].occurrences++;
				break;
			}
			if (k >= chunkstats->numchunks4) // not found, allocate new
			{
				chunkstats->chunks4[chunkstats->numchunks4].data = chunkstats->chunk4;
				chunkstats->chunks4[chunkstats->numchunks4].occurrences = 1;
				chunkstats->numchunks4++;
			}
		}
	}
	printf("\n");

	// print stats
	printf("=== Chunk (unsigned int) ===\n");
	printf("  occurence threshold = 4\n");
	for (i = 0; i < chunkstats->numchunks4; i++)
		if (chunkstats->chunks4[i].occurrences > 4)
			printf("  %.8X = %i occurences\n", chunkstats->chunks4[i].data, chunkstats->chunks4[i].occurrences);

	fclose(f);
	qfree(chunkstats);

	return 0;

}

/*
==========================================================================================

  Actions

==========================================================================================
*/

int BigFile_List(int argc, char **argv, char *listfile)
{
	FILE *f, *f2;
	bigfileheader_t *data;

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	data = ReadBigfileHeader(f, bigfile, false);
	BigfileScanFiletypes(f, data);

	// print or...
	if (listfile[0] == '-')
		BigfileWriteListfile(stdout, data);
	else // output to file
	{
		f2 = SafeOpen(listfile, "w");
		BigfileWriteListfile(f2, data);
		printf("wrote %s\n", listfile);
		fclose(f2);
	}
	printf("done.\n");

	fclose (f);

	return 0;
}


int BigFile_Unpack(int argc, char **argv, char *dstdir, qboolean tim2tga, qboolean bpp16to24)
{
	FILE *f, *f2;
	char savefile[MAX_BLOODPATH];
	bigfileheader_t *data;
	bigfileentry_t *entry;
	tim_image_t *tim;
	int i, k;

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	data = ReadBigfileHeader(f, bigfile, false);
	BigfileScanFiletypes(f, data);

	// make directory
	printf("%s folder created\n", dstdir);
	Q_mkdir(dstdir);

	if (tim2tga)
		printf("TIM->TGA conversion enabled\n");
	if (bpp16to24)
		printf("Targa compatibility mode enabled (converting 16-bit to 24-bit)\n");

	// export all files
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		// original pill.big has 'funky' files with zero len, export them as empty ones
		if (entry->size <= 0) 
		{
			sprintf(savefile, "%s/%.8X.%s", dstdir, entry->hash, bigentryext[entry->type]);
			f2 = SafeOpen(savefile, "wb");
			fclose(f2);
			continue;
		}

		printf("\runpacking file %i of %i", i + 1, data->numentries);
		fflush(stdout);

		// extract TGA
		if (tim2tga && entry->type == BIGENTRY_TIM)
		{
			BigfileSeekFile(f, entry);
			for (k = 0; k < entry->timlayers; k++)
			{
				tim = TIM_LoadFromStream(f);
				if (k == 0)
					sprintf(savefile, "%s/%.8X.tga", dstdir, entry->hash);
				else
					sprintf(savefile, "%s/%.8X_layer%i.tga", dstdir, entry->hash, k);

				if (tim->error)
					Error("error saving %s: %s\n", savefile, tim->error);
			
				sprintf(entry->name, "%.8X.tga", entry->hash); // write 'good' listfile.txt
				TIM_WriteTarga(tim, savefile, bpp16to24);
				FreeTIM(tim);
			}
			continue;
		}

		// extract original file
		sprintf(savefile, "%s/%.8X.%s", dstdir, entry->hash, bigentryext[entry->type]);
		f2 = SafeOpen(savefile, "wb");
		BigFileUnpackFile(f, entry, f2);
		fclose(f2);
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

int BigFile_Pack(int argc, char **argv, char *srcdir, qboolean lowmem)
{
	FILE *f, *f2;
	bigfileheader_t *data;
	bigfileentry_t *entry;
	tim_image_t *tim;
	char savefile[MAX_BLOODPATH], basename[MAX_BLOODPATH];
	byte *contents;
	int i, k, size;

	data = BigfileOpenListfile(srcdir, lowmem);

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

		printf("\rwriting header %i of %i", i + 1, data->numentries);
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

		// if file is already loaded
		if (entry->data != NULL)
		{
			if (entry->type == BIGENTRY_TIM)
			{
				// VorteX: -lomem key support
				if (lowmem)
				{
					sprintf(savefile, "%s/%s", srcdir, entry->name);
					f2 = SafeOpen(savefile, "rb");
					tim = TIM_LoadFromTarga(f2, entry->timtype[0]); 
					if (tim->error)
						Error("conversion error on %s: %s", entry->name, tim->errorstr);
					fclose(f2);
					entry->data = tim;
				}
				TIM_WriteToStream(entry->data, f);
				FreeTIM(entry->data);

				// add sublayers
				ExtractFileBase(entry->name, basename);
				for (k = 1; k < entry->timlayers; k++)
				{
					sprintf(savefile, "%s/%s_layer%i.tga", srcdir, basename, k);
					f2 = SafeOpen(savefile, "rb");
					tim = TIM_LoadFromTarga(f2, entry->timtype[k]); 
					if (tim->error)
						Error("conversion error on %s: %s", entry->name, tim->errorstr);
					fclose(f2);
					TIM_WriteToStream(tim, f);
					FreeTIM(tim);
				}
			}
			else
			{
				fwrite(entry->data, entry->size, 1, f);
				qfree(entry->data);
			}
		}
		// read file from HDD
		else
		{
			sprintf(savefile, "%s/%s", srcdir, entry->name);
			size = LoadFile(savefile, &contents);
			if (size != (int)entry->size)
				Error("entry %.8X: file size changed (%s%i bytes, newsize %i) while packing\n", entry->hash, (size - (int)entry->size) < 0 ? "-" : "+", size - (int)entry->size, size);
			fwrite(contents, size, 1, f);
			qfree(contents);
		}
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
	char *tofile, *srcdir, *dstdir, *knownfiles, *c;
	qboolean tim2tga, bpp16to24, lowmem;

	printf("=== BigFile ===\n");
	if (i < 1)
		Error("not enough parms");

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
	knownfiles = qmalloc(MAX_BLOODPATH);
	strcpy(tofile, "-");
	strcpy(dstdir, DEFAULT_PACKPATH);
	strcpy(srcdir, DEFAULT_PACKPATH);
	strcpy(knownfiles, "-");
	tim2tga = false;
	bpp16to24 = false;
	lowmem = false;
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
		else if (!strcmp(argv[k],"-klist"))
		{
			k++;
			if (k < argc)
				strcpy(knownfiles, argv[k]);
		}
		else if (!strcmp(argv[k],"-tim2tga"))
			tim2tga = true;
		else if (!strcmp(argv[k],"-16to24"))
			bpp16to24 = true;
		else if (!strcmp(argv[k],"-lowmem"))
			lowmem = true;
	}
	
	// load up knowledge base
	if (knownfiles[0] == '-')
		bigklist = BigfileLoadKList("klist.txt", true);
	else
		bigklist = BigfileLoadKList(knownfiles, false);

	// action
	if (!strcmp(argv[i], "-list"))
		returncode = BigFile_List(argc-i, argv+i, tofile);
	else if (!strcmp(argv[i], "-analyse"))
		returncode = BigFile_Analyse(argc-i, argv+i, tofile);
	else if (!strcmp(argv[i], "-unpack"))
		returncode = BigFile_Unpack(argc-i, argv+i, dstdir, tim2tga, bpp16to24);
	else if (!strcmp(argv[i], "-pack"))
		returncode = BigFile_Pack(argc-i, argv+i, srcdir, lowmem);
	else
		printf("unknown option %s", argv[i]);

	return returncode;
}