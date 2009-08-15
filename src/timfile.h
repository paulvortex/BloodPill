// thanks to Raul Sobon (Cheekyboy@2-hot.com) for this specs

// TIM magic numbers
#define	TIM_TAG		0x10
#define	TIM_4Bit	0x08		/* 4 bit, USE CLUT  */
#define	TIM_8Bit	0x09		/* 8 bit, USE CLUT  */
#define	TIM_16Bit	0x02		/* 16 bit NO CLUT   */
#define	TIM_24Bit	0x03		/* 24 bit NO CLUT   */

typedef struct 
{
	long	flags;
	short	ncols;
	short	npals;
	short	Clut16Bit[256];
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
	tim_clutinfo_t *CLUT;
	// pixel data, different 
	int pixelbytes; // sizeof of pixels
	unsigned char *pixels;
	// error data
	qboolean error;
	char *errorstr;
}tim_image_t;
