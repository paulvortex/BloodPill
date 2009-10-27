// thanks to XentaX (www.xentax.com) community for providing bigfile specs
#include "timfile.h"
#include "rawfile.h"
#include "sprfile.h"

#define MAX_TIM_LAYERS

// filetypes
#define BIGFILE_NUM_FILETYPES 6
typedef enum
{
	BIGENTRY_UNKNOWN,		// unknown data
	BIGENTRY_TIM,			// 4 bit TIM texture
	BIGENTRY_RAW_ADPCM,		// RAW 4 bit ADPCM
	BIGENTRY_RIFF_WAVE,		// RIFF wave file
	BIGENTRY_RAW_IMAGE,		// a RAW image file
	BIGENTRY_VAG,			// PSX VAG
}bigentrytype_t;

// filetype extensions
static char *bigentryext[BIGFILE_NUM_FILETYPES] = 
{ 
	"dat",
	"tim",
	"adpcm",
	"wav",
	"raw",
	"vag"
};	

// filetype autopaths
static char *bigentryautopaths[BIGFILE_NUM_FILETYPES] = 
{ 
	"unknown/", 
	"tims/",
	"adpcm/", 
	"wave/",
	"raw/",
	"vag/"
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

// convert functions
void TGAfromTIM(FILE *bigf, bigfileentry_t *entry, char *outfile, qboolean bpp16to24);
void TGAfromRAW(rawblock_t *rawblock, rawinfo_t *rawinfo, char *outfile, qboolean rawnoalign, qboolean verbose);