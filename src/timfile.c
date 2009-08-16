#include "bloodpill.h"
#include "timfile.h"
#include "cmdlib.h"
#include "mem.h"

tim_image_t *EmptyTIM(unsigned int type)
{
	tim_image_t *tim;

	tim = qmalloc(sizeof(tim_image_t));
	memset(tim, 0, sizeof(tim_image_t));
	tim->tag = TIM_TAG;
	tim->type = type;
	tim->error = false;
	tim->errorstr = NULL;
	tim->CLUT = NULL;
	tim->pixels = NULL;

	return tim;
}

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

tim_image_t *TIM_LoadFromStream(FILE *f, int filelen)
{
	tim_image_t *tim;
	long nextobjlen;
	int pixelbytes;
	int filepos = 0;

	tim = EmptyTIM(0);

	// 0x10 should be at beginning of standart TIM
	if (fread(&tim->tag, 4, 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at tag" : "unable to read tag");
	if (tim->tag != TIM_TAG)
		return TimError(tim, "funky tag %.8X", tim->tag);

	// second uint is type
	if (fread(&tim->type, 4, 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at type" : "unable to read type");

	// set bpp
	if (tim->type == TIM_4Bit)
		tim->bpp = 4;
	else if (tim->type == TIM_8Bit)
		tim->bpp = 8;
	else if (tim->type == TIM_16Bit)
		tim->bpp = 16;
	else if (tim->type == TIM_24Bit)
		tim->bpp = 24;
	else
		return TimError(tim, "unsupported type %.8X", tim->type);

	// third uint is size of CLUT/image data
	if (fread(&nextobjlen, 4, 1, f) < 1 || nextobjlen <= 4)
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

	// get actual width/height
	if (tim->type == TIM_4Bit)
	{
		tim->dim.xsize = tim->dim.xsize * 4;
		pixelbytes = (int)(tim->dim.xsize*tim->dim.ysize/2);
	}
	else if (tim->type == TIM_8Bit)
	{
		tim->dim.xsize = tim->dim.xsize * 2;
		pixelbytes = tim->dim.xsize*tim->dim.ysize;
	}
	else if (tim->type == TIM_16Bit)
		pixelbytes = tim->dim.xsize*tim->dim.ysize*2;
	else if (tim->type == TIM_24Bit)
	{
		tim->dim.xsize = (short)(tim->dim.xsize / 1.5);
		pixelbytes = tim->dim.xsize*tim->dim.ysize*3;
	}

	// read pixel data
	tim->filelen = filelen;
	tim->pixelbytes = (pixelbytes < nextobjlen) ? pixelbytes : nextobjlen;
	tim->pixels = qmalloc(tim->pixelbytes);
	if (fread(tim->pixels, tim->pixelbytes, 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at pixel data (%i bytes)" : "unable to read pixel data (%i bytes)", tim->pixelbytes);

	return tim;
}

void TIM_WriteToStream(tim_image_t *tim, FILE *f)
{
	tim_diminfo_t diminfo;
	unsigned int temp;

	// write header
	temp = TIM_TAG;
	fwrite(&temp, 4, 1, f);
	fwrite(&tim->type, 4, 1, f);

	// CLUT & size of pixelbytes
	if (tim->type == TIM_4Bit || tim->type == TIM_8Bit)
	{
		temp = (unsigned int)(sizeof(tim_clutinfo_t) + 4);
		fwrite(&temp, 4, 1, f);
		fwrite(tim->CLUT, sizeof(tim_clutinfo_t), 1, f);
	}
	fwrite(&tim->pixelbytes, 4, 1, f);

	// write dimensions
	memcpy(&diminfo, &tim->dim, sizeof(tim_diminfo_t));
	if (tim->type == TIM_4Bit)
		diminfo.xsize = (short)(diminfo.xsize / 4);
	else if (tim->type == TIM_8Bit)
		diminfo.xsize = (short)(diminfo.xsize / 2);
	else if (tim->type == TIM_24Bit)
		tim->dim.xsize = (short)(tim->dim.xsize * 1.5);
	fwrite(&diminfo, sizeof(tim_diminfo_t), 1, f);

	// write pixels
	fwrite(tim->pixels, tim->pixelbytes, 1, f);
}

tim_image_t *TIM_LoadFromTarga(FILE *f, unsigned int type)
{
	tim_image_t *tim;
	unsigned char targaheader[18];
	unsigned char *colormapdata, *pixeldata, *out;
	const unsigned char *in, *end;
	int width, height, colormaplen, y;

	tim = EmptyTIM(type);

	// read header
	if (fread(&targaheader, 18, 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at targa header" : "unable to read targa header");
	width = targaheader[12] + targaheader[13]*256;
	height = targaheader[14] + targaheader[15]*256;
	colormaplen = targaheader[5] + targaheader[6]*256;

	// autopick type
	if (!type)
	{
		if (targaheader[1] == 1)
			type = TIM_8Bit;
		else if (targaheader[16] == 16)
			type = TIM_16Bit;
		else if (targaheader[16] == 24)
			type = TIM_24Bit;
		else
			type = TIM_24Bit;
	}

	// verify header
	switch(type)
	{
		case TIM_4Bit:
			return TimError(tim, "4-bit TIM loading not supported");
			break;
		case TIM_8Bit:
			// check header
			if (targaheader[1] != 1)
				return TimError(tim, "8-bit TIM require colormapped TGA)");
			if (targaheader[2] != 1)
				return TimError(tim, "8-bit TIM require uncompressed TGA (type 1)");
			if (colormaplen > 256)
				return TimError(tim, "8-bit TIM require 256 colors TGA");
			if (targaheader[7] != 16 && targaheader[7] != 24)
				return TimError(tim, "8-bit TIM requires 16 or 24 bit colormapped TGA");
			if (targaheader[16] != 8)
				return TimError(tim, "8-bit TIM only supports 8-bit TGA");
	
			// load colormap data
			colormapdata = qmalloc(colormaplen * ((targaheader[7] == 16) ? 2 : 3));
			if (fread(colormapdata, colormaplen * ((targaheader[7] == 16) ? 2 : 3), 1, f) < 1)
				return TimError(tim, feof(f) ? "unexpected EOF at TGA colormap" : "unable to read TGA colormap");

			// load pixel data
			pixeldata = qmalloc(width * height);
			if (fread(pixeldata, width * height, 1, f) < 1)
				return TimError(tim, feof(f) ? "unexpected EOF at TGA pixeldata" : "unable to read TGA pixeldata");

			// fill TIM header
			tim->type = TIM_8Bit;
			tim->dim.xskip = targaheader[8] + targaheader[9]*256;
			tim->dim.yskip = targaheader[10] + targaheader[11]*256;
			tim->dim.xsize = width;
			tim->dim.ysize = height;
			tim->CLUT = qmalloc(sizeof(tim_clutinfo_t));
			memset(tim->CLUT, 0, sizeof(tim_clutinfo_t));
			tim->pixelbytes = tim->dim.xsize*tim->dim.ysize;
			tim->pixels = qmalloc(tim->pixelbytes);
			tim->bpp = 8;
			tim->filelen = 32 + sizeof(tim_clutinfo_t) + 4 + tim->pixelbytes - 12;

			// fill CLUT, write 15-bit colormap, swap bgr->rgb
			out = tim->CLUT->data;
			for (y = 0;y < colormaplen;y++)
			{
				if (targaheader[7] == 24)
				{
					in = colormapdata + y*3;
					*out++ = (int)(in[2]/8) + (((int)(in[1]/8) << 5) & 0xE0);
					*out++ = (((int)(in[1]/8) >> 3) & 0x03) + ((int)(in[0]/8) << 2);
				}
				else
				{
					in = colormapdata + y*2;
					*out++ = (in[0] & 0xE0) + ((in[1] & 0x7C) >> 2); 
					*out++ = (in[1] & 0x03) + ((in[0] & 0x1F) << 2);
				}
			}

			// fill pixels, flip upside down
			out = tim->pixels;
			for (y = height - 1;y >= 0;y--)
			{
				in = pixeldata + y * width;
				end = in + width;
				for (;in < end; in++)
					*out++ = in[0];
			}
			break;
		case TIM_16Bit:
			break;
		case TIM_24Bit:
			break;
		default:
			return TimError(tim, "unknown TIM type %.8X requested", type);
	}

	return tim;
}

void TIM_WriteTarga(tim_image_t *tim, char *savefile, qboolean bpp16to24)
{
	unsigned char *buffer, *out;
	const unsigned char *in, *end;
	FILE *f;
	int y;

	if (tim->error)
		return; // don't write erroneous TIM

	// write header
	f = SafeOpen(savefile, "wb");
	switch(tim->type)
	{
		case TIM_4Bit:
			break;
		case TIM_8Bit:
			buffer = qmalloc(tim->dim.xsize*tim->dim.ysize + (bpp16to24 ? 768 : 512) + 18);
			memset(buffer, 0, 18);
			buffer[1] = 1; // colormapped
			buffer[2] = 1; // uncompressed, colormapped
			buffer[5] = (256 >> 0) & 0xFF;
			buffer[6] = (256 >> 8) & 0xFF;
			buffer[7] = (bpp16to24 ? 24 : 16); // colormap BPP
			buffer[8] = (tim->dim.xskip >> 0) & 0xFF;
			buffer[9] = (tim->dim.xskip >> 8) & 0xFF;
			buffer[10] = (tim->dim.yskip >> 0) & 0xFF;
			buffer[11] = (tim->dim.yskip >> 8) & 0xFF;
			buffer[12] = (tim->dim.xsize >> 0) & 0xFF;
			buffer[13] = (tim->dim.xsize >> 8) & 0xFF;
			buffer[14] = (tim->dim.ysize >> 0) & 0xFF;
			buffer[15] = (tim->dim.ysize >> 8) & 0xFF;
			buffer[16] = 8;
			// write 16 or 24-bit colormap from 15-bit CLUT, swap bgr->rgb
			out = buffer + 18;
			for (y = 0;y < 256;y++)
			{
				in = tim->CLUT->data + y*2;
				if (bpp16to24)
				{
					*out++ = ((in[1] & 0x7C) >> 2) * 8;
					*out++ = (((in[0] & 0xE0) >> 5) + ((in[1] & 0x3) << 3)) * 8;
					*out++ = (in[0] & 0x1F) * 8; 
				}
				else
				{
					*out++ = (in[0] & 0xE0) + ((in[1] & 0x7C) >> 2); 
					*out++ = (in[1] & 0x03) + ((in[0] & 0x1F) << 2);
				}
			}
			// flip upside down, write
			out = buffer + (bpp16to24 ? 768 : 512) + 18;
			for (y = tim->dim.ysize - 1;y >= 0;y--)
			{
				in = tim->pixels + y * tim->dim.xsize;
				end = in + tim->dim.xsize;
				for (;in < end; in++)
					*out++ = in[0];
			}
			// write file
			fwrite(buffer, tim->dim.xsize*tim->dim.ysize + (bpp16to24 ? 768 : 512) + 18, 1, f);
			break;
		case TIM_16Bit:
			buffer = qmalloc(tim->dim.xsize*tim->dim.ysize*(bpp16to24 ? 3 : 2) + 18);
			memset(buffer, 0, 18);
			buffer[2] = 2; // uncompressed
			buffer[8] = (tim->dim.xskip >> 0) & 0xFF;
			buffer[9] = (tim->dim.xskip >> 8) & 0xFF;
			buffer[10] = (tim->dim.yskip >> 0) & 0xFF;
			buffer[11] = (tim->dim.yskip >> 8) & 0xFF;
			buffer[12] = (tim->dim.xsize >> 0) & 0xFF;
			buffer[13] = (tim->dim.xsize >> 8) & 0xFF;
			buffer[14] = (tim->dim.ysize >> 0) & 0xFF;
			buffer[15] = (tim->dim.ysize >> 8) & 0xFF;
			buffer[16] = (bpp16to24 ? 24 : 16);
			// swap bgr->rgb, flip upside down
			out = buffer + 18;
			for (y = tim->dim.ysize - 1;y >= 0;y--)
			{
				in = tim->pixels + y * tim->dim.xsize * 2;
				end = in + tim->dim.xsize * 2;
				for (;in < end;in += 2)
				{
					if (bpp16to24)
					{
						*out++ = ((in[1] & 0x7C) >> 2) * 8;
						*out++ = (((in[0] & 0xE0) >> 5) + ((in[1] & 0x3) << 3)) * 8;
						*out++ = (in[0] & 0x1F) * 8; 
					}
					else
					{
						*out++ = (in[0] & 0xE0) + ((in[1] & 0x7C) >> 2); 
						*out++ = (in[1] & 0x03) + ((in[0] & 0x1F) << 2);
					}
				}
			}
			fwrite(buffer, tim->dim.xsize*tim->dim.ysize*(bpp16to24 ? 3 : 2) + 18, 1, f);
			break;
		case TIM_24Bit:
			buffer = qmalloc(tim->dim.xsize*tim->dim.ysize*3 + 18);
			memset(buffer, 0, 18);
			buffer[2] = 2; // uncompressed
			buffer[8] = (tim->dim.xskip >> 0) & 0xFF;
			buffer[9] = (tim->dim.xskip >> 8) & 0xFF;
			buffer[10] = (tim->dim.yskip >> 0) & 0xFF;
			buffer[11] = (tim->dim.yskip >> 8) & 0xFF;
			buffer[12] = (tim->dim.xsize >> 0) & 0xFF;
			buffer[13] = (tim->dim.xsize >> 8) & 0xFF;
			buffer[14] = (tim->dim.ysize >> 0) & 0xFF;
			buffer[15] = (tim->dim.ysize >> 8) & 0xFF;
			buffer[16] = 24;
			// swap bgr->rgb, flip upside down
			out = buffer + 18;
			for (y = tim->dim.ysize - 1;y >= 0;y--)
			{
				in = tim->pixels + y * tim->dim.xsize * 3;
				end = in + tim->dim.xsize * 3;
				for (;in < end;in += 3)
				{
					*out++ = in[2];
					*out++ = in[1];
					*out++ = in[0];
				}
			}
			fwrite(buffer, tim->dim.xsize*tim->dim.ysize*3 + 18, 1, f);
			break;
		default:
			break;
	}
	fclose(f);
}

int Tim2Targa_Main(int argc, char **argv)
{
	int i = 1, filelen;
	char filename[MAX_BLOODPATH], ext[5], outfile[MAX_BLOODPATH], *c;
	tim_image_t *tim;
	qboolean bpp16to24;
	FILE *f; 

	printf("=== Tim2Tga ===\n");
	if (i < 1)
		Error("not enough parms");

	// get inner file
	strcpy(filename, argv[i]);
	ExtractFileExtension(filename, ext);
	i++;

	// get out file
	strcpy(outfile, filename);
	ReplaceExtension(outfile, ext, "tga", "tga");
	if (i < argc)
	{
		c = argv[i];
		if (c[0] != '-')
			strcpy(outfile, c);
	}

	// parse cmdline
	bpp16to24 = false;
	for (i = i; i < argc; i++)
	{
		if (!strcmp(argv[i], "-16to24"))
		{
			printf("Targa compatibility mode enabled (converting 16-bit to 24-bit)\n");
			bpp16to24 = true;
		}
	}

	// open source file, try load it
	f = SafeOpen(filename, "rb");
	filelen = Q_filelength(f);
	tim = TIM_LoadFromStream(f, filelen);
	fclose(f);
	if (tim->error)
		Error("Error loading %s: %s", filename, tim->errorstr);

	// print TIM stats
	printf("%s loaded\n", filename);
	printf("        type: %i-bit TIM\n", tim->bpp);
	printf(" orientation: %ix%i\n", tim->dim.xskip, tim->dim.yskip);
	printf("        size: %ix%i\n", tim->dim.xsize, tim->dim.ysize);
	printf(" pixel bytes: %i\n", tim->pixelbytes);
	printf(" file length: %i\n", tim->filelen);

	printf("writing %s\n", outfile);
	TIM_WriteTarga(tim, outfile, bpp16to24);
	printf("done.");

	return 0;
}

int Targa2Tim_Main(int argc, char **argv)
{
	char filename[MAX_BLOODPATH], ext[5], outfile[MAX_BLOODPATH], *c;
	unsigned int type;
	tim_image_t *tim;
	int i = 1;
	FILE *f;

	printf("=== Tga2Tim ===\n");
	if (i < 1)
		Error("not enough parms");

	// get inner file
	strcpy(filename, argv[i]);
	ExtractFileExtension(filename, ext);
	i++;

	// get out file
	strcpy(outfile, filename);
	ReplaceExtension(outfile, ext, "tim", "tim");
	if (i < argc)
	{
		c = argv[i];
		if (c[0] != '-')
			strcpy(outfile, c);
	}

	// parse cmdline
	type = 0;
	for (i = i; i < argc; i++)
	{
		if (!strcmp(argv[i], "-type"))
		{
			i++;
			if (i < argc)
			{
				if (!strcmp(argv[i], "8"))
					type = TIM_8Bit;
				else if (!strcmp(argv[i], "16"))
					type = TIM_16Bit;
				else if (!strcmp(argv[i], "24"))
					type = TIM_24Bit;
				else
					Error("parse commandline: bad type %i", argv[i]);
			}
		}
	}

	// open source file, try load it
	f = SafeOpen(filename, "rb");
	tim = TIM_LoadFromTarga(f, type);
	fclose(f);
	if (tim->error)
		Error("Error loading %s: %s", filename, tim->errorstr);

	// print TIM stats
	printf("%s loaded\n", filename);
	printf("        type: %i-bit TIM\n", tim->bpp);
	printf(" orientation: %ix%i\n", tim->dim.xskip, tim->dim.yskip);
	printf("        size: %ix%i\n", tim->dim.xsize, tim->dim.ysize);
	printf(" pixel bytes: %i\n", tim->pixelbytes);
	printf(" file length: %i\n", tim->filelen);

	printf("writing %s\n", outfile);
	f = SafeOpen(outfile, "wb");
	TIM_WriteToStream(tim, f);
	fclose(f);
	printf("done.");

	return 0;
}