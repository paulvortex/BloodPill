#include "cmdlib.h"

// raw types
typedef enum
{
	RAW_TYPE_UNKNOWN,
	RAW_TYPE_0,		// raw
	RAW_TYPE_1,		// multiobject alternated TIM
	RAW_TYPE_2,		// multiobject tile
	_
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

	// common
	rawswitch_t	doubleres;	// double the width & height
	qboolean	disableCLUT; // disable writing of clut, write indexes as grayscale instead
	qboolean	dontSwapBgr; // disable swapping of BGR->RGB, for images that are initially BGR	
}rawinfo_t;