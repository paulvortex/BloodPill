// thanks to XentaX (www.xentax.com) community for providing bigfile specs
#include "timfile.h"
#include "rawfile.h"
#include "sprfile.h"
#include "vagfile.h"

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
	"raw",
	"tim",
	"adpcm",
	"wav",
	"rsp",
	"vag"
};	

// filetype autopaths
static char *bigentryautopaths[BIGFILE_NUM_FILETYPES] = 
{ 
	"unknown/", 
	"graphics/",
	"adpcm/", 
	"wave/",
	"sprite/",
	"vag/"
};

// hash names search table, picked from Rackot's BO1.csv


// bigfile entry
typedef struct
{
	// base info
	unsigned int hash; // hashed name
	unsigned int size; // file size
	unsigned int offset; // file offset
	unsigned int oldoffset; // old file offset (before recalculating)

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

// knowledge base
typedef struct
{
	unsigned int hash;

	int adpcmrate; // adpcm rate
	char path[MAX_BLOODPATH]; // a path to extract
	qboolean pathonly; // only define path, not filename
	bigentrytype_t type; // a type of entry
	rawinfo_t *rawinfo; // raw format info
}
bigkentry_t;

typedef struct
{
	int numentries;
	bigkentry_t *entries;
}
bigklist_t;

extern bigklist_t *bigklist;

// base functions
bigklist_t *BigfileLoadKList(char *filename, qboolean stopOnError);
unsigned int BigfileEntryHashFromString(char *string);
bigfileentry_t *BigfileGetEntry(bigfileheader_t *bigfile, unsigned int hash);
bigfileheader_t *ReadBigfileHeader(FILE *f, char *filename, qboolean loadfilecontents, qboolean hashnamesonly);
void BigfileSeekContents(FILE *f, byte *contents, bigfileentry_t *entry);
void BigfileScanFiletype(FILE *f, bigfileentry_t *entry, qboolean scanraw, rawtype_t forcerawtype, qboolean allow_auto_naming);
void BigfileScanFiletypes(FILE *f, bigfileheader_t *data, qboolean scanraw, list_t *ixlist, rawtype_t forcerawtype);
void BigFile_ExtractRawImage(int argc, char **argv, char *outfile, bigfileentry_t *entry, rawblock_t *rawblock, char *format);
void BigFile_ExtractSound(int argc, char **argv, char *outfile, bigfileentry_t *entry, char *infileformat, int defaultinputrate, char *format);
void BigFile_ExtractEntry(int argc, char **argv, FILE *bigfile, bigfileentry_t *entry, char *outfile);

// convert functions
void TGAfromTIM(FILE *bigf, bigfileentry_t *entry, char *outfile, qboolean bpp16to24);
void TGAfromRAW(rawblock_t *rawblock, rawinfo_t *rawinfo, char *outfile, qboolean rawnoalign, qboolean verbose, qboolean usesubpaths);