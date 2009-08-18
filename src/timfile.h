// thanks to Klarth (stevemonaco@hotmail.com) and  Raul Sobon (Cheekyboy@2-hot.com) for TIM spec's

#include "bloodpill.h"

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
	short yskip;
	short xskip;
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

	// no a part of format spec's
	// filled by loader
	int bpp;
	int pixelbytes; 
	int filelen;
	qboolean error;
	char *errorstr;
}tim_image_t;

// timfile.c
void FreeTIM(tim_image_t *tim);

tim_image_t *TIM_LoadFromStream(FILE *f, int filelen);

void TIM_WriteTarga(tim_image_t *tim, char *savefile, qboolean bpp16to24);