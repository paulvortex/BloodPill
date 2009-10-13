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
	NUM_RAW_TYPES
}rawtype_t;

// raw boolean type
typedef enum 
{ 
	rauto, 
	rtrue, 
	rfalse 
} rawswitch_t;

// raw information
typedef struct rawinfo_s
{
	rawtype_t type;		// type of RAW file

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

int RawExtract(char *basefilename, char *filedata, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, rawtype_t forcetype);