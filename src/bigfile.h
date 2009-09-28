// thanks to XentaX (www.xentax.com) community for providing bigfile specs

#include "timfile.h"
#include "rawfile.h"

#define MAX_TIM_LAYERS

// filetypes
#define BIGFILE_NUM_FILETYPES 5
typedef enum
{
	BIGENTRY_UNKNOWN,		// unknown data
	BIGENTRY_TIM,			// 4 bit TIM texture
	BIGENTRY_RAW_ADPCM,		// RAW 4 bit ADPCM
	BIGENTRY_RIFF_WAVE,		// RIFF wave file
	BIGENTRY_RAW_IMAGE,		// a RAW image file
}bigentrytype_t;

// filetype extensions
static char *bigentryext[BIGFILE_NUM_FILETYPES] = 
{ 
	"dat", 
	"tim",
	"adpcm", 
	"wav",
	"raw"
};

// bigfile entry
typedef struct
{
	// base info
	unsigned int hash; // hashed name
	unsigned int size; // file size
	unsigned int offset; // file offset

	// loaded by tool
	char name[MAX_BLOODPATH];
	bigentrytype_t type;

	// for TIM
	int timlayers;
	unsigned int timtype[1 + MAX_TIM_MASKS];
	short timypos[1 + MAX_TIM_MASKS]; 
	short timxpos[1 + MAX_TIM_MASKS];

	// for VAG
	int adpcmrate;  

	// for RAW files (assigned by known-files-list)
	rawinfo_t *rawinfo;

	// only presented if loaded
	void *data; 
}
bigfileentry_t;

// bigfile header
typedef struct
{
		bigfileentry_t *entries;
		unsigned int	numentries;
}
bigfileheader_t;
