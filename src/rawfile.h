#include "cmdlib.h"

// raw types
typedef enum
{
	RAW_TYPE_UNKNOWN,
	RAW_TYPE_0,		// raw
	RAW_TYPE_1,		// item card
	RAW_TYPE_2,		// multiobject tile
	RAW_TYPE_3,		// multiobject strangely compressed
	RAW_TYPE_4,
	RAW_TYPE_5,
	RAW_TYPE_6,
	RAW_TYPE_7,
	RAW_TYPE_8,
	RAW_TYPE_SPECIAL,
	NUM_RAW_TYPES
}rawtype_t;

// raw read return codes
typedef enum
{
	// > 0 = file end pos
	RAWX_ERROR_HEADER_NOT_VALID = -1,
	RAWX_ERROR_IMPLICIT_OBJECTS_COUNT = -2,
	RAWX_ERROR_BAD_COLORMAP = -3,
	RAWX_ERROR_WIDTH_OR_HEIGHT_NOT_VALID = -4,
	RAWX_ERROR_FILE_BIGGER_THAN_REQUIRED = -5,
	RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED = -6,
	RAWX_ERROR_BAD_OBJECT_HEADER = -7,
	RAWX_ERROR_BAD_OBJECT_OFFSET = -8,
	RAWX_ERROR_COMPRESSED_UNPACK_OVERFLOW = -9,
	RAWX_ERROR_COMPRESSED_READ_OVERFLOW = -10,
	RAWX_ERROR_BAD_OPTIONS = -11,
	RAWX_ERROR_NOT_INDENTIFIED = -12
}rawextractresult_t;

// raw boolean type
typedef enum 
{ 
	rauto, 
	rtrue, 
	rfalse 
} rawswitch_t;

// raw chunk
typedef struct rawchunk_s
{
	int offset;
	int width;
	int height;
	int size; // width*height
	int x;
	int y;
	byte flagbit; 

	byte *colormap;
	qboolean colormapExternal; // set externally, don't free
	byte *pixels;
	qboolean pixelsExternal; // set externally, don't free
}rawchunk_t;

#define MAX_RAW_CHUNKS	1024

// returned extracted raw block
typedef struct rawblock_s
{
	rawextractresult_t errorcode;
	// shared colormap
	byte *colormap;
	qboolean colormapExternal; // set externally, don't free
	// chunks data
	int chunks;
	rawchunk_t chunk[MAX_RAW_CHUNKS];
		
	// set when chunk is readed, but it's not a end of file
	qboolean notEOF;
}rawblock_t;

// raw information
typedef struct rawinfo_s
{
	rawtype_t type;		// type of RAW file

	// dynamic only
	qboolean usecompression;
	byte compressionpixels[4];

	// type 0
	int width;
	int height;
	int offset; // offset into file when actual data starts
	int bytes;	// how many bytes per color (1, 2 or 3)
	int colormapoffset; // - 1 - no colormal
	int colormapbytes; // 2 or 3

	// common
	int chunknum; // if set, only extract this chunk number
	rawswitch_t	doubleres;	// double the width & height
	qboolean	disableCLUT; // disable writing of clut, write indexes as grayscale instead
	qboolean	dontSwapBgr; // disable swapping of BGR->RGB, for images that are initially BGR	
}rawinfo_t;

// util functions
void FlushRawInfo(rawinfo_t *rawinfo);
rawinfo_t *NewRawInfo();
qboolean ReadRawInfo(char *line, rawinfo_t *rawinfo);
void WriteRawInfo(FILE *f, rawinfo_t *rawinfo);
rawswitch_t ParseRawSwitch(char *str);
char *UnparseRawSwitch(rawswitch_t rawswitch);
rawtype_t ParseRawType(char *str);
char *UnparseRawType(rawtype_t rawtype);
char *RawStringForResult(int rescode);
char *PathForRawType(rawtype_t rawtype);

// rawblock tools
void RawblockFlip(rawblock_t *rawblock);
rawblock_t *RawblockCrop(rawblock_t *rawblock, qboolean cropeachchunk, int margin);
rawblock_t *RawblockAlign(rawblock_t *rawblock, int margin);
rawblock_t *RawblockPerturbate(rawblock_t *rawblock, list_t *includelist);

// raw blocks
char *RawStringForResult(int rescode);
rawblock_t *EmptyRawBlock(int numchunks);
rawblock_t *RawErrorBlock(rawblock_t *block, rawextractresult_t errorcode);
void FreeRawBlock(rawblock_t *block);

void RawTGA(char *outfile, int width, int height, int bx, int by, int ax, int ay, const char *colormapdata, const char *pixeldata, int bpp, rawinfo_t *rawinfo);
void RawTGAColormap(char *outfile, const byte *colormapdata, byte bytes);
rawblock_t *RawExtract(byte *filedata, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, rawtype_t forcetype);