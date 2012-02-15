// thanks to Klarth (stevemonaco@hotmail.com) and  Raul Sobon (Cheekyboy@2-hot.com) for TIM spec's

#include "bloodpill.h"

// max allowed sub-tims stored in single .TIM
#define MAX_TIM_MASKS	4

// TIM magic numbers
#define	TIM_TAG		0x00000010
#define	TIM_4Bit	0x08		/* 4 bit, USE CLUT  */
#define	TIM_8Bit	0x09		/* 8 bit, USE CLUT  */
#define	TIM_16Bit	0x02		/* 16 bit NO CLUT   */
#define	TIM_24Bit	0x03		/* 24 bit NO CLUT   */

typedef struct 
{
	long	flags;
	short	columns;
	short	palettes;
	byte	data[512];
}tim_clutinfo_t;

typedef struct 
{
	short ypos;
	short xpos;
	short xsize;
	short ysize;
}tim_diminfo_t;

typedef struct
{
	unsigned int tag;
	unsigned int type;
	tim_diminfo_t dim;

	// CLUT
	// todo: handle multiple CLUT's?
	tim_clutinfo_t *CLUT;
	unsigned char *pixels;
	unsigned char *pixelmask; //  16-bit TIM's only
	char *maskfile;

	// no a part of format spec's
	// filled by loader
	int bpp;
	int pixelbytes; 
	int filelen;
	bool error;
	char *errorstr;

}tim_image_t;

typedef struct
{
	tim_image_t *image;
	int masknum;
	tim_image_t *masks[MAX_TIM_MASKS];
}tim_file_t;

// timfile.c
void FreeTIM(tim_image_t *tim);

tim_image_t *TIM_LoadFromBuffer(byte *buf, int buflen);
tim_image_t *TIM_LoadFromStream(FILE *f);

void TIM_WriteToStream(tim_image_t *tim, FILE *f);

tim_image_t *TIM_LoadFromTargaStream(FILE *f, unsigned int type);

tim_image_t *TIM_LoadFromTarga(char *filename, unsigned int type);

void TIM_WriteTarga(tim_image_t *tim, char *savefile, bool bpp16to24);

void TIM_WriteTargaGrayscale(byte *data, short width, short height, char *savefile);


