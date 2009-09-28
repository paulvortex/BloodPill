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

void BigFileUnpackEntry(FILE *bigf, bigfileentry_t *entry, char *dstdir, qboolean tim2tga, qboolean bpp16to24, qboolean nopaths, qboolean vagconvert, qboolean vagpcm, qboolean vagogg, qboolean rawconvert, rawtype_t forcerawtype)
{
	char savefile[MAX_BLOODPATH], basename[MAX_BLOODPATH], path[MAX_BLOODPATH];
	char inputcmd[512], outputcmd[512];
	tim_image_t *tim;
	FILE *f;
	int i;

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
		BigfileSeekFile(bigf, entry);
		for (i = 0; i < entry->timlayers; i++)
		{
			StripFileExtension(entry->name, basename);
			// extract base
			tim = TIM_LoadFromStream(bigf);
			if (i == 0)
				sprintf(savefile, "%s/%s.tga", dstdir, basename);
			else
				sprintf(savefile, "%s/%s_layer%i.tga", dstdir, basename, i);
			if (tim->error)
				Error("error saving %s: %s\n", savefile, tim->error);
		
			// write basefile
			sprintf(entry->name, "%s.tga", basename); // write correct listfile.txt
			TIM_WriteTarga(tim, savefile, bpp16to24);

			// write maskfile
			if (tim->pixelmask != NULL)
			{
				if (i == 0)
					sprintf(savefile, "%s/%s_mask.tga", dstdir, basename); 
				else
					sprintf(savefile, "%s/%s_layer%i_mask.tga", dstdir, basename, i);
				TIM_WriteTargaGrayscale(tim->pixelmask, tim->dim.xsize, tim->dim.ysize, savefile);
			}
			FreeTIM(tim);
		}
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
		ExtractFileBase(entry->name, basename);
		sprintf(savefile, "%s/%s", dstdir, basename);
		RawExtract(savefile, entry->data, entry->size, entry->rawinfo, false, false, forcerawtype);
	}

	// unpack original
	sprintf(savefile, "%s/%s", dstdir, entry->name);
	SaveFile(savefile, entry->data, entry->size);
	qfree(entry->data);
	entry->data = NULL;
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
		sprintf(entry->name, "%.8X.%s", read[0], bigentryext[BIGENTRY_UNKNOWN]);
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
// this functino laso does autoconvert job
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

	// calc stats
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

	// emit stats
	if (stats[BIGENTRY_RAW_ADPCM])
		Print(" %6i raw ADPCM\n", stats[BIGENTRY_RAW_ADPCM]);
	if (stats[BIGENTRY_RIFF_WAVE])
		Print(" %6i RIFF WAVE\n", stats[BIGENTRY_RIFF_WAVE]);
	// TIM
	if (timstats[0])
		Print(" %6i 4-bit TIM\n", timstats[0]);
	if (timstats[1])
		Print(" %6i 8-bit TIM\n", timstats[1]);
	if (timstats[2])
		Print(" %6i 16-bit TIM\n", timstats[2]);
	if (timstats[3])
		Print(" %6i 24-bit TIM\n", timstats[3]);
	if (stats[BIGENTRY_TIM])
		Print(" %6i TIM total\n", stats[BIGENTRY_TIM]);
	// RAW
	if (rawstats[RAW_TYPE_0])
		Print(" %6i raw type 0\n", rawstats[RAW_TYPE_0]);
	if (rawstats[RAW_TYPE_1A])
		Print(" %6i raw type 1A\n", rawstats[RAW_TYPE_1A]);
	if (rawstats[RAW_TYPE_1])
		Print(" %6i raw type 1\n", rawstats[RAW_TYPE_1]);
	if (rawstats[RAW_TYPE_2])
		Print(" %6i raw type 2\n", rawstats[RAW_TYPE_2]);
	if (stats[BIGENTRY_RAW_IMAGE])
		Print(" %6i raw image total\n", stats[BIGENTRY_RAW_IMAGE]);
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

qboolean BigFileScanRaw(FILE *f, bigfileentry_t *entry, rawtype_t forcerawtype)
{
	unsigned char *filedata;
	rawinfo_t *rawinfo;

	// load file contents
	BigfileSeekFile(f, entry);
	filedata = qmalloc(entry->size);
	if (fread(filedata, entry->size, 1, f) < 1)
	{
		free(filedata);
		return false;
	}

	// check all raw types
	rawinfo = NewRawInfo();
	if (RawExtract("", filedata, entry->size, rawinfo, true, false, forcerawtype) >= 0)
	{
		entry->rawinfo = rawinfo;
		free(filedata);
		return true;
	}

	// not found
	free(filedata);
	free(rawinfo);
	return false;
}

bigentrytype_t BigfileDetectFiletype(FILE *f, bigfileentry_t *entry, qboolean scanraw, rawtype_t forcerawtype)
{
	if (BigFileScanTIM(f, entry))
		return BIGENTRY_TIM;
	if (BigFileScanRiffWave(f, entry))
		return BIGENTRY_RIFF_WAVE;
	if (scanraw)
		if (BigFileScanRaw(f, entry, forcerawtype))
			return BIGENTRY_RAW_IMAGE;
	return BIGENTRY_UNKNOWN;
}

void BigfileScanFiletypes(FILE *f, bigfileheader_t *data, qboolean scanraw, rawtype_t forcerawtype)
{
	fpos_t fpos;
	bigentrytype_t autotype;
	bigfileentry_t *entry;
	bigkentry_t *kentry;
	int i;

	fgetpos(f, &fpos);
	// scan for filetypes
	for (i = 0; i < (int)data->numentries; i++)
	{
		entry = &data->entries[i];

		// ignore null-sized
		if (!entry->size)
			continue;

		Pacifier("scanning type for entry %i of %i...", i + 1, data->numentries);
		
		// detect filetype automatically
		autotype = BigfileDetectFiletype(f, entry, scanraw, forcerawtype);
		if (autotype != BIGENTRY_UNKNOWN) 
		{
			entry->type = autotype;
			sprintf(entry->name, "%.8X.%s", entry->hash, bigentryext[entry->type]);
		}
		// check listfile
		else
		{
			kentry = BigfileSearchKList(entry->hash);
			if (kentry != NULL)
			{
				if (kentry->path[0])
					sprintf(entry->name, "%s", kentry->path);
				else
					sprintf(entry->name, "%.8X.%s", entry->hash, bigentryext[entry->type]);
				entry->type = (bigentrytype_t)kentry->type;
				entry->adpcmrate = (int)kentry->adpcmrate;
				if (entry->type == BIGENTRY_RAW_IMAGE)
					entry->rawinfo = kentry->rawinfo;
			}
		}
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
	BigfileScanFiletypes(f, data, true, RAW_TYPE_UNKNOWN);

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

  Actions

==========================================================================================
*/

int BigFile_List(int argc, char **argv, char *listfile, qboolean scanraw)
{
	FILE *f, *f2;
	bigfileheader_t *data;

	// open file & load header
	f = SafeOpen(bigfile, "rb");
	data = ReadBigfileHeader(f, bigfile, scanraw);
	BigfileScanFiletypes(f, data, true, RAW_TYPE_UNKNOWN);

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
	Print("done.\n");
	fclose (f);
	return 0;
}

int BigFile_Unpack(int argc, char **argv, char *dstdir, qboolean tim2tga, qboolean bpp16to24, qboolean nopaths, qboolean vagconvert, qboolean vagpcm, qboolean vagogg, qboolean scanraw, qboolean rawconvert, rawtype_t forcerawtype)
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
	BigfileScanFiletypes(f, data, scanraw, forcerawtype);

	// make directory
	Q_mkdir(dstdir);
	Verbose("%s folder created\n", dstdir);
	
	// export all files
	for (i = 0; i < (int)data->numentries; i++)
	{
		Pacifier("unpacking entry %i of %i...", i + 1, data->numentries);
		BigFileUnpackEntry(f, &data->entries[i], dstdir, tim2tga, bpp16to24, nopaths, vagconvert, vagpcm, vagogg, rawconvert, forcerawtype);
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

int BigFile_Main(int argc, char **argv)
{
	int i = 1, k, returncode = 0;
	char *tofile, *srcdir, *dstdir, *knownfiles, *c;
	qboolean tim2tga, bpp16to24, lowmem, nopaths, vagconvert, vagpcm, vagogg, scanraw, rawconvert;
	rawtype_t forcerawtype;

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
	nopaths = false;
	vagconvert = false;
	vagpcm = false;
	vagogg = false;
	scanraw = false;
	rawconvert = false;
	forcerawtype = RAW_TYPE_UNKNOWN;
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
			k++;
			if (k < argc)
				forcerawtype = ParseRawType(argv[k]);
		}
		else if (!strcmp(argv[k],"-rawconvert"))
			rawconvert = true;

		
	}
	
	// load up knowledge base
	if (knownfiles[0] == '-')
		bigklist = BigfileLoadKList("klist.txt", true);
	else
		bigklist = BigfileLoadKList(knownfiles, false);

	// action
	if (!strcmp(argv[i], "-list"))
		returncode = BigFile_List(argc-i, argv+i, tofile, scanraw);
	else if (!strcmp(argv[i], "-analyse"))
		returncode = BigFile_Analyse(argc-i, argv+i, tofile);
	else if (!strcmp(argv[i], "-unpack"))
		returncode = BigFile_Unpack(argc-i, argv+i, dstdir, tim2tga, bpp16to24, nopaths, vagconvert, vagpcm, vagogg, scanraw, rawconvert, forcerawtype);
	else if (!strcmp(argv[i], "-pack"))
		returncode = BigFile_Pack(argc-i, argv+i, srcdir, lowmem);
	else
		Warning("unknown option %s", argv[i]);

	return returncode;
}