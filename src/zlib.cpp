////////////////////////////////////////////////////////////////
//
// Blood Pill - zlib support for PK3 writing
// coded by Pavel [VorteX] Timofeyev and placed to public domain
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
#include "zlib.h"
#include "mem.h"

// dll pointer
static dllhandle_t zlib_dll = NULL;

// dll names to load
const char* zlib_dllnames[] =
{
#if defined(WIN32)
	"zlib1.dll",
#elif defined(MACOSX)
	"libz.dylib",
#else
	"libz.so.1",
	"libz.so",
#endif
	NULL
};

// functions to import
static dllfunction_t zlib_funcs[] =
{
	{"inflate",			(void **) &zlib_inflate},
	{"inflateEnd",		(void **) &zlib_inflateEnd},
	{"inflateInit2_",	(void **) &zlib_inflateInit2_},
	{"inflateReset",	(void **) &zlib_inflateReset},
	{"deflateInit2_",   (void **) &zlib_deflateInit2_},
	{"deflateEnd",      (void **) &zlib_deflateEnd},
	{"deflate",         (void **) &zlib_deflate},
	{NULL, NULL}
};

// deflating
#define PK3_MAX_FILESIZE 1024*1024*100 // 100 megs
unsigned char pk3buf[PK3_MAX_FILESIZE];

/*
====================
PK3_CreateFile

PK3 writing
====================
*/

pk3_file_t *PK3_Create(char *filepath)
{
	pk3_file_t *pk3;

	if (!PK3_Enabled())
		Error("PK3_Create: zlib not enabled!");

	pk3 = (pk3_file_t *)mem_alloc(sizeof(pk3_file_t));
	CreatePath(filepath);
	pk3->file = SafeOpen(filepath, "wb");
	pk3->numfiles = 0;

	return pk3;
}

// creates a new pk3 file handle
pk3_entry_t *PK3_CreateFile(pk3_file_t *pk3, char *filename)
{
	pk3_entry_t *entry;

	if (!PK3_Enabled())
		Error("PK3_CreateFile: zlib not enabled!");

	// allocate file
	if (pk3->numfiles >= MAX_PK3_FILES)
		Error("PK3_CreateFile: MAX_PK3_FILES exceeded!");
	entry = (pk3_entry_t *)mem_alloc(sizeof(pk3_entry_t));
	memset(entry, 0, sizeof(pk3_entry_t));
	pk3->files[pk3->numfiles] = entry;
	pk3->numfiles++;

	// set filename under central directory
	entry->filenamelen = strlen(filename)+1;
	strcpy(entry->filename, filename);

	return entry;
}

// compress file contents into PK3 file
void PK3_CompressFileData(pk3_file_t *pk3, pk3_entry_t *entry, unsigned char *filedata, unsigned int datasize)
{
	unsigned int compressed_size;
	int ret;

	if (!PK3_Enabled())
		Error("PK3_CompressFile: zlib not enabled!");

	// allocate file compressor
	memset(&entry->stream, 0, sizeof(z_stream));
	entry->stream.zalloc = Z_NULL;
    entry->stream.zfree = Z_NULL;
    entry->stream.opaque = Z_NULL;
	if (zlib_deflateInit2(&entry->stream, 8, Z_DEFLATED, -MAX_WBITS, Z_MEMLEVEL_DEFAULT, Z_BINARY) != Z_OK)
		Error("PK3_CompressFile: failed to allocate compressor");

	// compress
	entry->stream.avail_in = datasize;
	entry->stream.next_in = filedata;
	entry->stream.avail_out = PK3_MAX_FILESIZE;
    entry->stream.next_out = pk3buf;
	if (zlib_deflate(&entry->stream, Z_FINISH) == Z_STREAM_ERROR)
		Error("PK3_CompressFile: error during compression");
	compressed_size = PK3_MAX_FILESIZE - entry->stream.avail_out;
	if (compressed_size >= PK3_MAX_FILESIZE)
		Error("PK3_CompressFile: PK3_MAX_FILESIZE exceeded!");

	// register in central directory
	entry->xver = 20; // File is a folder (directory), is compressed using Deflate compression
	entry->bitflag = 2; // maximal compression
	entry->crc32 = crc32(filedata, datasize);
	entry->compression = 8; // Deflate
	entry->csize = compressed_size;
	entry->usize = datasize;
	entry->offset = ftell(pk3->file);

	// write local file header & file contents
	ret = 0x04034b50;
	fwrite(&ret, 4, 1, pk3->file);
	fwrite(&entry->xver, 2, 1, pk3->file);
	fwrite(&entry->bitflag, 2, 1, pk3->file);
	fwrite(&entry->compression, 2, 1, pk3->file);
	ret = 0;
	fwrite(&ret, 2, 1, pk3->file);
	fwrite(&ret, 2, 1, pk3->file);
	fwrite(&entry->crc32, 4, 1, pk3->file);
	fwrite(&entry->csize, 4, 1, pk3->file);
	fwrite(&entry->usize, 4, 1, pk3->file);
	fwrite(&entry->filenamelen, 2, 1, pk3->file);
    fwrite(&ret, 2, 1, pk3->file);
	fwrite(entry->filename, entry->filenamelen, 1, pk3->file);
	fwrite(pk3buf, entry->csize, 1, pk3->file);
	if (ferror(pk3->file))
		Error("PK3_CompressFile: failed write pk3");

	zlib_deflateEnd(&entry->stream);
}

// write out a whole file to the pk3
void PK3_AddFile(pk3_file_t *pk3, char *filename, unsigned char *filedata, unsigned int datasize)
{
	pk3_entry_t *entry;
	
	entry = PK3_CreateFile(pk3, filename);
	PK3_CompressFileData(pk3, entry, filedata, datasize);
}

// add file from disk to pk3
void PK3_AddExternalFile(pk3_file_t *pk3, char *filename, char *externalfile)
{
	byte *filedata;
	pk3_entry_t *entry;
	unsigned int datasize;

	datasize = LoadFile(externalfile, &filedata);
	entry = PK3_CreateFile(pk3, filename);
	PK3_CompressFileData(pk3, entry, (byte *)filedata, datasize);
	mem_free(filedata);
}

#ifdef __CMDLIB_WRAPFILES__
// add wrapped files to PK3
void PK3_AddWrappedFiles(pk3_file_t *pk3)
{
	int i, numfiles;
	pk3_entry_t *entry;
	char *filename;
	byte *filedata;
	unsigned int datasize;

	numfiles = CountWrappedFiles();
	for (i = 0; i < numfiles; i++)
	{
		datasize = LoadWrappedFile(i, &filedata, &filename);
		entry = PK3_CreateFile(pk3, filename);
		PK3_CompressFileData(pk3, entry, (byte *)filedata, datasize);
		mem_free(filedata);
	}
	FreeWrappedFiles();
}
#endif

// write PK3 foot and close it
void PK3_Close(pk3_file_t *pk3)
{
	unsigned short i;
	unsigned int ret, cdofs, cdsize;
	pk3_entry_t *entry;

	if (!PK3_Enabled())
		Error("PK3_Close: zlib not enabled!");

	// write central directory and file tail
	cdofs = ftell(pk3->file);
	for (i = 0; i < pk3->numfiles; i++)
	{
		entry = pk3->files[i];
		ret = 0x02014b50;
		fwrite(&ret, 4, 1, pk3->file);
		ret = 0;
		fwrite(&ret, 2, 1, pk3->file);
		fwrite(&entry->xver, 2, 1, pk3->file);
		fwrite(&entry->bitflag, 2, 1, pk3->file);
		fwrite(&entry->compression, 2, 1, pk3->file);
		fwrite(&ret, 2, 1, pk3->file);
		fwrite(&ret, 2, 1, pk3->file);
		fwrite(&entry->crc32, 4, 1, pk3->file);
		fwrite(&entry->csize, 4, 1, pk3->file);
		fwrite(&entry->usize, 4, 1, pk3->file);
		fwrite(&entry->filenamelen, 2, 1, pk3->file);
		fwrite(&ret, 2, 1, pk3->file);
		fwrite(&ret, 2, 1, pk3->file);
		fwrite(&ret, 2, 1, pk3->file);
		fwrite(&ret, 2, 1, pk3->file);
		fwrite(&ret, 4, 1, pk3->file);
		fwrite(&entry->offset, 4, 1, pk3->file);
		fwrite(entry->filename, entry->filenamelen, 1, pk3->file);
		if (ferror(pk3->file))
			Error("PK3_Close: failed write pk3");
		mem_free(entry);
	}
	cdsize = ftell(pk3->file) - cdofs;

	// write end of central directory
	ret = 0x06054b50;
	fwrite(&ret, 4, 1, pk3->file);
	ret = 0;
	fwrite(&ret, 2, 1, pk3->file);
	fwrite(&ret, 2, 1, pk3->file);
	fwrite(&pk3->numfiles, 2, 1, pk3->file);
	fwrite(&pk3->numfiles, 2, 1, pk3->file);
	fwrite(&cdsize, 4, 1, pk3->file);
	fwrite(&cdofs, 4, 1, pk3->file);
	fwrite(&ret, 2, 1, pk3->file);
	if (ferror(pk3->file))
		Error("PK3_Close: failed write pk3");

	fclose(pk3->file);
	mem_free(pk3);
}


/*
====================
PK3_CloseLibrary

Unload the Zlib DLL
====================
*/

void PK3_CloseLibrary(void)
{
	UnloadDll(&zlib_dll);
}

/*
====================
PK3_OpenLibrary

Try to load the Zlib DLL
====================
*/

bool PK3_OpenLibrary(bool verbose)
{
	if (zlib_dll)
		return true;
	return LoadDll(zlib_dllnames, &zlib_dll, zlib_funcs, verbose);
}

/*
====================
PK3_Enabled

See if zlib (and pk3 routines) is available
====================
*/

bool PK3_Enabled(void)
{
	PK3_OpenLibrary(false); // to be safe
	return (zlib_dll != 0);
}
