// thanks to XentaX (www.xentax.com) community for providing bigfile specs

#include "timfile.h"

#define MAX_TIM_LAYERS

// filetypes
#define BIGFILE_NUM_FILETYPES 4
typedef enum
{
	BIGENTRY_UNKNOWN,		// unknown data
	BIGENTRY_TIM,			// 4 bit TIM texture
	BIGENTRY_RAW_VAG,		// RAW 4 bit ADPCM
	BIGENTRY_RIFF_WAVE,		// RIFF wave file
}bigentrytype_t;

// filetype extensions
static char *bigentryext[BIGFILE_NUM_FILETYPES] = 
{ 
	"dat", 
	"tim",
	"vag", 
	"wav"
};

// bigfile entry
typedef struct
{
	// base info
	unsigned int hash; // hashed name
	unsigned int size; // file size
	unsigned int offset; // file offset

	// loaded by tool
	char name[16];
	bigentrytype_t type;

	// for TIM
	int timlayers;
	unsigned int timtype[1 + MAX_TIM_MASKS];
	short timypos[1 + MAX_TIM_MASKS]; 
	short timxpos[1 + MAX_TIM_MASKS];

	// for VAG
	int vagrate;  

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