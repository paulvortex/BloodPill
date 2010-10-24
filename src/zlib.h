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

/// inside a package (PAK or PK3)
#define QFILE_FLAG_PACKED (1 << 0)
/// file is compressed using the deflate algorithm (PK3 only)
#define QFILE_FLAG_DEFLATED (1 << 1)
/// file is actually already loaded data
#define QFILE_FLAG_DATA (1 << 2)
/// real file will be removed on close
#define QFILE_FLAG_REMOVE (1 << 3)

#define FILE_BUFF_SIZE 2048
typedef struct
{
	z_stream	zstream;
	size_t		comp_length;			///< length of the compressed file
	size_t		in_ind, in_len;			///< input buffer current index and length
	size_t		in_position;			///< position in the compressed file
	unsigned char		input [FILE_BUFF_SIZE];
} ztoolkit_t;

struct qfile_s
{
	int				flags;
	int				handle;					///< file descriptor
	size_t			real_length;			///< uncompressed file size (for files opened in "read" mode)
	size_t			position;				///< current position in the file
	size_t			offset;					///< offset into the package (0 if external file)
	int				ungetc;					///< single stored character from ungetc, cleared to EOF when read

	// Contents buffer
	size_t			buff_ind, buff_len;		///< buffer current index and length
	unsigned char	buff [FILE_BUFF_SIZE];

	ztoolkit_t*		ztk; ///< For zipped files.

	const unsigned char *data;	///< For data files.

	const char *filename; ///< Kept around for QFILE_FLAG_REMOVE, unused otherwise
};

// dll functions to import
static int (*qz_inflate) (z_stream* strm, int flush);
static int (*qz_inflateEnd) (z_stream* strm);
static int (*qz_inflateInit2_) (z_stream* strm, int windowBits, const char *version, int stream_size);
static int (*qz_inflateReset) (z_stream* strm);
static int (*qz_deflateInit2_) (z_stream* strm, int level, int method, int windowBits, int memLevel, int strategy, const char *version, int stream_size);
static int (*qz_deflateEnd) (z_stream* strm);
static int (*qz_deflate) (z_stream* strm, int flush);

#define qz_inflateInit2(strm, windowBits) \
        qz_inflateInit2_((strm), (windowBits), ZLIB_VERSION, sizeof(z_stream))
#define qz_deflateInit2(strm, level, method, windowBits, memLevel, strategy) \
        qz_deflateInit2_((strm), (level), (method), (windowBits), (memLevel), (strategy), ZLIB_VERSION, sizeof(z_stream))

#endif

// functions to use
void PK3_CloseLibrary(void);
qboolean PK3_OpenLibrary(qboolean verbose);
qboolean PK3_Enabled(void);