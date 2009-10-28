////////////////////////////////////////////////////////////////
//
// Bigfile operations
// coded by Pavel [VorteX] Timofeyev and placed to public domain
// thanks to XentaX (www.xentax.com) community for providing bigfile specs
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
#include "bigfile.h"
#include "soxsupp.h"
#include "cmdlib.h"
#include "mem.h"
#include <windows.h>

#define DEFAULT_BIGFILENAME	"pill.big"
#define DEFAULT_PACKPATH	"bigfile"

char bigfile[MAX_BLOODPATH];

// knowledge base
typedef struct
{
	unsigned int hash;

	int adpcmrate; // adpcm rate
	char path[MAX_BLOODPATH]; // a path to extract
	bigentrytype_t type; // a type of entry
	rawinfo_t *rawinfo; // raw format info
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

  UTIL FUNCTIONS

==========================================================================================
*/

bigentrytype_t ParseBigentryTypeFromExtension(char *ext)
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

bigentrytype_t ParseBigentryType(char *str)
{
	bigentrytype_t bigtype;

	// file type by ext
	bigtype = ParseBigentryTypeFromExtension(str);
	if (bigtype != BIGENTRY_UNKNOWN)
		return bigtype;

	// special keywords
	if (!strcmp(str, "unknown") || !strcmp(str, "data"))
		return BIGENTRY_UNKNOWN;
	if (!strcmp(str, "wave") || !strcmp(str, "riff") || !strcmp(str, "riffwave"))
		return BIGENTRY_RIFF_WAVE;
	return BIGENTRY_UNKNOWN;
}

char *UnparseBigentryType(bigentrytype_t bigtype)
{
	return bigentryext[bigtype];
}

/*
==========================================================================================

  CHECK IF BIGFILE ENTRY MATCHES LIST

==========================================================================================
*/

// #hash
// $type
// wildcard - *
// whether to match include-exclude list
qboolean MatchIXList(bigfileentry_t *entry, list_t *list, qboolean matchtypes, qboolean matchnames)
{
	char *buf, *name;
	unsigned int hash;
	int skip;
	int i;

	for (i = 0; i < list->items; i++)
	{
		buf = list->item[i];
		// hashname compare
		if (buf[0] == '#')
		{
			buf++;
			sscanf(buf, "%X", &hash);
			if (entry->hash == hash)
				return (list->x[i]) ? true : false;
			continue;
		}
		// type compare
		if (buf[0] == '$')
		{
			if (matchtypes)
			{
				buf++;
				if (entry->type == ParseBigentryType(buf))
					return (list->x[i]) ? true : false;
			}
			continue;
		}
		// wildcard compare
		if (!matchnames)
			continue;
	
		skip = false;
		name = entry->name;
		while(name)
		{
			if (buf[0] == '*')
			{
				skip = -1;
				buf++; if (!buf[0]) break;
			}
			else if (buf[0] == '?')
			{
				skip = 1;
				buf++; if (!buf[0]) break;
			}
			// * were in previous try, skip all symbols until find match
			if (skip)
			{
				if (skip != -1) // one char skip
					name++;
				else 
				{
					while(name[0] && name[0] != buf[0])
						name++;
				}
				if (!name[0]) break;
				name++;
				buf++; if (!buf[0]) break;
				skip = 0;
				continue;
			}
			// check a char and continue
			if (name[0] != buf[0]) break;
			name++;
			buf++; if (!buf[0]) break;
		}
		if (!name[0] || skip == -1) // passed
			return (list->x[i]) ? true : false;
	}
	return false;
}

/*
==========================================================================================

  KNOWN-FILES FEATURE

==========================================================================================
*/

bigklist_t *BigfileLoadKList(char *filename, qboolean stopOnError)
{
	bigklist_t *klist;
	bigkentry_t *entry;
	int linenum = 0;
	char line[256], temp[256], ext[15];
	unsigned int hash;
	qboolean options;
	int val, i;
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
		if (line[0] == '[')
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

		// new entry
		if (line[0] == '[')
		{
			if (!strcmp(line, "[options]"))
				options = true;
			else
			{
				options = false;
				if (sscanf(line, "[%X]", &hash) < 1)
					Error("bad entry definition on line %i: %s", linenum, line);

				// new entry
				entry = &klist->entries[klist->numentries];
				entry->hash = hash;
				entry->adpcmrate = 11025; // default ADPCM sampling rate
				entry->type = BIGENTRY_UNKNOWN;
				entry->rawinfo = NULL;
				strcpy(entry->path, "");
				
				// warn for double defienition
				for (i = 0; i < klist->numentries; i++)
					if (klist->entries[i].hash == hash)
						Warning("redefenition of hash %.8X on line %i", hash, linenum);

				klist->numentries++;
			}
			continue;
		}

		// option directives
		if (options == true)
		{
			continue;
		}

		if (entry == NULL)
			continue;

		// parms
		if (sscanf(line, "type=%s", &temp))
		{
			entry->type = ParseBigentryType(temp);
			if (entry->type == BIGENTRY_RAW_IMAGE)
				entry->rawinfo = NewRawInfo();
			continue;
		}
		if (entry->type == BIGENTRY_RAW_IMAGE)
		{
			if (ReadRawInfo(line, entry->rawinfo) == true)
				continue;
		}
	
		// path=%s - force file path
		// if file with extension (eg sound/kain1.vag) - a full path override
		// otherwise just add path to hashed name
		if (sscanf(line,"path=%s", &temp))
		{
			ExtractFileExtension(temp, ext);
			// no extension, so this is a path
			if (!ext[0]) 
				sprintf(entry->path, "%s/%.8X.%s", temp, entry->hash, bigentryext[entry->type]);
			// full name override
			else
				strcpy(entry->path, temp);

			// warn for double path defienition
			for (i = 0; i < (klist->numentries - 1); i++)
				if (!strcmp(klist->entries[i].path, entry->path))
					Warning("path redefenition on entry #%.8X on line %i (previously defined for entry #%.8X\n", entry->hash, linenum, klist->entries[i].hash);
			continue;
		}

		// rate=%i - force ADPCM rate (conversion needs to know)
		if (sscanf(line,"rate=%i", &val))
		{
			entry->adpcmrate = val;
			continue;
		}

		// warn for bad syntax
		if (line[0] == '\n')
			continue;
		if (line[0] == '#')
			continue;
		Warning("bad line %i: %s", linenum, line);
	}

	Verbose("%s: %i entries\n", filename, klist->numentries);

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
	entry->adpcmrate = 11025;
	entry->rawinfo = NULL;
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
		fprintf(f, "# entry %i\n", i + 1);
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
			case BIGENTRY_RAW_ADPCM:
				fprintf(f, "adpcm.rate=%i\n", entry->adpcmrate);
				break;
			case BIGENTRY_RAW_IMAGE:
				WriteRawInfo(f, entry->rawinfo);
				break;
			default:
				break;
		}
	}
}


// quick way to get entry data from header
bigfileentry_t *ReadBigfileHeaderOneEntry(FILE *f, unsigned int hash)
{
	unsigned int numentries, i;
	unsigned int *read;
	bigfileentry_t *entry;

	entry = NULL;
	if (fread(&numentries, sizeof(unsigned int), 1, f) < 1)
		Error("BigfileHeader: wrong of broken file\n");
	if (!numentries)
		Error("BigfileHeader: funny entries count, perhaps file is broken\n");

	read = qmalloc(numentries * 3 * sizeof(unsigned int));
	if (fread(read, numentries * 3 * sizeof(unsigned int), 1, f) < 1)
		Error("BigfileHeader: error reading header %s\n", strerror(errno));
	
	for (i = 0; i < numentries*3; i += 3)
	{
		if (read[i] != hash)
			continue;
		// make entry
		entry = qmalloc(sizeof(bigfileentry_t));
		BigfileEmptyEntry(entry);
		
		entry->hash = read[i];
		entry->size = read[i+1];
		entry->offset = read[i+2];
		entry->type = BIGENTRY_UNKNOWN;
		// assign default name
		sprintf(entry->name, "%s%.8X.%s", bigentryautopaths[BIGENTRY_UNKNOWN], read[0], bigentryext[BIGENTRY_UNKNOWN]);
		if (!entry->hash || !entry->offset)
			Error("BigfileHeader: entry %i is broken\n", i);
		break;
	}

	qfree(read);
	return entry;
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
	Verbose("%s: %i entries", filename, data->numentries);

	// read entries
	data->entries = qmalloc(data->numentries * sizeof(bigfileentry_t));
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];
		BigfileEmptyEntry(entry);

		Pacifier("reading entry %i of %i...", i + 1, data->numentries);

		if (fread(&read, 12, 1, f) < 1)
			Error("BigfileHeader: error on entry %i (%s)\n", i, strerror(errno));
		entry->hash = read[0];
		entry->size = read[1];
		entry->offset = read[2];
		entry->type = BIGENTRY_UNKNOWN;
		// assign default name
		sprintf(entry->name, "%s%.8X.%s", bigentryautopaths[BIGENTRY_UNKNOWN], read[0], bigentryext[BIGENTRY_UNKNOWN]);
		if (!entry->hash || !entry->offset)
			Error("BigfileHeader: entry %i is broken\n", i);
	}
	PacifierEnd();

	// load contents
	if (loadfilecontents)
	{
		for (i = 0; i < (int)data->numentries; i++)
		{
			entry = &data->entries[i];
			if (entry->size <= 0)
				continue;

			Pacifier("loading entry %i of %i...", i + 1, data->numentries);

			entry->data = qmalloc(entry->size);
			BigfileSeekContents(f, entry->data, entry);
		}
		PacifierEnd();
	}

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
// this function also does autoconvert job
void BigfileFixListfileEntry(char *srcdir, bigfileentry_t *entry, qboolean lowmem)
{
	char ext[16], filename[MAX_BLOODPATH], basename[MAX_BLOODPATH];
	tim_image_t *tim;
	byte *contents;
	int i, size;
	FILE *f;

	// extract extension
	StripFileExtension(entry->name, basename);
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
			// get base filename
			if (i == 0)
				sprintf(filename, "%s/%s", srcdir, entry->name);
			else
				sprintf(filename, "%s/%s_layer%i.tga", srcdir, basename, i);
			tim = TIM_LoadFromTarga(filename, entry->timtype[i]); 
			entry->size += (unsigned int)tim->filelen;
			if (i == 0)
				entry->data = tim;
			if (i != 0 || lowmem) // VorteX: -lomem key support, read that again later
				FreeTIM(tim);
		}

		return;
	}

	// WAV/OGG -> VAG autoconversion
	if (entry->type == BIGENTRY_RAW_ADPCM)
	{
		sprintf(filename, "%s/%s", srcdir, entry->name);
		if (!SoX_FileToData(filename, "--no-dither", "", "-t ima -c 1", &size, &contents))
			Error("unable to convert %s, SoX Error #%i\n", entry->name, GetLastError());
		entry->data = contents;
		entry->size = (unsigned int)size;

		if (lowmem) // we know the size, convert again later
			qfree(entry->data);

		return;
	}


	Error("can't identify file %s", entry->name);
}

// print stats about loaded bigfile entry
void BigfileEmitStats(bigfileheader_t *data)
{
	bigfileentry_t *entry;
	int stats[BIGFILE_NUM_FILETYPES], timstats[4], rawstats[NUM_RAW_TYPES];
	int i;

	// collect stats
	memset(stats, 0, sizeof(stats));
	memset(timstats, 0, sizeof(timstats));
	memset(rawstats, 0, sizeof(rawstats));
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
		else if (entry->type == BIGENTRY_RAW_IMAGE)
			rawstats[entry->rawinfo->type]++;
		stats[entry->type]++;
	}

	// print stats
	if (stats[BIGENTRY_RAW_ADPCM])
		Print(" %6i raw ADPCM\n", stats[BIGENTRY_RAW_ADPCM]);
	if (stats[BIGENTRY_RIFF_WAVE])
		Print(" %6i RIFF WAVE\n", stats[BIGENTRY_RIFF_WAVE]);
	// TIM
	if (timstats[0])
		Print(" %6i  4-bit TIM\n", timstats[0]);
	if (timstats[1])
		Print(" %6i  8-bit TIM\n", timstats[1]);
	if (timstats[2])
		Print(" %6i  16-bit TIM\n", timstats[2]);
	if (timstats[3])
		Print(" %6i  24-bit TIM\n", timstats[3]);
	if (stats[BIGENTRY_TIM])
		Print(" %6i TIM total\n", stats[BIGENTRY_TIM]);
	// RAW
	if (rawstats[RAW_TYPE_0])
		Print(" %6i  raw type 0\n", rawstats[RAW_TYPE_0]);
	if (rawstats[RAW_TYPE_1])
		Print(" %6i  raw type 1\n", rawstats[RAW_TYPE_1]);
	if (rawstats[RAW_TYPE_2])
		Print(" %6i  raw type 2\n", rawstats[RAW_TYPE_2]);
	if (rawstats[RAW_TYPE_3])
		Print(" %6i  raw type 3\n", rawstats[RAW_TYPE_3]);
	if (rawstats[RAW_TYPE_4])
		Print(" %6i  raw type 4\n", rawstats[RAW_TYPE_4]);
	if (rawstats[RAW_TYPE_5])
		Print(" %6i  raw type 5\n", rawstats[RAW_TYPE_5]);
	if (rawstats[RAW_TYPE_6])
		Print(" %6i  raw type 6\n", rawstats[RAW_TYPE_6]);
	if (rawstats[RAW_TYPE_7])
		Print(" %6i  raw type 7\n", rawstats[RAW_TYPE_7]);
	if (rawstats[RAW_TYPE_8])
		Print(" %6i  raw type 8\n", rawstats[RAW_TYPE_8]);
	if (rawstats[RAW_TYPE_SPECIAL])
		Print(" %6i  special raw\n", rawstats[RAW_TYPE_SPECIAL]);
	if (stats[BIGENTRY_RAW_IMAGE])
		Print(" %6i raw total\n", stats[BIGENTRY_RAW_IMAGE]);
	if (stats[BIGENTRY_VAG])
		Print(" %6i VAG\n", stats[BIGENTRY_VAG]);

	// total
	if (stats[BIGENTRY_UNKNOWN])
		Print(" %6i unknown\n", stats[BIGENTRY_UNKNOWN]);
	Verbose(" %6i TOTAL\n", data->numentries);
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
	Verbose("%s: %i entries\n", filename, numentries);

	// read all entries
	linenum = 1;
	entry = NULL;
	data->entries = qmalloc(numentries * sizeof(bigfileentry_t));
	data->numentries = 0;
	while(!feof(f))
	{
		linenum++;
		fgets(line, 256, f);

		// comments
		if (!line[0] || line[0] == '#')
			continue;

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
			
			Pacifier("reading entry %i of %i...", data->numentries, numentries);
			continue;
		}

		// scan parameter
		if (entry == NULL)
			Error("Entry data without actual entry on line %i: %s\n", linenum, line);

		// parse base parms
		if (sscanf(line, "type=%i", &val))
		{
			entry->type = (bigentrytype_t)val;
			if (entry->type == BIGENTRY_RAW_IMAGE)
				entry->rawinfo = NewRawInfo();
		}
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
		else if (sscanf(line, "adpcm.rate=%i", &val))
			entry->adpcmrate = val;
		// for raw
		else if (entry->type == BIGENTRY_RAW_IMAGE)
			ReadRawInfo(line, entry->rawinfo);
	}
	PacifierEnd();

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

  Extracting entries from bigfile

==========================================================================================
*/

void TGAfromTIM(FILE *bigf, bigfileentry_t *entry, char *outfile, qboolean bpp16to24)
{
	char name[MAX_BLOODPATH], maskname[MAX_BLOODPATH], suffix[21];
	tim_image_t *tim;
	int i;

	BigfileSeekFile(bigf, entry);
	for (i = 0; i < entry->timlayers; i++)
	{
		// extract base
		tim = TIM_LoadFromStream(bigf);
		strcpy(name, outfile);
		strcpy(entry->name, name); // write correct listfile.txt
		if (i != 0)
		{
			sprintf(suffix, "_layer%i", i);
			AddSuffix(name, name, suffix);
		}
		if (tim->error)
			Error("error saving %s: %s\n", name, tim->error);
		// write basefile
		TIM_WriteTarga(tim, name, bpp16to24);
		// write maskfile
		if (tim->pixelmask != NULL)
		{
			if (i == 0)
				sprintf(suffix, "_mask");
			else
				sprintf(suffix, "_layer%i_mask", i);
			AddSuffix(maskname, name, suffix);
			TIM_WriteTargaGrayscale(tim->pixelmask, tim->dim.xsize, tim->dim.ysize, maskname);
		}
		FreeTIM(tim);
	}
}

void TGAfromRAW(rawblock_t *rawblock, rawinfo_t *rawinfo, char *outfile, qboolean rawnoalign, qboolean verbose)
{
	int maxwidth, maxheight, i;
	char name[MAX_BLOODPATH], suffix[8];

	// detect maxwidth/maxheight for alignment
	maxwidth = maxheight = 0;
	for (i = 0; i < rawblock->chunks; i++)
	{
		maxwidth = max(maxwidth, (rawblock->chunk[i].width + rawblock->chunk[i].x));
		maxheight = max(maxheight, (rawblock->chunk[i].height + rawblock->chunk[i].y));
	}

	// export all chunks
	for (i = 0; i < rawblock->chunks; i++)
	{
		if (rawinfo->chunknum != -1 && i != rawinfo->chunknum)
			continue; // skip this chunk
		strcpy(name, outfile);
		if (rawblock->chunks != 1)
		{
			sprintf(suffix, "_%03i", i);
			AddSuffix(name, name, suffix);
		}
		if (verbose == true)
			Print("writing %s.\n", name);
		if (rawnoalign)
			RawTGA(name, rawblock->chunk[i].width, rawblock->chunk[i].height, 0, 0, 0, 0, rawblock->chunk[i].colormap ? rawblock->chunk[i].colormap : rawblock->colormap, rawblock->chunk[i].pixels, 8, rawinfo);
		else
			RawTGA(name, rawblock->chunk[i].width, rawblock->chunk[i].height, rawblock->chunk[i].x, rawblock->chunk[i].y, max(0, maxwidth - rawblock->chunk[i].width - rawblock->chunk[i].x), max(0, maxheight - rawblock->chunk[i].height - rawblock->chunk[i].y), rawblock->chunk[i].colormap ? rawblock->chunk[i].colormap : rawblock->colormap, rawblock->chunk[i].pixels, 8, rawinfo);
	}
}

void BigFileUnpackEntry(FILE *bigf, bigfileentry_t *entry, char *dstdir, qboolean tim2tga, qboolean bpp16to24, qboolean nopaths, qboolean vagconvert, qboolean vagpcm, qboolean vagogg, qboolean rawconvert, rawtype_t forcerawtype, qboolean rawnoalign)
{
	char savefile[MAX_BLOODPATH], outfile[MAX_BLOODPATH], basename[MAX_BLOODPATH], path[MAX_BLOODPATH];
	char inputcmd[512], outputcmd[512];
	rawblock_t *rawblock;
	FILE *f;

	// nopaths, clear path
	if (nopaths)
	{
		ExtractFileBase(entry->name, path);
		strcpy(entry->name, path);
	}

	// make directory
	ExtractFilePath(entry->name, path);
	if (path[0])
	{
		sprintf(savefile, "%s/%s", dstdir, path);
		FS_CreatePath(savefile);
	}

	// original pill.big has 'funky' files with zero len, export them as empty ones
	if (entry->size <= 0) 
	{
		sprintf(savefile, "%s/%s", dstdir, entry->name);
		f = SafeOpen(savefile, "wb");
		fclose(f);
		return;
	}

	// autoconvert TGA
	// todo: optimise to use common pipeline
	if (tim2tga && entry->type == BIGENTRY_TIM)
	{
		sprintf(outfile, "%s/%s.tga", dstdir, entry->name);
		TGAfromTIM(bigf, entry, outfile, bpp16to24);
		return;
	}

	// load file contents
	if (entry->data == NULL)
	{
		entry->data = qmalloc(entry->size);
		BigfileSeekContents(bigf, entry->data, entry);
	}

	// autoconvert VAG
	if (vagconvert && entry->type == BIGENTRY_RAW_ADPCM)
	{
		StripFileExtension(entry->name, basename);

		// try to save
		sprintf(savefile, (vagogg) ? "%s/%s.ogg" : "%s/%s.wav", dstdir, basename);
		sprintf(inputcmd, "-t ima -r %i -c 1", entry->adpcmrate);
		if (vagogg)
			sprintf(outputcmd, "-t ogg -C 7");
		else if (vagpcm)
			sprintf(outputcmd, "-t wav -e signed-integer");
		else 
			sprintf(outputcmd, "-t wav");
		if (SoX_DataToFile(entry->data, entry->size, "--no-dither", inputcmd, outputcmd, savefile))
			sprintf(entry->name, (vagogg) ? "%s.ogg" : "%s.wav", basename);  // write correct listfile.tx
		else
		{
			Warning("unable to convert %s, SoX Error #%i, unpacking original", entry->name, GetLastError());
			sprintf(savefile, "%s/%s", dstdir, entry->name);
			SaveFile(savefile, entry->data, entry->size);
		}

		qfree(entry->data);
		entry->data = NULL;
		return;
	}

	// convert raw file
	if (rawconvert && entry->type == BIGENTRY_RAW_IMAGE)
	{
		rawblock = RawExtract(entry->data, entry->size, entry->rawinfo, true, false, forcerawtype);
		if (rawblock->errorcode < 0)
			Print("warning: cound not extract raw %s: %s\n", entry->name, RawStringForResult(rawblock->errorcode));
		else
		{
			StripFileExtension(entry->name, basename);
			sprintf(outfile, "%s/%s.tga", dstdir, basename);
			TGAfromRAW(rawblock, entry->rawinfo, outfile, rawnoalign, false); 
		}
		FreeRawBlock(rawblock);
	}

	// unpack original
	sprintf(savefile, "%s/%s", dstdir, entry->name);
	SaveFile(savefile, entry->data, entry->size);
	qfree(entry->data);
	entry->data = NULL;
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

qboolean BigFileScanVAG(FILE *f, bigfileentry_t *entry)
{
	byte tag[4];

	BigfileSeekFile(f, entry);

	// first unsigned int - tag
	if (fread(&tag, sizeof(char), 4, f) < 1)
		return false;
	if (tag[0] != 'V' || tag[1] != 'A' || tag[2] != 'G' || tag[3] != 'p')
		return false;

	// it's a VAG
	return true;
}

qboolean BigFileScanRaw(FILE *f, bigfileentry_t *entry, rawtype_t forcerawtype)
{
	unsigned char *filedata;
	rawinfo_t *rawinfo;
	rawblock_t *rawblock;

	// load file contents
	BigfileSeekFile(f, entry);
	filedata = qmalloc(entry->size);
	if (fread(filedata, entry->size, 1, f) < 1)
	{
		qfree(filedata);
		return false;
	}

	// check all raw types
	rawinfo = NewRawInfo();
	rawblock = RawExtract(filedata, entry->size, rawinfo, true, false, forcerawtype);
	if (rawblock->errorcode >= 0)
	{
		FreeRawBlock(rawblock);
		entry->rawinfo = rawinfo;
		qfree(filedata);
		return true;
	}
	// not found
	FreeRawBlock(rawblock);
	qfree(filedata);
	qfree(rawinfo);
	return false;
}

bigentrytype_t BigfileDetectFiletype(FILE *f, bigfileentry_t *entry, qboolean scanraw, rawtype_t forcerawtype)
{
	if (BigFileScanTIM(f, entry))
		return BIGENTRY_TIM;
	if (BigFileScanRiffWave(f, entry))
		return BIGENTRY_RIFF_WAVE;
	if (BigFileScanVAG(f, entry))
		return BIGENTRY_VAG;
	if (scanraw)
		if (BigFileScanRaw(f, entry, forcerawtype))
			return BIGENTRY_RAW_IMAGE;
	return BIGENTRY_UNKNOWN;
}

void BigfileScanFiletype(FILE *f, bigfileentry_t *entry, qboolean scanraw, rawtype_t forcerawtype)
{
	bigentrytype_t autotype;
	bigkentry_t *kentry;
	char *autopath;

	// detect filetype automatically
	autotype = BigfileDetectFiletype(f, entry, scanraw, forcerawtype);
	if (autotype != BIGENTRY_UNKNOWN) 
	{
		entry->type = autotype;
		// automatic path
		autopath = NULL;
		if (autotype == BIGENTRY_RAW_IMAGE)
			autopath = PathForRawType(entry->rawinfo->type);
		if (autopath == NULL)
			autopath = bigentryautopaths[autotype];
		sprintf(entry->name, "%s%.8X.%s", autopath, entry->hash, bigentryext[entry->type]);
	}
	// check listfile
	else
	{
		kentry = BigfileSearchKList(entry->hash);
		if (kentry != NULL)
		{
			entry->type = (bigentrytype_t)kentry->type;
			entry->adpcmrate = (int)kentry->adpcmrate;
			if (entry->type == BIGENTRY_RAW_IMAGE)
				entry->rawinfo = kentry->rawinfo;
			// check custom path
			if (kentry->path[0])
				sprintf(entry->name, "%s", kentry->path);
			else
			{
				// automatic path
				autopath = NULL;
				if (entry->type == BIGENTRY_RAW_IMAGE)
					autopath = PathForRawType(entry->rawinfo->type);
				if (autopath == NULL)
					autopath = bigentryautopaths[autotype];
				sprintf(entry->name, "%s%.8X.%s", autopath, entry->hash, bigentryext[entry->type]);
			}
		}
	}
}

void BigfileScanFiletypes(FILE *f, bigfileheader_t *data, qboolean scanraw, list_t *ixlist, rawtype_t forcerawtype)
{
	fpos_t fpos;
	bigfileentry_t *entry;
	int i;
	
	fgetpos(f, &fpos);
	// scan for filetypes
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		// ignore if mismatched
		if (ixlist)
			if (!MatchIXList(entry, ixlist, false, false))
				continue;

		// ignore null-sized
		if (!entry->size)
			continue;

		Pacifier("scanning type for entry %i of %i...", i + 1, data->numentries);
		BigfileScanFiletype(f, entry, scanraw, forcerawtype);
	}
	fsetpos(f, &fpos);
	
	PacifierEnd();

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
	BigfileScanFiletypes(f, data, true, NULL, RAW_TYPE_UNKNOWN);

	// analyse headers
	chunkstats = (bigchunkstats_t *)qmalloc(sizeof(bigchunkstats_t));
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];
		if (entry->type != BIGENTRY_UNKNOWN)
			continue;
		Pacifier("analysing entry %i of %i...", i + 1, data->numentries);

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
	PacifierEnd("\n");

	// print stats
	Print("=== Chunk (unsigned int) ===\n");
	Print("  occurence threshold = 4\n");
	for (i = 0; i < chunkstats->numchunks4; i++)
		if (chunkstats->chunks4[i].occurrences > 4)
			Print("  %.8X = %i occurences\n", chunkstats->chunks4[i].data, chunkstats->chunks4[i].occurrences);

	fclose(f);
	qfree(chunkstats);
	return 0;

}

/*
==========================================================================================

  -bigfile -list

  lists bigfile contents

==========================================================================================
*/

int BigFile_List(int argc, char **argv, char *listfile, qboolean scanraw, char *csvfile)
{
	FILE *f, *f2;
	bigfileheader_t *data;
	bigfileentry_t *entry;
	char name[MAX_BLOODPATH], typestr[128], extrainfo[128];
	int i;

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	data = ReadBigfileHeader(f, bigfile, false);
	BigfileScanFiletypes(f, data, true, NULL, RAW_TYPE_UNKNOWN);

	// print or...
	if (listfile[0] == '-')
		BigfileWriteListfile(stdout, data);
	else // output to file
	{
		f2 = SafeOpen(listfile, "w");
		BigfileWriteListfile(f2, data);
		Print("wrote %s\n", listfile);
		fclose(f2);
	}

	// export CSV file
	if (csvfile[0] != '-')
	{
		Print("Exporting %s...\n", csvfile);
		f2 = SafeOpenWrite(csvfile);
		for (i = 0; i < (int)data->numentries; i++)
		{
			entry = &data->entries[i];
			// base info
			if (entry->type == BIGENTRY_RAW_IMAGE)
				sprintf(typestr, "%s.%s", bigentryext[entry->type], UnparseRawType(entry->rawinfo->type));
			else
				sprintf(typestr, "%s", bigentryext[entry->type]);
			// extra info
			if (entry->type == BIGENTRY_RAW_ADPCM)
				sprintf(extrainfo, "%i", entry->adpcmrate);
			else if (entry->type == BIGENTRY_RAW_IMAGE)
			{
				if (entry->rawinfo->type == RAW_TYPE_2 && entry->rawinfo->doubleres == true)
					sprintf(extrainfo, "doubleres");
				else
					sprintf(extrainfo, "");
			}	
			else sprintf(extrainfo, "");
			// print
			strcpy(name, entry->name);
			ConvSlash(name);
			fprintf(f2, "%i;%s;%s;%s;\n", entry->hash, name, typestr, extrainfo);
		}
		fclose(f2);
	}

/*
	f2 = SafeOpenWrite("list.log");
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];
		if (entry->type == BIGENTRY_RAW_IMAGE)
		{
			if (entry->rawinfo->type ==
				sprintf(typestr, "%s.%s", bigentryext[entry->type], UnparseRawType(entry->rawinfo->type));

		else if (entry->type == BIGENTRY_RAW_IMAGE)
			sprintf(eypestr, "%s.%s", bigentryext[entry->type], UnparseRawType(entry->rawinfo->type));

	}
	fclose(f);
*/

	Print("done.\n");
	fclose (f);
	return 0;
}

/*
==========================================================================================

  -bigfile -extract

  extract single entity

==========================================================================================
*/

// "-bigfile c:/pill.big -extract 0AD312F45 -format tga"
int BigFile_Extract(int argc, char **argv, char *outfile, unsigned int hash, char *whatformat)
{
	bigfileentry_t *entry;
	char filename[MAX_BLOODPATH], basename[MAX_BLOODPATH], format[16], last;
	int spritex = 0, spritey = 0, spritesp = -1;
	rawblock_t *rawblock;
	FILE *f;

	// check format
	strcpy(format, whatformat);
	if (!format[0])
	{
		if (outfile != NULL)
			ExtractFileExtension(outfile, format);
		if (!format[0])
			Error("Format is not given\n");
	}

	// open & scan
	f = SafeOpen(bigfile, "rb");
	entry = ReadBigfileHeaderOneEntry(f, hash);
	if (entry == NULL)
		Error("Failed to find entry %.8X\n", hash);
	BigfileScanFiletype(f, entry, true, RAW_TYPE_UNKNOWN);

	// cannot extract empty files
	if (entry->size == 0)
		Error("Empty file\n", hash);
	
	// raw extract (no conversion)
	if (!stricmp(format, "raw"))
		return 0;	

	// get outfile
	if (outfile == NULL)
		ExtractFileBase(entry->name, filename); // pick automatic name
	else
	{
		last = outfile[strlen(outfile)-1];
		if (last != '/' && last != '\\') // full path is given
			strcpy(filename, outfile);
		else // only path is given
		{
			ExtractFileBase(entry->name, filename);
			StripFileExtension(filename, basename);
			sprintf(filename, "%s%s", outfile, basename);
		}
	}
	
	// extract
	switch(entry->type)
	{
		case BIGENTRY_UNKNOWN:
			Error("unknown entry type, bad format '%s'\n", format);
			break;
		case BIGENTRY_TIM:
			if (!stricmp(format, "tga"))
			{
				DefaultExtension(filename, ".tga", sizeof(filename));
				Print("writing %s.\n", filename);
				TGAfromTIM(f, entry, filename, false); 
			}
			else if (!stricmp(format, "tga24"))
			{
				DefaultExtension(filename, ".tga", sizeof(filename));
				Print("writing %s.\n", filename);
				TGAfromTIM(f, entry, filename, true); 
			}
			else
				Error("unknown format '%s'\n", format);
			break;
		case BIGENTRY_RAW_ADPCM:
			break;
		case BIGENTRY_RIFF_WAVE:
			break;
		case BIGENTRY_RAW_IMAGE:
			// read file contents and converto to rawblock
			entry->data = qmalloc(entry->size);
			BigfileSeekContents(f, entry->data, entry);
			rawblock = RawExtract(entry->data, entry->size, entry->rawinfo, false, false, RAW_TYPE_UNKNOWN);
			qfree(entry->data);
			entry->data = NULL;
			// convert rawblock to something
			if (!strnicmp(format, "spr32", 5))
			{
				DefaultExtension(filename, ".spr32", sizeof(filename));
				Print("writing %s.\n", filename);
				if (!stricmp(format, "spr32:oriented") || !stricmp(format, "spr"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_DARKPLACES, SPR_ORIENTED, spritex, spritey, spritesp, 0);
				else if (stricmp(format, "spr32:parallel_upright"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_DARKPLACES, SPR_VP_PARALLEL_UPRIGHT, spritex, spritey, spritesp, 0);
				else if (!stricmp(format, "spr32:facing_upright"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_DARKPLACES, SPR_VP_FACING_UPRIGHT, spritex, spritey, spritesp, 0);
				else if (!stricmp(format, "spr32:parallel"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_DARKPLACES, SPR_VP_PARALLEL, spritex, spritey, spritesp, 0);
				else if (!stricmp(format, "spr32:parallel_oriented"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_DARKPLACES, SPR_VP_PARALLEL_ORIENTED, spritex, spritey, spritesp, 0);
				else
					Error("unknown format '%s'\n", format);
			}
			else if (!strnicmp(format, "spr", 3))
			{
				DefaultExtension(filename, ".spr", sizeof(filename));
				Print("writing %s.\n", filename);
				if (!stricmp(format, "spr:oriented") || !stricmp(format, "spr"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_QUAKE, SPR_ORIENTED, spritex, spritey, spritesp, 0);
				else if (stricmp(format, "spr:parallel_upright"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_QUAKE, SPR_VP_PARALLEL_UPRIGHT, spritex, spritey, spritesp, 0);
				else if (!stricmp(format, "spr:facing_upright"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_QUAKE, SPR_VP_FACING_UPRIGHT, spritex, spritey, spritesp, 0);
				else if (!stricmp(format, "spr:parallel"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_QUAKE, SPR_VP_PARALLEL, spritex, spritey, spritesp, 0);
				else if (!stricmp(format, "spr:parallel_oriented"))
					SPR_WriteFromRawblock(rawblock, filename, SPR_QUAKE, SPR_VP_PARALLEL_ORIENTED, spritex, spritey, spritesp, 0);
				else
					Error("unknown format '%s'\n", format);
			}
			else if (!stricmp(format, "tga"))
			{

			}
			else
				Error("unknown format '%s'\n", format);
			FreeRawBlock(rawblock);
			break;
		case BIGENTRY_VAG:
			Error("Vag extraction not supported");
			break;
		default:
			Error("bad entry type\n");
			break;
	}

	Print("done.\n");
	fclose(f);
	return 0;
}

/*
==========================================================================================

  -bigfile -unpack

  unpack whole bigfile to a folder

==========================================================================================
*/

int BigFile_Unpack(int argc, char **argv, char *dstdir, list_t *ixlist, qboolean tim2tga, qboolean bpp16to24, qboolean nopaths, qboolean vagconvert, qboolean vagpcm, qboolean vagogg, qboolean scanraw, qboolean rawconvert, rawtype_t forcerawtype, qboolean rawnoalign)
{
	FILE *f, *f2;
	char savefile[MAX_BLOODPATH];
	bigfileheader_t *data;
	int i;

	// show options
	if (tim2tga)
		Print("Option: TIM->TGA conversion\n");
	if (bpp16to24)
		Print("Option: Targa compatibility mode (converting 16-bit to 24-bit)\n");
	if (nopaths)
		Print("Option: Disallow klist-set subpaths\n");
	if (scanraw)
		Print("Option: Scan for raw filetypes\n");
	if (forcerawtype != RAW_TYPE_UNKNOWN)
		Print("Option: Guessing all raw images is %s\n", UnparseRawType(forcerawtype));
	if (rawconvert)
		Print("Option: Converting raw images to TGA\n");
	if (rawnoalign)
		Print("Option: Disable RAW images aligning\n");
	if (vagconvert)
	{
		if (vagogg)
			Print("Option: VAG->OGG Vorbis Quality 5 conversion\n");
		else if (vagpcm)
			Print("Option: VAG->WAV PCM conversion\n");
		else
			Print("Option: VAG->WAV ADPCM conversion\n");
	}

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	data = ReadBigfileHeader(f, bigfile, false);
	BigfileScanFiletypes(f, data, scanraw, ixlist->items ? ixlist : NULL, forcerawtype);

	// make directory
	Q_mkdir(dstdir);
	Verbose("%s folder created\n", dstdir);
	
	// export all files
	for (i = 0; i < (int)data->numentries; i++)
	{
		if (ixlist->items)
			if (!MatchIXList(&data->entries[i], ixlist, true, true))
				continue;
		Pacifier("unpacking entry %i of %i...", i + 1, data->numentries);
		BigFileUnpackEntry(f, &data->entries[i], dstdir, tim2tga, bpp16to24, nopaths, vagconvert, vagpcm, vagogg, rawconvert, forcerawtype, rawnoalign);
	}
	PacifierEnd();

	// write listfile
	sprintf(savefile, "%s/listfile.txt", dstdir);
	f2 = SafeOpen(savefile, "w");
	BigfileWriteListfile(f2, data);
	fclose(f2);
	Print("wrote %s\ndone.\n", savefile);

	fclose (f);
	return 0;
}

int BigFile_Pack(int argc, char **argv, char *srcdir, qboolean lowmem)
{
	FILE *f;
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
		Verbose("%s already exists, overwriting\n", bigfile);
		fclose(f);
	}
	f = SafeOpen(bigfile, "wb");

	// write header
	SafeWrite(f, &data->numentries, 4);
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		Pacifier("writing header %i of %i...", i + 1, data->numentries);
	
		SafeWrite(f, &entry->hash, 4);
		SafeWrite(f, &entry->size, 4);
		SafeWrite(f, &entry->offset, 4);
	}
	PacifierEnd();

	// show options
	if (lowmem)
		Print("enabling low memory usage\n");

	// write files
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		Pacifier("writing entry %i of %i...\r", i + 1, data->numentries);

		// if file is already loaded
		if (entry->data != NULL)
		{
			// autoconverted TIM
			if (entry->type == BIGENTRY_TIM)
			{
				// VorteX: -lomem key support
				if (lowmem)
				{
					sprintf(savefile, "%s/%s", srcdir, entry->name);
					entry->data = TIM_LoadFromTarga(savefile, entry->timtype[0]);
				}
				
				tim = entry->data;
				size = tim->filelen;
				TIM_WriteToStream(entry->data, f);
				FreeTIM(entry->data);

				// add sublayers
				StripFileExtension(entry->name, basename);
				for (k = 1; k < entry->timlayers; k++)
				{
					sprintf(savefile, "%s/%s_layer%i.tga", srcdir, basename, k);
					tim = TIM_LoadFromTarga(savefile, entry->timtype[k]); 
					size += tim->filelen;
					TIM_WriteToStream(tim, f);
					FreeTIM(tim);
				}

				if (size != (int)entry->size)
					Error("entry %.8X (TIM): file size changed (%s%i bytes, newsize %i) while packing\n", entry->hash, (size - (int)entry->size) < 0 ? "-" : "+", size - (int)entry->size, size);
			}
			// autoconverted WAV/OGG
			else if (entry->type == BIGENTRY_RAW_ADPCM)
			{
				// VorteX: -lomem key support
				if (lowmem)
				{
					sprintf(savefile, "%s/%s", srcdir, entry->name);
					if (!SoX_FileToData(savefile, "--no-dither", "", "-t ima -c 1", &size, &contents))
						Error("unable to convert %s, SoX Error #%i\n", entry->name, GetLastError());
					entry->data = contents;

					if (size != (int)entry->size)
						Error("entry %.8X (RAW ADPCM): file size changed (%s%i bytes, newsize %i) while packing\n", entry->hash, (size - (int)entry->size) < 0 ? "-" : "+", size - (int)entry->size, size);
				}

				// write
				SafeWrite(f, entry->data, entry->size);
				qfree(entry->data);
			}
			// just write
			else
			{
				SafeWrite(f, entry->data, entry->size);
				qfree(entry->data);
			}
		}
		else
		{
			sprintf(savefile, "%s/%s", srcdir, entry->name);
			size = LoadFile(savefile, &contents);
			if (size != (int)entry->size)
				Error("entry %.8X (DAT): file size changed (%s%i bytes, newsize %i) while packing\n", entry->hash, (size - (int)entry->size) < 0 ? "-" : "+", size - (int)entry->size, size);
			SafeWrite(f, contents, size);
			qfree(contents);
		}
	}
	PacifierEnd();

	fclose (f);
	Print("done.\n");
	return 0;
}

/*
==========================================================================================

  Main

==========================================================================================
*/

#define MAX_INCLUDELIST		64
#define MAX_EXCLUDELIST		64

int BigFile_Main(int argc, char **argv)
{
	int i = 1, k, returncode = 0;
	char tofile[MAX_BLOODPATH], srcdir[MAX_BLOODPATH], dstdir[MAX_BLOODPATH], knownfiles[MAX_BLOODPATH], csvfile[MAX_BLOODPATH], format[256], *c;
	qboolean tim2tga, bpp16to24, lowmem, nopaths, vagconvert, vagpcm, vagogg, scanraw, rawconvert, rawnoalign;
	unsigned int hash;
	list_t *ixlist;
	rawtype_t forcerawtype;
	bigfileentry_t entry;

	Verbose("=== BigFile ===\n");
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
	strcpy(tofile, "-");
	strcpy(format, "");
	strcpy(dstdir, DEFAULT_PACKPATH);
	strcpy(srcdir, DEFAULT_PACKPATH);
	strcpy(csvfile, "-");
	strcpy(knownfiles, "-");
	tim2tga = false;
	bpp16to24 = false;
	lowmem = false;
	nopaths = false;
	vagconvert = false;
	vagpcm = false;
	vagogg = false;
	scanraw = false;
	rawconvert = false;
	forcerawtype = RAW_TYPE_UNKNOWN;
	rawnoalign = false;
	hash = 0;
	ixlist = NewList();
	for (k = 2; k < argc; k++)
	{
		if (!strcmp(argv[k],"-to"))
		{
			k++; if (k < argc)
				strlcpy(tofile, argv[k], sizeof(tofile));
		}
		else if (!strcmp(argv[k],"-dstdir"))
		{
			k++; if (k < argc)
				strlcpy(dstdir, argv[k], sizeof(dstdir));
		}
		else if (!strcmp(argv[k],"-srcdir"))
		{
			k++; if (k < argc)
				strlcpy(srcdir, argv[k], sizeof(srcdir));
		}
		else if (!strcmp(argv[k],"-klist"))
		{
			k++; if (k < argc)
				strlcpy(knownfiles, argv[k], sizeof(knownfiles));
		}
		else if (!strcmp(argv[k],"-extract"))
		{
			k++; if (k < argc)
				sscanf(argv[k], "%X", &hash);
		}
		else if (!strcmp(argv[k],"-format"))
		{
			k++; if (k < argc)
				strlcpy(format, argv[k], sizeof(format));
		}
		else if (!strcmp(argv[k],"-tim2tga"))
			tim2tga = true;
		else if (!strcmp(argv[k],"-csv"))
		{
			k++; if (k < argc)
				strlcpy(csvfile, argv[k], sizeof(csvfile));
		}
		else if (!strcmp(argv[k],"-16to24"))
			bpp16to24 = true;
		else if (!strcmp(argv[k],"-lowmem"))
			lowmem = true;
		else if (!strcmp(argv[k],"-nopaths"))
			nopaths = true;
		else if (!strcmp(argv[k],"-vagconvert"))
			vagconvert = true;
		else if (!strcmp(argv[k],"-pcm"))
			vagpcm = true;
		else if (!strcmp(argv[k],"-oggvorbis"))
			vagogg = true;
		else if (!strcmp(argv[k],"-scanraw"))
			scanraw = true;
		else if (!strcmp(argv[k],"-forcerawtype"))
		{
			k++; if (k < argc)
				forcerawtype = ParseRawType(argv[k]);
		}
		else if (!strcmp(argv[k],"-rawconvert"))
			rawconvert = true;
		else if (!strcmp(argv[k],"-noalign"))
			rawnoalign = true;
		else if (!strcmp(argv[k],"-x"))
		{
			k++; if (k < argc)
				ListAdd(ixlist, argv[k], false);
		}
		else if (!strcmp(argv[k],"-i"))
		{
			k++; if (k < argc)
				ListAdd(ixlist, argv[k], true);
		}
		else if (!strcmp(argv[k],"-matchtest"))
		{
			k++;
			if (k < argc)
			{
				strlcpy(entry.name, argv[k], sizeof(entry.name));
				if (MatchIXList(&entry, ixlist, false, true))
					Print("%s matched\n", argv[k]);
				else
					Print("%s not matched\n", argv[k]);
			}
		}
	}
	
	// load up knowledge base
	if (knownfiles[0] == '-')
		bigklist = BigfileLoadKList("klist.txt", true);
	else
		bigklist = BigfileLoadKList(knownfiles, false);

	// action
	if (!strcmp(argv[i], "-list"))
		returncode = BigFile_List(argc-i, argv+i, tofile, scanraw, csvfile);
	else if (!strcmp(argv[i], "-analyse"))
		returncode = BigFile_Analyse(argc-i, argv+i, tofile);
	else if (!strcmp(argv[i], "-extract"))
		returncode = BigFile_Extract(argc-i, argv+i, (tofile[0] == '-') ? NULL : tofile, hash, format);
	else if (!strcmp(argv[i], "-unpack"))
		returncode = BigFile_Unpack(argc-i, argv+i, dstdir, ixlist, tim2tga, bpp16to24, nopaths, vagconvert, vagpcm, vagogg, scanraw, rawconvert, forcerawtype, rawnoalign);
	else if (!strcmp(argv[i], "-pack"))
		returncode = BigFile_Pack(argc-i, argv+i, srcdir, lowmem);
	else
		Warning("unknown option %s", argv[i]);

	return returncode;
}