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
#include "BO1.h"
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
	qboolean pathonly; // only define path, not filename
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
	if (!strcmp(str, "raw"))
		return BIGENTRY_RAW_IMAGE;
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
	char line[1024], temp[256], ext[15];
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
				entry->pathonly = false;
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
			entry->pathonly = ext[0] ? false : true;
			strcpy(entry->path, temp);
			// warn for double path definition
			if (ext[0])
				for (i = 0; i < (klist->numentries - 1); i++)
					if (!strcmp(klist->entries[i].path, entry->path))
						Warning("path '%s' redefenition on entry #%.8X on line %i (previously defined for entry #%.8X\n", entry->path, entry->hash, linenum, klist->entries[i].hash);
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

bigfileheader_t *ReadBigfileHeader(FILE *f, char *filename, qboolean loadfilecontents, qboolean hashnamesonly)
{	
	bigfileheader_t *data;
	bigfileentry_t *entry;
	FILE *csvf;
	char line[512], temp[256];
	unsigned int read[3];
	unsigned int hash;
	int i, linenum, namesloaded;

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

	// load CSV list for filenames
	if (!hashnamesonly)
	{
		if (FileExists("BO1.csv"))
		{
			csvf = SafeOpen("BO1.csv", "r");
			linenum = 0;
			namesloaded = 0;
			while(!feof(csvf))
			{
				linenum++;
				fgets(line, 512, csvf);
				if (!sscanf(line, "%i;%s", &hash, temp))
				{
					Verbose("Warning: corrupted line %i in BO1.csv: '%s'!\n", linenum, line);
					continue;
				}
				// find hash
				for (i = 0; i < (int)data->numentries; i++)
				{
					if (data->entries[i].hash != hash)
						continue;
					ConvSlashW2U(temp);
					ExtractFileName(temp, line); 
					sprintf(entry->name, "%s%s", bigentryautopaths[BIGENTRY_UNKNOWN], line);
					namesloaded++;
					break;
				}
			}
			Verbose("BO1.csv: loaded %i names\n", namesloaded);
			fclose(csvf);
			// write BO1.h
			csvf = SafeOpenWrite("BO1.h");
			fprintf(csvf, "// BO1.h, converted automatically from Rackot's BO1.csv\n");
			fprintf(csvf, "// Thanks to Ben Lincoln And Andrey [Rackot] Grachev for this\n");
			fprintf(csvf, "// do not modify\n");
			fprintf(csvf, "typedef struct\n");
			fprintf(csvf, "{\n");
			fprintf(csvf, "	unsigned int	hash;\n");
			fprintf(csvf, "	char	name[17];\n");
			fprintf(csvf, "}wheelofdoom_names_t;\n");
			fprintf(csvf, "\n");
			fprintf(csvf, "#define NUM_CSV_ENTRIES %i\n", linenum);
			fprintf(csvf, "\n");
			fprintf(csvf, "wheelofdoom_names_t wheelofdoom_names[NUM_CSV_ENTRIES] =\n");
			fprintf(csvf, "{\n");
			for (i = 0; i < (int)data->numentries; i++)
			{
				ExtractFileName(data->entries[i].name, temp);
				if (strlen(temp) > 16)
				{
					Verbose("Warning: BO1.h: name '%s' is more that 16 chars, will be truncated!\n", temp);
					temp[17] = 0;
				}
				if (i+1 < (int)data->numentries)
					fprintf(csvf, "	{%10i, \"%s\"},\n", data->entries[i].hash, temp);
				else
					fprintf(csvf, "	{%10i, \"%s\"}\n", data->entries[i].hash, temp);
			}
			fprintf(csvf, "};\n");
			fclose(csvf);
			Verbose("BO1.h written.\n", namesloaded);
		}
		else
		{
			// or use internal array
			namesloaded = 0;
			for (linenum = 0; linenum < NUM_CSV_ENTRIES; linenum++)
			{
				hash = wheelofdoom_names[linenum].hash;
				for (i = 0; i < (int)data->numentries; i++)
				{
					if (data->entries[i].hash != hash)
						continue;
					namesloaded++;
					sprintf(data->entries[i].name, "%s%s", bigentryautopaths[BIGENTRY_UNKNOWN], wheelofdoom_names[linenum].name);
					break;
				}
			}
			if (namesloaded)
				Verbose("Loaded %i internal filenames.\n", namesloaded);
		}
	}

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
bigfileheader_t *BigfileOpenListfile(char *srcdir)
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

	// emit some ststs
	BigfileEmitStats(data);

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
			sprintf(suffix, "_mask");
			AddSuffix(maskname, name, suffix);
			TIM_WriteTargaGrayscale(tim->pixelmask, tim->dim.xsize, tim->dim.ysize, maskname);
		}
		FreeTIM(tim);
	}
}

void TGAfromRAW(rawblock_t *rawblock, rawinfo_t *rawinfo, char *outfile, qboolean rawnoalign, qboolean verbose, qboolean usesubpaths)
{
	char name[MAX_BLOODPATH], suffix[8], path[MAX_BLOODPATH], basename[MAX_BLOODPATH];
	int maxwidth, maxheight, i;

	// detect maxwidth/maxheight for alignment
	maxwidth = maxheight = 0;
	for (i = 0; i < rawblock->chunks; i++)
	{
		maxwidth = max(maxwidth, (rawblock->chunk[i].width + rawblock->chunk[i].x));
		maxheight = max(maxheight, (rawblock->chunk[i].height + rawblock->chunk[i].y));
	}

	// quick fix for files in separate folders
	// todo: optimize
	if (usesubpaths)
	{
		strcpy(name, outfile);
		ExtractFilePath(name, path);
		ExtractFileBase(name, basename);
		sprintf(name, "%s%s/", path, basename);
		CreatePath(name);
		sprintf(name, "%s%s/%s.tga", path, basename, basename);
		strcpy(basename, name);
	}
	else
		sprintf(basename, outfile);

	// export all chunks
	for (i = 0; i < rawblock->chunks; i++)
	{
		if (rawinfo->chunknum != -1 && i != rawinfo->chunknum)
			continue; // skip this chunk
		strcpy(name, basename);
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

// unpack entry to 'original' dir, assumes that entity is already loaded
void BigFileUnpackOriginalEntry(bigfileentry_t *entry, char *dstdir, qboolean place_separate)
{
	char savefile[MAX_BLOODPATH];

	if (place_separate)
	{
		ExtractFileName(entry->name, savefile);
		sprintf(entry->name, "original/%s", savefile);
	}
	sprintf(savefile, "%s/%s", dstdir, entry->name);
	SaveFile(savefile, entry->data, entry->size);
}

void BigFileUnpackEntry(FILE *bigf, bigfileentry_t *entry, char *dstdir, qboolean tim2tga, qboolean bpp16to24, qboolean nopaths, int adpcmconvert, int vagconvert, qboolean rawconvert, rawtype_t forcerawtype, qboolean rawnoalign, qboolean psone)
{
	char savefile[MAX_BLOODPATH], outfile[MAX_BLOODPATH], basename[MAX_BLOODPATH], path[MAX_BLOODPATH];
	char inputcmd[512], outputcmd[512];
	rawblock_t *rawblock;
	char *data;
	int c, size;

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
		CreatePath(savefile);
	}

	// original pill.big has 'funky' files with zero len, export them as empty ones
	if (entry->size <= 0)
		return;

	// autoconvert TGA
	if (tim2tga && entry->type == BIGENTRY_TIM)
	{
		ExtractFileBase(entry->name, basename);
		ExtractFilePath(entry->name, path);
		sprintf(entry->name, "%s%s.tga", path, basename); // write correct listfile.txt
		sprintf(outfile, "%s/%s%s.tga", dstdir, path, basename);
		TGAfromTIM(bigf, entry, outfile, bpp16to24);
		return;
	}

	// load file contents
	if (entry->data == NULL)
	{
		entry->data = qmalloc(entry->size);
		BigfileSeekContents(bigf, entry->data, entry);
	}

	// autoconvert raw ADPCM or VAG
	if ((adpcmconvert && entry->type == BIGENTRY_RAW_ADPCM) || (vagconvert && entry->type == BIGENTRY_VAG))
	{
		StripFileExtension(entry->name, basename);
		if (entry->type == BIGENTRY_RAW_ADPCM)
		{
			c = adpcmconvert;
		//	if (psone) // if PlayStation pill.big, thread ADPCM as RAW VAG
		//	{
		//		VAG_Unpack(entry->data, 0, entry->size, &data, &size);
		//		sprintf(inputcmd, "-t s16 -r %i -c 1", entry->adpcmrate);
		//	}
		//	else
		//	{
				data = entry->data;
				size = entry->size;
				sprintf(inputcmd, "-t ima -r %i -c 1", entry->adpcmrate);
		//  }
		}
		else
		{
			c = vagconvert;
			// unpack vag
			VAG_Unpack(entry->data, 64, entry->size, &data, &size);
			sprintf(inputcmd, "-t s16 -r %i -c 1", entry->adpcmrate);
		}

		// try to save
		sprintf(savefile, (c == 3) ? "%s/%s.ogg" : "%s/%s.wav", dstdir, basename);
		if (c == 3)
			sprintf(outputcmd, "-t ogg -C 7");
		else if (c == 2)
			sprintf(outputcmd, "-t wav -e signed-integer");
		else 
			sprintf(outputcmd, "-t wav");
		if (SoX_DataToFile(data, size, "--no-dither", inputcmd, outputcmd, savefile, ""))
			sprintf(entry->name, (c == 3) ? "%s.ogg" : "%s.wav", basename);  // write correct listfile.tx
		else
		{
			//Warning("unable to convert %s, SoX Error #%i, unpacking original", entry->name, GetLastError());
			Warning("unable to convert %s, SoX Error, unpacking original", entry->name);
			BigFileUnpackOriginalEntry(entry, dstdir, false);
		}

		// for VAG, unpack original anyway
		// fixme: make backwards conversion
		if (entry->type == BIGENTRY_VAG)
			BigFileUnpackOriginalEntry(entry, dstdir, true);

		if (data != entry->data)
			qfree(data);
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
			TGAfromRAW(rawblock, entry->rawinfo, outfile, rawnoalign, false, (rawblock->chunks > 5) ? true : false); 
		}
		FreeRawBlock(rawblock);
		// unpack original
		BigFileUnpackOriginalEntry(entry, dstdir, true);
		qfree(entry->data);
		entry->data = NULL;
		return;
	}

	// convert wave file
	if (entry->type == BIGENTRY_RIFF_WAVE)
	{
		// change file extension to wav and write original
		StripFileExtension(entry->name, basename);
		sprintf(entry->name, "%s.wav", basename);
	}

	// unpack original
	BigFileUnpackOriginalEntry(entry, dstdir, false);
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
		if (rawblock->errorcode > 0)
			if (rawblock->errorcode < (int)entry->size)
				printf("%.8x: file read pos %i of %i\n", entry->hash, rawblock->errorcode, (int)entry->size);
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

void BigfileScanFiletype(FILE *f, bigfileentry_t *entry, qboolean scanraw, rawtype_t forcerawtype, qboolean allow_auto_naming)
{
	char name[MAX_BLOODPATH], ext[MAX_BLOODPATH];
	bigentrytype_t autotype;
	bigkentry_t *kentry;
	char *autopath;

	// detect filetype automatically
	autotype = BigfileDetectFiletype(f, entry, scanraw, forcerawtype);
	if (autotype != BIGENTRY_UNKNOWN) 
	{
		entry->type = autotype;
		// automatic path
		if (allow_auto_naming)
		{
			ExtractFileBase(entry->name, name);
			ExtractFileExtension(entry->name, ext);
			autopath = NULL;
			if (autotype == BIGENTRY_RAW_IMAGE)
				autopath = PathForRawType(entry->rawinfo->type);
			if (autopath == NULL)
				autopath = bigentryautopaths[autotype];
			if (!strcmp(ext, bigentryext[BIGENTRY_UNKNOWN]))
				sprintf(entry->name, "%s%s.%s", autopath, name, bigentryext[entry->type]);
			else
				sprintf(entry->name, "%s%s.%s", autopath, name, ext);
		}
		// check klist and pick rawinfo anyway
		kentry = BigfileSearchKList(entry->hash);
		if (kentry != NULL)
		{
			entry->adpcmrate = (int)kentry->adpcmrate;
			if (entry->type == BIGENTRY_RAW_IMAGE)
				entry->rawinfo = kentry->rawinfo;
		}
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
			if (allow_auto_naming)
			{
				ExtractFileName(entry->name, name);
				if (kentry->path[0])
				{
					if (kentry->pathonly)
						sprintf(entry->name, "%s/%s", kentry->path, name);
					else
						sprintf(entry->name, "%s", kentry->path);
				}
				else
				{
					ExtractFileName(entry->name, name);
					ExtractFileExtension(entry->name, ext);
					// automatic path
					autopath = NULL;
					if (entry->type == BIGENTRY_RAW_IMAGE)
						autopath = PathForRawType(entry->rawinfo->type);
					if (autopath == NULL)
						autopath = bigentryautopaths[autotype];
					if (!strcmp(ext, bigentryext[BIGENTRY_UNKNOWN]))
						sprintf(entry->name, "%s%s.%s", autopath, name, bigentryext[entry->type]);
					else
						sprintf(entry->name, "%s%s.%s", autopath, name, ext);
				}
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
		BigfileScanFiletype(f, entry, scanraw, forcerawtype, /*data->namesfromcsv ? false : */true);
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

/*
==========================================================================================

  -bigfile -list [-to]

  lists bigfile contents

==========================================================================================
*/

int BigFile_List(int argc, char **argv)
{
	FILE *f, *f2;
	bigfileheader_t *data;
	bigfileentry_t *entry;
	char name[MAX_BLOODPATH], listfile[MAX_BLOODPATH], exportcsv[MAX_BLOODPATH], typestr[128], extrainfo[128];
	qboolean hashnamesonly;
	int i;

	// check parms
	strcpy(exportcsv, "-");
	strcpy(listfile, "-");
	hashnamesonly = false;
	if (argc > 0) 
	{
		strcpy(listfile, argv[0]);
		for (i = 0; i < argc; i++)
		{
			if (!strcmp(argv[i], "-exportcsv"))
			{
				i++; 
				if (i < argc)
				{
					strlcpy(exportcsv, argv[i], sizeof(exportcsv));
					Verbose("Option: export CSV file '%s'\n", argv[i]);
				}
				continue;
			}
			if (!strcmp(argv[i], "-hashasnames"))
			{
				hashnamesonly = true;
				Verbose("Option: use pure hash names\n");
				continue;
			}
			if (i != 0)
				Warning("unknown parameter '%s'",  argv[i]);
		}
	}

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	data = ReadBigfileHeader(f, bigfile, false, hashnamesonly);
	BigfileScanFiletypes(f, data, true, NULL, RAW_TYPE_UNKNOWN);

	// print or...
	if (listfile[0] == '-')
		BigfileWriteListfile(stdout, data);
	else // output to file
	{
		f2 = SafeOpenWrite(listfile);
		BigfileWriteListfile(f2, data);
		Print("wrote %s\n", listfile);
		fclose(f2);
	}

	// export CSV file
	if (exportcsv[0] != '-')
	{
		Print("Exporting %s...\n", exportcsv);
		f2 = SafeOpenWrite(exportcsv);
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
			ConvSlashU2W(name);
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

void BigFile_ExtractRawImage(int argc, char **argv, char *outfile, bigfileentry_t *entry, rawblock_t *rawblock, char *format)
{
	int i, num, minp, maxp, margin, aver, diff, spritex, spritey, spriteflags;
	sprtype_t spritetype = SPR_VP_PARALLEL;
	rawblock_t *tb1, *tb2, *tb3, *tb4;
	qboolean noalign, nocrop, flip, scale;
	byte pix, shadowpix, shadowalpha;
	byte c[3];
	double colorscale, cscale, alphascale;
	list_t *includelist;
	FILE *f;

	// additional parms
	includelist = NewList();
	shadowalpha = 160;
	noalign = false;
	nocrop = false;
	flip = false;
	margin = 1;
	spritex = 0;
	spritey = 0;
	spriteflags = 0;
	shadowpix = 15;
	alphascale = 1.0f;
	scale = false;
	for (i = 2; i < argc; i++)
	{
		if (!strcmp(argv[i], "-oriented"))
		{
			spritetype = SPR_ORIENTED;
			Verbose("Option: sprite type = ORIENTED\n");
			continue;
		}
		if (!strcmp(argv[i], "-parallel_upright"))
		{
			spritetype = SPR_VP_PARALLEL_UPRIGHT;
			Verbose("Option: sprite type = PARALLEL_UPRIGHT\n");
			continue;
		}
		if (!strcmp(argv[i], "-facing_upright"))
		{
			spritetype = SPR_VP_FACING_UPRIGHT;
			Verbose("Option: sprite type = FACING_UPRIGHT\n");
			continue;
		}
		if (!strcmp(argv[i], "-parallel"))
		{
			spritetype = SPR_VP_PARALLEL;
			Verbose("Option: sprite type = PARALLEL\n");
			continue;
		}
		if (!strcmp(argv[i], "-overhead"))
		{
			spritetype = SPR_OVERHEAD;
			Verbose("Option: sprite type = OVERHEAD\n");
			continue;
		}
		if (!strcmp(argv[i], "-i"))
		{
			i++; 
			if (i < argc)
			{
				ListAdd(includelist, argv[i], true);
				Verbose("Option: include chunks '%s'\n", argv[i]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-bgcolor"))
		{
			i++;
			if (i < argc)
			{
				num = ParseHex(argv[i]);
				rawblock->colormap[0] = (byte)((num >> 16) & 0xFF);
				rawblock->colormap[1] = (byte)((num >> 8) & 0xFF);
				rawblock->colormap[2] = (byte)(num & 0xFF);
				Verbose("Option: custom background color '%i %i %i'\n", rawblock->colormap[0], rawblock->colormap[1], rawblock->colormap[2]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-altcolor"))
		{
			i++;
			if (i < argc)
			{
				pix = (byte)atoi(argv[i]);
				i++;
				if (i < argc)
				{
					num = ParseHex(argv[i]);
					rawblock->colormap[pix*3] = (byte)((num >> 16) & 0xFF);
					rawblock->colormap[pix*3 + 1] = (byte)((num >> 8) & 0xFF);
					rawblock->colormap[pix*3 + 2] = (byte)(num & 0xFF);
					Verbose("Option: replace colormap index #%i by '%i %i %i'\n", rawblock->colormap[pix*3 + 0], rawblock->colormap[pix*3 + 1], rawblock->colormap[pix*3 + 2]);
				}
			}
			continue;
		}
		if (!strcmp(argv[i], "-colormapscale"))
		{
			i++;
			if (i < argc)
			{
				colorscale = atof(argv[i]);
				for (num = 0; num < 768; num++)
					rawblock->colormap[num] = (byte)max(0, min(rawblock->colormap[num]*colorscale, 255));
				Verbose("Option: scale colormap colors by %f'\n", colorscale);
			}
			continue;
		}
		if (!strcmp(argv[i], "-shadowpixel"))
		{
			i++;
			if (i < argc)
			{
				shadowpix = (byte)atoi(argv[i]);
				Verbose("Option: shadow pixel index #%i\n", shadowpix);
			}
			continue;
		}
		if (!strcmp(argv[i], "-shadowcolor"))
		{
			i++;
			if (i < argc)
			{
				num = ParseHex(argv[i]);
				rawblock->colormap[shadowpix*3] = (byte)((num >> 16) & 0xFF);
				rawblock->colormap[shadowpix*3 + 1] = (byte)((num >> 8) & 0xFF);
				rawblock->colormap[shadowpix*3 + 2] = (byte)(num & 0xFF);
				Verbose("Option: custom shadow color '%i %i %i'\n", rawblock->colormap[shadowpix*3], rawblock->colormap[shadowpix*3 + 1], rawblock->colormap[shadowpix*3 + 2]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-shadowalpha"))
		{
			i++;
			if (i < argc)
			{
				shadowalpha = (byte)atoi(argv[i]);
				Verbose("Option: custom shadow alpha %i\n", shadowalpha);
			}
			continue;
		}
		if (!strcmp(argv[i], "-alpha"))
		{
		
			i++;
			if (i < argc)
			{
				alphascale = atof(argv[i]);
				Verbose("Option: alpha scaled by %f\n", alphascale);
			}
			continue;
		}
		if (!strcmp(argv[i], "-replacecolormap"))
		{
			i++;
			if (i < argc)
			{
				Verbose("Option: replacing colormap from %s\n", argv[i]);
				if (!rawblock->colormap)
					Warning("cannot replace colormap, rawfile has no shared palette");
				else if (FileExists(argv[i]))
					ColormapFromTGA(argv[i], rawblock->colormap);
				else
					Warning("cannot replace colormap, %s not found", argv[i]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-noalign"))
		{
			noalign = true;
			Verbose("Option: Disable chunks aligning\n");
			continue;
		}
		if (!strcmp(argv[i], "-nocrop"))
		{
			nocrop = true;
			Verbose("Option: Disable null pixels cropping\n");
			continue;
		}
		if (!strcmp(argv[i], "-margin"))
		{
			i++;
			if (i < argc)
			{
				margin = atoi(argv[i]);
				if (margin < 0)
					margin = 0;
				if (margin > 100)
					margin = 100;
				Verbose("Option: %i-pixel margin\n", margin);
			}
			continue;
		} 
		if (!strcmp(argv[i], "-flip"))
		{
			flip = true;
			Verbose("Option: horizontal flipping\n", margin);
			continue;
		}
		if (!strcmp(argv[i], "-ofs"))
		{
			i++;
			if ((i + 1) < argc)
			{
				spritex = atoi(argv[i]);
				i++;
				spritey = atoi(argv[i]);
				Verbose("Option: offset sprite by x = %i, y = %i\n", spritex, spritey);
			}
			continue;
		}
		if (!strcmp(argv[i], "-nearest2x"))
		{
			scale = true;
			Verbose("Option: Scale by factor 2 with nearest neighbour\n");
			continue;
		}
		if (!strcmp(argv[i], "-colormap2nsx"))
		{
			i++;
			if ((i + 4) < argc)
			{
				f = fopen(argv[i+4], "a");
				if (!f)
					Warning("Option: cannot open %s for input\n", argv[i+3]);
				else
				{
					fputs(argv[i+3], f);
					fputs("=", f);
					// write colormap
					minp = min(255, max(0, atoi(argv[i])));
					maxp = min(255, max(0, atoi(argv[i+1])));
					cscale = atof(argv[i+2]);
					for (num = minp; num < maxp; num++)
					{
						memcpy(c, rawblock->colormap + num*3, 3);
						aver = (int)((c[0] + c[1] + c[2])/3);
						diff = max(c[0], max(c[1], c[2]));
						// reject any color thats too gray
						if (!diff || aver/diff > 0.8)
							continue;
						fprintf(f, "'%i %i %i'", (int)(c[0]*cscale), (int)(c[1]*cscale), (int)(c[2]*cscale));
					}
					fputs("\n", f);
					fclose(f);
					Verbose("Option: Export palette indexes %s-%s scale %s as #%s to %s\n", argv[i], argv[i+1], argv[i+2], argv[i+3], argv[i+4]);
				}
				i += 4;
			}
			continue;
		}
		Warning("unknown parameter '%s'",  argv[i]);
	}

	// perturbare rawblock
	tb1 = NULL;
	tb2 = NULL;
	tb3 = NULL;
	tb4 = NULL;
	if (includelist->items)
	{
		Print("Perturbating...\n");
		rawblock = tb1 = RawblockPerturbate(rawblock, includelist);
	}
	// aligning/cropping/flipping (alternate offsetting) or just flipping (original offsetting)
	if (!noalign)
	{
		Print("Aligning...\n");
		rawblock = tb2 = RawblockAlign(rawblock, margin);
		// go crop unused
		if (!nocrop)
		{
			Print("Cropping...\n");
			rawblock = tb3 = RawblockCrop(rawblock, false, margin);
		}
		// go flip
		if (flip)
		{
			Print("Flipping...\n");
			RawblockFlip(rawblock, false);
		}
	}
	else if (flip)
	{
		Print("Flipping...\n");
		RawblockFlip(rawblock, true);
	}
	// scaling
	if (scale)
	{
		Print("Scaling...\n");
		spritex = spritex * 2;
		spritey = spritey * 2;
		rawblock = tb4 = RawblockScale2x_Nearest(rawblock);
	}

	// write file
	Print("Writing images...\n");
	if (!stricmp(format, "spr"))
		Error("Quake sprites format is not supported!\n");
	else if (!stricmp(format, "spr32"))
	{
		if (noalign)
		{
			for (i = 0; i < rawblock->chunks; i++)
			{
				rawblock->chunk[i].x = (0 - rawblock->chunk[i].width) - rawblock->chunk[i].x;
				rawblock->chunk[i].y = 0 - rawblock->chunk[i].y;
			}
		}
		SPR_WriteFromRawblock(rawblock, outfile, SPR_DARKPLACES, spritetype, spritex, spritey, (float)alphascale, shadowpix, shadowalpha, spriteflags);
	}
	else if (!stricmp(format, "tga"))
		TGAfromRAW(rawblock, entry->rawinfo, outfile, true, true, false);
	else
		Error("unknown sprite format '%s'!\n", format);
	Print("done.\n");

	// free allocated data
	if (tb1) FreeRawBlock(tb1);
	if (tb2) FreeRawBlock(tb2);
	if (tb3) FreeRawBlock(tb3);
	if (tb4) FreeRawBlock(tb4);
}

void BigFile_ExtractSound(int argc, char **argv, char *outfile, bigfileentry_t *entry, char *infileformat, int defaultinputrate, char *format)
{
	char informat[1024], outformat[1024], effects[1024], temp[1024];
	int i, ir;

	if (!soxfound)
		Error("SoX not found!");

	// additional parms
	ir = defaultinputrate;
	strcpy(effects, "");
	for (i = 2; i < argc; i++)
	{
		if (!strcmp(argv[i], "-trimstart"))
		{
			i++;
			if (i < argc)
			{
				strcpy(temp, effects);
				sprintf(effects, "%s trim %s", temp, argv[i]);
				Verbose("Option: trim start by %s seconds\n", argv[i]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-speed"))
		{
			i++;
			if (i < argc)
			{
				strcpy(temp, effects);
				sprintf(effects, "%s speed %s", temp, argv[i]);
				Verbose("Option: sound speed %sx\n", argv[i]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-tempo"))
		{
			i++;
			if ((i+1) < argc)
			{
				strcpy(temp, effects);
				sprintf(effects, "%s tempo %s %s", temp, argv[i], argv[i+1]);
				Verbose("Option: tempo %s %s\n", argv[i], argv[i+1]);
				i++;
			}
			continue;
		}
		if (!strcmp(argv[i], "-pitch"))
		{
			i++;
			if ((i+1) < argc)
			{
				strcpy(temp, effects);
				sprintf(effects, "%s pitch %s %s", temp, argv[i], argv[i+1]);
				Verbose("Option: pitch %s %s\n", argv[i], argv[i+1]);
				i++;
			}
			continue;
		}
		if (!strcmp(argv[i], "-gain"))
		{
			i++;
			if (i < argc)
			{
				strcpy(temp, effects);
				sprintf(effects, "%s gain %s", temp, argv[i]);
				Verbose("Option: volume gain %sDb\n", argv[i]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-bass"))
		{
			i++;
			if (i < argc)
			{
				strcpy(temp, effects);
				sprintf(effects, "%s bass %s", temp, argv[i]);
				Verbose("Option: bass gain %sDb\n", argv[i]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-treble"))
		{
			i++;
			if (i < argc)
			{
				strcpy(temp, effects);
				sprintf(effects, "%s bass %s", temp, argv[i]);
				Verbose("Option: treble gain %sDb\n", argv[i]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-normalize"))
		{
			strcpy(temp, effects);
			sprintf(effects, "%s gain -n", temp);
			Verbose("Option: normalize volume\n");
			continue;
		}
		if (!strcmp(argv[i], "-reverb"))
		{
			strcpy(temp, effects);
			sprintf(effects, "%s reverb", temp);
			Verbose("Option: reverbance\n");
			continue;
		}
		if (!strcmp(argv[i], "-ir"))
		{
			i++;
			if (i < argc)
			{
				ir = atoi(argv[i]);
				Verbose("Option: input rate %ihz\n", ir);
			}
			continue;
		}
		// add to effect
		sprintf(effects, "%s %s", temp, argv[i]);
	}

	// get format
	if (!stricmp(format, "wav"))
		strcpy(outformat, "-t wav -e signed-integer");
	else if (!stricmp(format, "ogg"))
		strcpy(outformat, "-t ogg -C 7");
	else
	{
		strcpy(outformat, format);
		Verbose("Option: using custom format '%s'\n", format);
	}

	// input parms
	strcpy(informat, infileformat);
	if (ir)
	{
		strcpy(temp, informat);
		sprintf(informat, "%s -r %i", temp, ir);
	}

	// run SoX
	if (!SoX_DataToFile(entry->data, entry->size, "--no-dither", informat, outformat, outfile, effects))
		Error("SoX error\n");
}

// "-bigfile c:/pill.big -extract 0AD312F45 0AD312F45.tga"
int BigFile_Extract(int argc, char **argv)
{
	char filename[MAX_BLOODPATH], basename[MAX_BLOODPATH], outfile[MAX_BLOODPATH], format[512], last;
	unsigned int hash;
	bigfileentry_t *entry;
	rawblock_t *rawblock;
	FILE *f;
	byte *data;
	int i, size;

	// read source hash and out file
	if (argc < 2)
		Error("not enough parms");
	if (argv[0][0] == '#')
		sscanf(argv[0], "#%X", &hash);
	else // filename or path
	{
		for (i = 0; i < NUM_CSV_ENTRIES; i++)
			if (!stricmp(argv[0], wheelofdoom_names[i].name))
				break;
		if (i < NUM_CSV_ENTRIES)
		{
			hash = wheelofdoom_names[i].hash;
			Verbose("Hash filename: %.8X\n", hash);
		}
		else Error("Failed to lookup entry name '%s' - no such file\n", argv[0]);
	}
	strcpy(outfile, argv[1]);
	ExtractFileExtension(outfile, format);
	
	// additional parms
	for (i = 2; i < argc; i++)
	{
		if (!strcmp(argv[i], "-f"))
		{
			i++; 
			if (i < argc)
			{
				strlcpy(format, argv[i], sizeof(format));
				Verbose("Option: format '%s'\n", argv[i]);
			}
			continue;
		}
	}

	// check format
	if (!format[0])
		Error("Format is not given\n");

	// open, get entry, scan
	f = SafeOpen(bigfile, "rb");
	entry = ReadBigfileHeaderOneEntry(f, hash);
	if (entry == NULL)
		Error("Failed to find entry %.8X\n", hash);
	BigfileScanFiletype(f, entry, true, RAW_TYPE_UNKNOWN, true);

	// cannot extract empty files
	if (entry->size == 0)
		Error("Empty file\n", hash);
	
	// raw extract (no conversion)
	if (!stricmp(format, "raw"))
	{
		// load file contents
		entry->data = qmalloc(entry->size);
		BigfileSeekContents(f, entry->data, entry);
		// save file
		SaveFile(outfile, entry->data, entry->size);
		qfree(entry->data);
		entry->data = NULL;
		fclose(f);
		return 0;
	}

	// get outfile
	if (outfile == NULL)
		ExtractFileName(entry->name, filename); // pick automatic name
	else
	{
		last = outfile[strlen(outfile)-1];
		if (last != '/' && last != '\\') // full path is given
			strcpy(filename, outfile);
		else // only path is given
		{
			ExtractFileName(entry->name, basename);
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
			// TIM extraction is simple
			if (!stricmp(format, "tga"))
			{
				DefaultExtension(filename, ".tga", sizeof(filename));
				Print("writing %s.\n", filename);
				TGAfromTIM(f, entry, filename, false); 
			}
			else if (!stricmp(format, "tga24") || !format[0])
			{
				DefaultExtension(filename, ".tga", sizeof(filename));
				Print("writing %s.\n", filename);
				TGAfromTIM(f, entry, filename, true); 
			}
			else Error("unknown format '%s'\n", format);
			break;
		case BIGENTRY_RAW_ADPCM:
			// load file contents
			entry->data = qmalloc(entry->size);
			BigfileSeekContents(f, entry->data, entry);
			// process
			if (!stricmp(format, "wav") || !format[0])
			{
				DefaultExtension(filename, ".wav", sizeof(filename));
				BigFile_ExtractSound(argc, argv, outfile, entry, "-t ima -c 1", 11025, "wav");
			}
			else if (!stricmp(format, "ogg"))
			{
				DefaultExtension(filename, ".ogg", sizeof(filename));
				BigFile_ExtractSound(argc, argv, outfile, entry, "-t ima -c 1", 11025, "ogg");
			}
			else Error("unknown format '%s'\n", format);
			// close
			qfree(entry->data);
			entry->data = NULL;
			fclose(f);
			break;
		case BIGENTRY_RIFF_WAVE:
			// load file contents
			entry->data = qmalloc(entry->size);
			BigfileSeekContents(f, entry->data, entry);
			// process
			if (!stricmp(format, "wav") || !format[0])
			{
				DefaultExtension(filename, ".wav", sizeof(filename));
				BigFile_ExtractSound(argc, argv, outfile, entry, "", 0, "wav");
			}
			else if (!stricmp(format, "ogg"))
			{
				DefaultExtension(filename, ".ogg", sizeof(filename));
				BigFile_ExtractSound(argc, argv, outfile, entry, "", 0, "ogg");
			}
			else Error("unknown format '%s'\n", format);
			// close
			qfree(entry->data);
			entry->data = NULL;
			fclose(f);
			break;
		case BIGENTRY_VAG:
			// load file contents
			entry->data = qmalloc(entry->size);
			BigfileSeekContents(f, entry->data, entry);
			// unpack vag
			VAG_Unpack(entry->data, 64, entry->size, &data, &size);
			qfree(entry->data);
			entry->data = data;
			entry->size = size;
			// extract as normal sound then
			if (!stricmp(format, "wav") || !format[0])	
			{
				DefaultExtension(filename, ".wav", sizeof(filename));
				BigFile_ExtractSound(argc, argv, outfile, entry, "-t s16 -c 1", 11025, "wav");
			}
			else if (!stricmp(format, "ogg"))
			{
				DefaultExtension(filename, ".ogg", sizeof(filename));
				BigFile_ExtractSound(argc, argv, outfile, entry, "-t s16 -c 1", 11025, "ogg");
			}
			else Error("unknown format '%s'\n", format);
			// close
			qfree(entry->data);
			entry->data = NULL;
			fclose(f);
			Error("Vag files not supported");
			break;
		case BIGENTRY_RAW_IMAGE:
			// read file contents and convert to rawblock, then pass to extraction func
			entry->data = qmalloc(entry->size);
			BigfileSeekContents(f, entry->data, entry);
			rawblock = RawExtract(entry->data, entry->size, entry->rawinfo, false, false, RAW_TYPE_UNKNOWN);
			entry->data = NULL;
			if (!stricmp(format, "tga") || !format[0])
			{
				DefaultExtension(filename, ".tga", sizeof(filename));
				BigFile_ExtractRawImage(argc, argv, outfile, entry, rawblock, "tga");
			}
			else if (!stricmp(format, "spr32"))
			{
				DefaultExtension(filename, ".spr32", sizeof(filename));
				BigFile_ExtractRawImage(argc, argv, outfile, entry, rawblock, "spr32");
			}
			else Error("unknown format '%s'\n", format);
			FreeRawBlock(rawblock);
			qfree(entry->data);
			break;
		default:
			Error("bad entry type\n");
			break;
	}
	fclose(f);
	return 0;
}

/*
==========================================================================================

  -bigfile -unpack

  unpack whole bigfile to a folder

==========================================================================================
*/

int BigFile_Unpack(int argc, char **argv)
{
	FILE *f, *f2;
	char savefile[MAX_BLOODPATH], dstdir[MAX_BLOODPATH];
	qboolean tim2tga, bpp16to24, nopaths, rawconvert, rawnoalign, psone, hashnamesonly;
	rawtype_t forcerawtype;
	bigfileheader_t *data;
	list_t *ixlist;
	int i, adpcmconvert, vagconvert;

	// parse commandline parms
	strcpy(dstdir, DEFAULT_PACKPATH);
	ixlist = NewList();
	tim2tga = false;
	bpp16to24 = false;
	nopaths = false;
	adpcmconvert = 0;
	vagconvert = 0;
	rawconvert = false;
	forcerawtype = RAW_TYPE_UNKNOWN;
	rawnoalign = false;
	psone = false;
	hashnamesonly = false;
	if (argc > 0)
	{
		if (argv[0][0] != '-')
		{
			strcpy(dstdir, argv[0]);
			Verbose("Option: destination directory '%s'\n", dstdir);
		}
		for (i = 0; i < argc; i++)
		{
			if (!strcmp(argv[i], "-x"))
			{
				i++; 
				if (i < argc)
					ListAdd(ixlist, argv[i], false);
				Verbose("Option: exclude mask '%s'\n", argv[i]);
				continue;
			}
			if (!strcmp(argv[i], "-i"))
			{
				i++; 
				if (i < argc)
					ListAdd(ixlist, argv[i], true);
				Verbose("Option: include mask '%s'\n", argv[i]);
				continue;
			}
			if (!strcmp(argv[i], "-hashasnames"))
			{
				hashnamesonly = true;
				Verbose("Option: use pure hash names\n");
				continue;
			}
			if (!strcmp(argv[i], "-tim2tga"))
			{
				tim2tga = true;
				Verbose("Option: TIM->TGA conversion\n");
				continue;
			}
			if (!strcmp(argv[i], "-16to24"))
			{
				bpp16to24 = true;
				Verbose("Option: Targa compatibility mode (converting 16-bit to 24-bit)\n");
				continue;
			}
			if (!strcmp(argv[i], "-nopaths"))
			{
				nopaths = true;
				Verbose("Option: Disallow klist-set subpaths\n");
				continue;
			}
			if (!strcmp(argv[i], "-adpcm2wav"))
			{
				adpcmconvert = 1;
				Verbose("Option: ADPCM->WAV (native) conversion\n");
				continue;
			}
			if (!strcmp(argv[i], "-adpcm2pcm"))
			{
				adpcmconvert = 2;
				Verbose("Option: ADPCM->WAV (PCM) conversion\n");
				continue;
			}
			if (!strcmp(argv[i], "-adpcm2ogg"))
			{
				adpcmconvert = 3;
				Verbose("Option: ADPCM->OGG (Vorbis quality 5) conversion\n");
				continue;
			}
			if (!strcmp(argv[i], "-vag2wav"))
			{
				vagconvert = 2;
				Verbose("Option: VAG->WAV (PCM native) conversion\n");
				continue;
			}
			if (!strcmp(argv[i], "-vag2ogg"))
			{
				vagconvert = 3;
				Verbose("Option: VAG->OGG (Vorbis quality 5) conversion\n");
				continue;
			}
			if (!strcmp(argv[i], "-forcerawtype"))
			{
				i++;
				if (i < argc)
				{
					forcerawtype = ParseRawType(argv[i]);
					Verbose("Option: Guessing all raw images is %s\n", UnparseRawType(forcerawtype));
				}
				continue;
			}
			if (!strcmp(argv[i], "-raw2tga"))
			{
				rawconvert = true;
				Verbose("Option: Converting raw images to TGA\n");
				continue;
			}
			if (!strcmp(argv[i], "-rawnoalign"))
			{
				rawnoalign = true;
				Verbose("Option: Disable RAW images aligning\n");
				continue;
			}
			if (!strcmp(argv[i], "-psone"))
			{
				psone = true;
				Verbose("Option: PlayStation pill.big\n");
				continue;
			}
			if (i != 0)
				Warning("unknown parameter '%s'",  argv[i]);
		}
	}

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	data = ReadBigfileHeader(f, bigfile, false, hashnamesonly);
	BigfileScanFiletypes(f, data, true, ixlist->items ? ixlist : NULL, forcerawtype);

	// export all files
	for (i = 0; i < (int)data->numentries; i++)
	{
		if (ixlist->items)
			if (!MatchIXList(&data->entries[i], ixlist, true, true))
				continue;
		Pacifier("unpacking entry %i of %i...", i + 1, data->numentries);
		BigFileUnpackEntry(f, &data->entries[i], dstdir, tim2tga, bpp16to24, nopaths, adpcmconvert, vagconvert, rawconvert, forcerawtype, rawnoalign, psone);
	}
	PacifierEnd();

	// write listfile
	sprintf(savefile, "%s/listfile.txt", dstdir);
	f2 = SafeOpenWrite(savefile);
	BigfileWriteListfile(f2, data);
	fclose(f2);
	Print("wrote %s\ndone.\n", savefile);

	fclose (f);
	return 0;
}

int BigFile_Pack(int argc, char **argv)
{
	FILE *f;
	bigfileheader_t *data;
	bigfileentry_t *entry;
	tim_image_t *tim;
	char savefile[MAX_BLOODPATH], basename[MAX_BLOODPATH], srcdir[MAX_BLOODPATH], ext[128];
	byte *contents;
	int i, k, size;

	// check parms
	strcpy(srcdir, DEFAULT_PACKPATH);
	if (argc > 0)
	{
		if (argv[0][0] != '-')
		{
			strcpy(srcdir, argv[0]);
			Verbose("Option: source directory '%s'\n", srcdir);
		}
		for (i = 0; i < argc; i++)
		{
			if (i != 0)
				Warning("unknown parameter '%s'",  argv[i]);
		}
	}

	// open listfile
	data = BigfileOpenListfile(srcdir);

	// open bigfile
	f = fopen(bigfile, "rb");
	if (f)
	{
		Verbose("%s already exists, overwriting\n", bigfile);
		fclose(f);
	}
	f = SafeOpenWrite(bigfile);

	// write entries count, write headers later
	SafeWrite(f, &data->numentries, 4);
	fseek(f, data->numentries*12, SEEK_CUR);

	// write files
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		if (entry->size == 0)
			continue; // skip null files

		Pacifier("writing entry %i of %i...\r", i + 1, data->numentries);

		ExtractFileExtension(entry->name, ext);
		// autoconverted TIM
		if (!strcmp(ext, "tga") && entry->type == BIGENTRY_TIM)
		{
			sprintf(savefile, "%s/%s", srcdir, entry->name);
			entry->data = TIM_LoadFromTarga(savefile, entry->timtype[0]);
			size = ((tim_image_t *)entry->data)->filelen;
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
			entry->size = size;
		}
		// autoconverted WAV/OGG
		else if ((!strcmp(ext, "wav") || !strcmp(ext, "ogg")) && entry->type == BIGENTRY_RAW_ADPCM)
		{
			sprintf(savefile, "%s/%s", srcdir, entry->name);
			if (!SoX_FileToData(savefile, "--no-dither", "", "-t ima -c 1", &size, &contents, ""))
				Error("unable to convert %s, SoX Error\n", entry->name);
			entry->data = contents;
			entry->size = size;
			// write
			SafeWrite(f, entry->data, entry->size);
			qfree(entry->data);
		}
		// just write
		else
		{
			sprintf(savefile, "%s/%s", srcdir, entry->name);
			entry->size = LoadFile(savefile, &entry->data);
			SafeWrite(f, entry->data, entry->size);
			qfree(entry->data);
		}
	}
	PacifierEnd();

	// write headers
	Verbose("Recalculating offsets...\n", bigfile);
	BigfileHeaderRecalcOffsets(data);
	fseek(f, 4, SEEK_SET);
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];
		Pacifier("writing header %i of %i...", i + 1, data->numentries);
		SafeWrite(f, &entry->hash, 4);
		SafeWrite(f, &entry->size, 4);
		SafeWrite(f, &entry->offset, 4);
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

int BigFile_Main(int argc, char **argv)
{
	int i = 1, returncode = 0;
	char knownfiles[MAX_BLOODPATH], *c;

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

	// check for special directives
	strcpy(knownfiles, "-");
	while(i < argc)
	{
		if (!strcmp(argv[i], "-klist"))
		{
			i++; 
			if (i < argc)
				strlcpy(knownfiles, argv[i], sizeof(knownfiles));
			i++; 
			continue;
		}
		break;
	}

	// load up knowledge base
	// FIXME: stupid code, rewrite
	if (knownfiles[0] == '-')
		bigklist = BigfileLoadKList("klist.txt", false);
	else
	{
		Print("Using custom known-files-list %s\n", knownfiles);
		bigklist = BigfileLoadKList(knownfiles, true);
	}

	// now we have to parse action
	if (argc <= i)
		Error("no action specified, try %s -help", progname);
	if (!strcmp(argv[i], "-list")) 
		returncode = BigFile_List(argc-i-1, argv+i+1);
	else if (!strcmp(argv[i], "-extract"))
		returncode = BigFile_Extract(argc-i-1, argv+i+1);
	else if (!strcmp(argv[i], "-unpack"))
		returncode = BigFile_Unpack(argc-i-1, argv+i+1);
	else if (!strcmp(argv[i], "-pack"))
		returncode = BigFile_Pack(argc-i-1, argv+i+1);
	else
		Warning("unknown action %s", argv[i]);

	return returncode;
}