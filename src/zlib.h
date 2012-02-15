////////////////////////////////////////////////////////////////
//
// Blood Pill - zlib support for PK3 writing
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

#include "cmdlib.h"

#ifndef __ZLIB_H__
#define __ZLIB_H__

// Zlib constants (from zlib.h)
#define Z_SYNC_FLUSH	2
#define MAX_WBITS		15
#define Z_OK			0
#define Z_STREAM_END	1
#define Z_STREAM_ERROR  (-2)
#define Z_DATA_ERROR    (-3)
#define Z_MEM_ERROR     (-4)
#define Z_BUF_ERROR     (-5)
#define ZLIB_VERSION	"1.2.3"

#define Z_BINARY 0
#define Z_DEFLATED 8
#define Z_MEMLEVEL_DEFAULT 8

#define Z_NULL 0
#define Z_DEFAULT_COMPRESSION (-1)
#define Z_NO_FLUSH 0
#define Z_SYNC_FLUSH 2
#define Z_FULL_FLUSH 3
#define Z_FINISH 4

/*! Zlib stream (from zlib.h)
 * \warning: some pointers we don't use directly have
 * been cast to "void*" for a matter of simplicity
 */
typedef struct
{
	unsigned char	*next_in;	///< next input byte
	unsigned int	avail_in;	///< number of bytes available at next_in
	unsigned long	total_in;	///< total nb of input bytes read so far

	unsigned char	*next_out;	///< next output byte should be put there
	unsigned int	avail_out;	///< remaining free space at next_out
	unsigned long	total_out;	///< total nb of bytes output so far

	char			*msg;		///< last error message, NULL if no error
	void			*state;		///< not visible by applications

	void			*zalloc;	///< used to allocate the internal state
	void			*zfree;		///< used to free the internal state
	void			*opaque;	///< private data object passed to zalloc and zfree

	int				data_type;	///< best guess about the data type: ascii or binary
	unsigned long	adler;		///< adler32 value of the uncompressed data
	unsigned long	reserved;	///< reserved for future use
} z_stream;

// dll functions to import
static int ( *zlib_inflate) (z_stream* strm, int flush);
static int ( *zlib_inflateEnd) (z_stream* strm);
static int ( *zlib_inflateInit2_) (z_stream* strm, int windowBits, const char *version, int stream_size);
static int ( *zlib_inflateReset) (z_stream* strm);
static int ( *zlib_deflateInit2_) (z_stream* strm, int level, int method, int windowBits, int memLevel, int strategy, const char *version, int stream_size);
static int ( *zlib_deflateEnd) (z_stream* strm);
static int ( *zlib_deflate) (z_stream* strm, int flush);

#define zlib_inflateInit2(strm, windowBits) zlib_inflateInit2_((strm), (windowBits), ZLIB_VERSION, sizeof(z_stream))
#define zlib_deflateInit2(strm, level, method, windowBits, memLevel, strategy) zlib_deflateInit2_((strm), (level), (method), (windowBits), (memLevel), (strategy), ZLIB_VERSION, sizeof(z_stream))

#endif

// exporting pk3file structure
#define MAX_PK3_FILES 65536
typedef struct pk3_entry_s
{
	z_stream stream;
	unsigned short	xver; // version needed to extract
	unsigned short	bitflag; // general purpose bit flag
	unsigned short	compression; // compression method
	unsigned short	mtime; // last mod file time
	unsigned short	mdate; // last mod file date 
	unsigned int	crc32; // crc-32
	unsigned int	csize; // compressed size
	unsigned int	usize; // uncompressed size 
	unsigned int	offset; // relative offset of local header 4 bytes
	unsigned short	filenamelen;
	char			filename[1024];
}pk3_entry_t;

typedef struct pk3_file_s
{
	FILE		*file;
	pk3_entry_t	*files[MAX_PK3_FILES];
	unsigned short numfiles;
	
}pk3_file_t;

// pk3 writing
pk3_file_t *PK3_Create(char *filepath);
pk3_entry_t *PK3_CreateFile(pk3_file_t *pk3, char *filename);
void PK3_CompressFileData(pk3_file_t *pk3, pk3_entry_t *entry, unsigned char *filedata, unsigned int datasize);
void PK3_AddFile(pk3_file_t *pk3, char *filename, unsigned char *filedata, unsigned int datasize);
void PK3_AddExternalFile(pk3_file_t *pk3, char *filename, char *externalfile);
void PK3_Close(pk3_file_t *pk3);

#ifdef __CMDLIB_WRAPFILES__
void PK3_AddWrappedFiles(pk3_file_t *pk3);
#endif

// functions to use
void PK3_CloseLibrary(void);
bool PK3_OpenLibrary(bool verbose);
bool PK3_Enabled(void);
