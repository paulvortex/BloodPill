#include "bloodpill.h"
#include "timfile.h"
#include "cmdlib.h"
#include "mem.h"

void FreeTIM(tim_image_t *tim)
{
	if (!tim)
		return;
	if (tim->CLUT != NULL ) 
		qfree(tim->CLUT); 
	if (tim->pixels != NULL)
		qfree(tim->pixels); 
	qfree(tim); 
}

tim_image_t *TimError(tim_image_t *tim, char *error, ...)
{
	va_list argptr;

	va_start(argptr, error);
	tim->error = true;
	tim->errorstr = qmalloc(strlen(error)+1);
	vsprintf(tim->errorstr, error, argptr);
	va_end(argptr);

	return tim;
}

tim_image_t *TIM_LoadFromStream(FILE *f, int filesize)
{
	tim_image_t *tim;
	long nextobjlen;
	int rb, filepos = 0;

	tim = qmalloc(sizeof(tim_image_t));
	tim->error = false;
	tim->errorstr = NULL;
	tim->CLUT = NULL;
	tim->pixels = NULL;

	// 0x10 should be at beginning of standart TIM
	rb = fread(&tim->tag, 4, 1, f);
	if (rb < 1 || tim->tag != TIM_TAG)
		return TimError(tim, "funky tag %.8X", tim->tag);

	// second uint is type
	rb = fread(&tim->type, 4, 1, f);
	if (rb < 1 || (tim->type != TIM_4Bit && tim->type != TIM_8Bit && tim->type != TIM_16Bit && tim->type != TIM_24Bit))
		return TimError(tim, "unsupported type %.8X", tim->type);

	// third uint is size of CLUT/image data
	rb = fread(&nextobjlen, 4, 1, f);
	if (rb < 1 || nextobjlen <= 4)
		return TimError(tim, feof(f) ? "unexpected EOF at CLUT/image" : "unable to read CLUT/image");

	// load CLUT if presented
	if (tim->type == TIM_4Bit || tim->type == TIM_8Bit)
	{
		tim->CLUT = qmalloc(sizeof(tim_clutinfo_t));
		fread(tim->CLUT, nextobjlen-4, 1, f);
		fread(&nextobjlen, 4, 1, f);
	}		

	// read dimension info
	if (fread(&tim->dim, sizeof(tim_diminfo_t), 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at dimension info" : "unable to read dimension info");

	// read pixel data
	tim->pixelbytes = nextobjlen;
	tim->pixels = qmalloc(nextobjlen);
	rb = fread(tim->pixels, nextobjlen, 1, f);
	if (rb < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at pixel data" : "unable to read pixel data");

	return tim;
}
