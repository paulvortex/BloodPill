// filetypes
#define BIGFILE_NUM_FILETYPES		6
typedef enum
{
	BIGENTRY_UNKNOWN,		// unknown data
	BIGENTRY_TIM,			// TIM texture
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
	// used by tool
	bigentrytype_t type;
	char name[16];
	int samplingrate; // for VAG 
}
bigfileentry_t;

// bigfile header
typedef struct
{
		bigfileentry_t *entries;
		unsigned int	numentries;
}
bigfileheader_t;