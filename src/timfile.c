////////////////////////////////////////////////////////////////
//
// TIM files loader/writer
// coded by Pavel [VorteX] Timofeyev and placed to public domain
// thanks to Klarth (stevemonaco@hotmail.com) and  Raul Sobon (Cheekyboy@2-hot.com) for TIM spec's
//
////////////////////////////////

#include "bloodpill.h"
#include "timfile.h"
#include "cmdlib.h"
#include "mem.h"

/*
==========================================================================================

  General loader/writer

==========================================================================================
*/

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
	tim->pixelmask = NULL;
	tim->maskfile = NULL;

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
	if (tim->pixelmask != NULL) 
		qfree(tim->pixelmask); 
	if (tim->maskfile != NULL)
		qfree(tim->maskfile);
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

tim_image_t *TIM_LoadFromStream(FILE *f)
{
	tim_image_t *tim;
	long nextobjlen;
	unsigned char *out;
	int filepos = 0, y;

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
		tim->pixelbytes = (int)(tim->dim.xsize*tim->dim.ysize/2);
		tim->filelen = 20 + sizeof(tim_clutinfo_t) + 4 + tim->pixelbytes;
	}
	else if (tim->type == TIM_8Bit)
	{
		tim->dim.xsize = tim->dim.xsize * 2;
		tim->pixelbytes = tim->dim.xsize*tim->dim.ysize;
		tim->filelen = 20 + sizeof(tim_clutinfo_t) + 4 + tim->pixelbytes;
	}
	else if (tim->type == TIM_16Bit)
	{
		tim->pixelbytes = tim->dim.xsize*tim->dim.ysize*2;
		tim->filelen = 20 + tim->pixelbytes;
	}
	else if (tim->type == TIM_24Bit)
	{
		tim->dim.xsize = (short)(tim->dim.xsize / 1.5);
		tim->pixelbytes = tim->dim.xsize*tim->dim.ysize*3;
		tim->filelen = 20 + tim->pixelbytes;
	}

	// read pixels
	tim->pixels = qmalloc(tim->pixelbytes);
	if (fread(tim->pixels, tim->pixelbytes, 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at pixel data (%i bytes)" : "unable to read pixel data (%i bytes)", tim->pixelbytes);

	// extract pixel mask for 16-bit TIM
	if (tim->type == TIM_16Bit)
	{
		tim->pixelmask = qmalloc(tim->dim.xsize*tim->dim.ysize);
		out = tim->pixelmask;
		for (y = 0; y < tim->dim.xsize*tim->dim.ysize; y++)
		{
			if ((tim->pixels[y*2 + 1]) & 0x80)
			{
				tim->pixels[y*2 + 1] -= 0x80;
				*out++ = 255;
			}
			else
				*out++ = 0;
		}
	}
	return tim;
}

void TIM_WriteToStream(tim_image_t *tim, FILE *f)
{
	tim_diminfo_t diminfo;
	unsigned int temp;
	unsigned char *data;
	int i;

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
		diminfo.xsize = (short)(diminfo.xsize * 1.5);

	fwrite(&diminfo, sizeof(tim_diminfo_t), 1, f);

	// write pixels
	if (tim->type == TIM_16Bit)
	{
		// interleave pixelmask into tim
		data = qmalloc(tim->pixelbytes);
		if (tim->pixelmask != NULL)
		{
			for (i = 0; i < tim->pixelbytes; i += 2)
			{
				data[i] = tim->pixels[i];
				data[i + 1] = (tim->pixels[i+1] & 0x7F);
				if (tim->pixelmask[i / 2])
					data[i + 1] += 0x80;
			}
		}
		// interleave with default white pixelmask
		else
		{
			for (i = 0; i < tim->pixelbytes; i += 2)
			{
				data[i] = tim->pixels[i];
				data[i + 1] = (tim->pixels[i+1] & 0x7F) + 0x80;
			}
		}
		fwrite(data, tim->pixelbytes, 1, f);
		qfree(data);
	}
	else
		fwrite(tim->pixels, tim->pixelbytes, 1, f);
}

/*
==========================================================================================

  TGA CONVERTER

==========================================================================================
*/

tim_image_t *TIM_LoadPixelmaskFromTargaStream(FILE *f, tim_image_t *tim)
{
	unsigned char targaheader[18];
	unsigned char *pixeldata, *out;
	const unsigned char *in, *end;
	int width, height, colormaplen, y;

	if (tim->type != TIM_16Bit)
		return TimError(tim, "maskfiles only matter for 16-bit TIM");

	// read header
	if (fread(&targaheader, 18, 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at targa header" : "unable to read targa header");
	width = targaheader[12] + targaheader[13]*256;
	height = targaheader[14] + targaheader[15]*256;
	colormaplen = (targaheader[1] == 1) ? targaheader[5] + targaheader[6]*256 : 0;

	if (colormaplen)
		return TimError(tim, "colormapped TGA not supported for maskfiles");
	if (targaheader[2] != 2 || targaheader[16] != 8)
		return TimError(tim, "maskfiles only supportuncompressed 8-bit TGA");
	if (tim->dim.xsize != width)
		return TimError(tim, "image width do not match");
	if (tim->dim.ysize != height)
		return TimError(tim, "image height do not match");

	// load pixel data
	pixeldata = qmalloc(width * height);
	if (fread(pixeldata, width * height, 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at maskfile pixeldata" : "unable to read TGA pixeldata");

	tim->pixelmask = qmalloc(width * height);
	// fill pixels, flip upside down
	out = tim->pixelmask;
	for (y = height - 1;y >= 0;y--)
	{
		in = pixeldata + y * width;
		end = in + width;
		for (;in < end; in++)
			*out++ = (in[0]) ? 1 : 0;
	}
	qfree(pixeldata);

	return tim;
}

tim_image_t *TIM_LoadFromTargaStream(FILE *f, unsigned int type)
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
	colormaplen = (targaheader[1] == 1) ? targaheader[5] + targaheader[6]*256 : 0;

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
				return TimError(tim, "8-bit TIM require colormapped TGA");
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
			tim->dim.xpos = targaheader[8] + targaheader[9]*256;
			tim->dim.ypos = targaheader[10] + targaheader[11]*256;
			tim->dim.xsize = width;
			tim->dim.ysize = height;
			tim->CLUT = qmalloc(sizeof(tim_clutinfo_t));
			memset(tim->CLUT, 0, sizeof(tim_clutinfo_t));
			tim->pixelbytes = tim->dim.xsize*tim->dim.ysize;
			tim->pixels = qmalloc(tim->pixelbytes);
			tim->bpp = 8;
			tim->filelen = 20 + sizeof(tim_clutinfo_t) + 4 + tim->pixelbytes;

			// fill CLUT, write 15-bit colormap, swap bgr->rgb
			out = tim->CLUT->data;
			if (targaheader[7] == 24)
			{
				for (y = 0;y < colormaplen;y++)
				{
					in = colormapdata + y*3;
					*out++ = (int)(in[2]/8) + (((int)(in[1]/8) << 5) & 0xE0);
					*out++ = (((int)(in[1]/8) >> 3) & 0x03) + ((int)(in[0]/8) << 2);
				}
			}
			else
			{
				for (y = 0;y < colormaplen;y++)
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

			qfree(colormapdata);
			qfree(pixeldata);
			break;
		case TIM_16Bit:
			// check header
			if (colormaplen)
				return TimError(tim, "16-bit TIM require unmapped TGA");
			if (targaheader[2] != 2)
				return TimError(tim, "16-bit TIM require uncompressed RGB TGA");
			if (targaheader[16] != 16 && targaheader[16] != 24)
				return TimError(tim, "16-bit TIM require 16 or 24-bit TGA");

			// load pixel data
			pixeldata = qmalloc(width * height * ((targaheader[16] == 16) ? 2 : 3));
			if (fread(pixeldata, width * height * ((targaheader[16] == 16) ? 2 : 3), 1, f) < 1)
				return TimError(tim, feof(f) ? "unexpected EOF at TGA pixeldata" : "unable to read TGA pixeldata");

			// fill TIM header
			tim->type = TIM_16Bit;
			tim->dim.xpos = targaheader[8] + targaheader[9]*256;
			tim->dim.ypos = targaheader[10] + targaheader[11]*256;
			tim->dim.xsize = width;
			tim->dim.ysize = height;
			tim->CLUT = NULL;
			tim->pixelbytes = tim->dim.xsize*tim->dim.ysize*2;
			tim->pixels = qmalloc(tim->pixelbytes);
			tim->bpp = 16;
			tim->filelen = 20 + tim->pixelbytes;

			// fill pixels, flip upside down, swap bgr->rgb, convert 24-bit to 16 if needed
			out = tim->pixels;
			if (targaheader[16] == 24)
			{
				for (y = height - 1;y >= 0;y--)
				{
					in = pixeldata + y * width * 3;
					end = in + width * 3;
					for (;in < end; in += 3)
					{
						*out++ = (int)(in[2]/8) + (((int)(in[1]/8) << 5) & 0xE0);
						*out++ = (((int)(in[1]/8) >> 3) & 0x03) + ((int)(in[0]/8) << 2);
					}
				}
			}
			else
			{
				for (y = height - 1;y >= 0;y--)
				{
					in = pixeldata + y * width * 2;
					end = in + width * 2;
					for (;in < end; in += 2)
					{
						*out++ = (in[0] & 0xE0) + ((in[1] & 0x7C) >> 2); 
						*out++ = (in[1] & 0x03) + ((in[0] & 0x1F) << 2);
					}
				}
			}

			qfree(pixeldata);
			break;
		// VorteX: pretty same as 24 bit
		case TIM_24Bit:
			// check header
			if (colormaplen)
				return TimError(tim, "24-bit TIM require unmapped TGA");
			if (targaheader[2] != 2)
				return TimError(tim, "24-bit TIM require uncompressed RGB TGA");
			if (targaheader[16] != 16 && targaheader[16] != 24)
				return TimError(tim, "24-bit TIM require 16 or 24-bit TGA");

			// load pixel data
			pixeldata = qmalloc(width * height * ((targaheader[16] == 16) ? 2 : 3));
			if (fread(pixeldata, width * height * ((targaheader[16] == 16) ? 2 : 3), 1, f) < 1)
				return TimError(tim, feof(f) ? "unexpected EOF at TGA pixeldata" : "unable to read TGA pixeldata");

			// fill TIM header
			tim->type = TIM_24Bit;
			tim->dim.xpos = targaheader[8] + targaheader[9]*256;
			tim->dim.ypos = targaheader[10] + targaheader[11]*256;
			tim->dim.xsize = width;
			tim->dim.ysize = height;
			tim->CLUT = NULL;
			tim->pixelbytes = tim->dim.xsize*tim->dim.ysize*3;
			tim->pixels = qmalloc(tim->pixelbytes);
			tim->bpp = 24;
			tim->filelen = 20 + tim->pixelbytes;

			// fill pixels, flip upside down, swap bgr->rgb, convert 24-bit to 16 if needed
			out = tim->pixels;
			if (targaheader[16] == 24)
			{
				for (y = height - 1;y >= 0;y--)
				{
					in = pixeldata + y * width * 3;
					end = in + width * 3;
					for (;in < end; in += 3)
					{
						*out++ = in[2];
						*out++ = in[1];
						*out++ = in[0];	
					}
				}
			}
			else
			{
				for (y = height - 1;y >= 0;y--)
				{
					in = pixeldata + y * width * 2;
					end = in + width * 2;
					for (;in < end; in += 2)
					{
						*out++ = ((in[1] & 0x7C) >> 2) * 8;
						*out++ = (((in[0] & 0xE0) >> 5) + ((in[1] & 0x3) << 3)) * 8;
						*out++ = (in[0] & 0x1F) * 8; 
					}
				}
			}
			qfree(pixeldata);
			break;
		default:
			return TimError(tim, "unknown TIM type %.8X requested", type);
	}

	return tim;
}

tim_image_t *TIM_LoadFromTarga(char *filename, unsigned int type)
{
	tim_image_t *tim;
	char basename[MAX_BLOODPATH], ext[15], maskfile[MAX_BLOODPATH];
	FILE *f;

	// open base TIM
	f = SafeOpen(filename, "rb");
	tim = TIM_LoadFromTargaStream(f, type);
	if (tim->error)
		Error("Error loading %s: %s", filename, tim->errorstr);
	fclose(f);

	// try open mask file
	if (tim->type != TIM_16Bit)
		return tim;
	StripFileExtension(filename, basename);
	ExtractFileExtension(filename, ext);
	sprintf(maskfile, "%s_mask.%s", basename, ext);

	f = fopen(maskfile, "rb");
	if (!f)
		Error("Error loading %s: mask file %s not found", filename, maskfile);

	tim->maskfile = qmalloc(MAX_BLOODPATH);
	strcpy(tim->maskfile, maskfile);
	TIM_LoadPixelmaskFromTargaStream(f, tim);
	if (tim->error)
		Error("Error loading maskfile %s: %s", maskfile, tim->errorstr);
	fclose(f);

	return tim;
}

void TIM_WriteTargaGrayscale(char *data, short width, short height, char *savefile)
{
	unsigned char *buffer, *out;
	const unsigned char *in, *end;
	int i;
	FILE *f;

	// create grayscale targa header
	buffer = qmalloc(width*height + 18);
	memset(buffer, 0, 18);
	buffer[2] = 2;
	buffer[12] = (width >> 0) & 0xFF;
	buffer[13] = (width >> 8) & 0xFF;
	buffer[14] = (height >> 0) & 0xFF;
	buffer[15] = (height >> 8) & 0xFF;
	buffer[16] = 8;

	// flip upside down, write grayscale
	out = buffer + 18;
	for (i = height - 1;i >= 0;i--)
	{
		in = data + i * width;
		end = in + width;
		for (;in < end; in++)
			*out++ = in[0];
	}

	// write file
	f = SafeOpen(savefile, "wb");
	fwrite(buffer, width*height + 18, 1, f);
	fclose(f);
	qfree(buffer);
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
			buffer[8] = (tim->dim.xpos >> 0) & 0xFF;
			buffer[9] = (tim->dim.xpos >> 8) & 0xFF;
			buffer[10] = (tim->dim.ypos >> 0) & 0xFF;
			buffer[11] = (tim->dim.ypos >> 8) & 0xFF;
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
			buffer[8] = (tim->dim.xpos >> 0) & 0xFF;
			buffer[9] = (tim->dim.xpos >> 8) & 0xFF;
			buffer[10] = (tim->dim.ypos >> 0) & 0xFF;
			buffer[11] = (tim->dim.ypos >> 8) & 0xFF;
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
			buffer[8] = (tim->dim.xpos >> 0) & 0xFF;
			buffer[9] = (tim->dim.xpos >> 8) & 0xFF;
			buffer[10] = (tim->dim.ypos >> 0) & 0xFF;
			buffer[11] = (tim->dim.ypos >> 8) & 0xFF;
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

/*
==========================================================================================

  UTIL MAIN

==========================================================================================
*/

void TimEmitStats(tim_image_t *tim)
{
	printf("        type: %i-bit TIM\n", tim->bpp);
	printf(" orientation: %ix%i\n", tim->dim.xpos, tim->dim.ypos);
	printf("        size: %ix%i\n", tim->dim.xsize, tim->dim.ysize);
	if (tim->CLUT == NULL)
		printf("        CLUT: no\n");
	else
	{
		printf("        CLUT: yes\n");
		printf("   CLUT flag: %i\n", tim->CLUT->flags);
		printf("   CLUT cols: %i\n", tim->CLUT->columns);
		printf("   CLUT pals: %i\n", tim->CLUT->palettes);
	}
	printf(" pixel bytes: %i\n", tim->pixelbytes);
	printf(" file length: %i\n", tim->filelen);
}

int Tim2Targa_Main(int argc, char **argv)
{
	int i = 1;
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
	tim = TIM_LoadFromStream(f);
	fclose(f);
	if (tim->error)
		Error("Error loading %s: %s", filename, tim->errorstr);

	// print TIM stats
	printf("%s loaded\n", filename);
	TimEmitStats(tim);

	// write basefile
	printf("writing %s\n", outfile);
	TIM_WriteTarga(tim, outfile, bpp16to24);

	// write maskfile
	if (tim->pixelmask != NULL)
	{
		ExtractFileExtension(outfile, ext);
		StripFileExtension(outfile, outfile);
		sprintf(outfile, "%s_mask.%s", outfile, ext); 
		printf("writing %s\n", outfile);
		TIM_WriteTargaGrayscale(tim->pixelmask, tim->dim.xsize, tim->dim.ysize, outfile);
	}
	printf("done.");

	return 0;
}

int Targa2Tim_Main(int argc, char **argv)
{
	char filename[MAX_BLOODPATH], basefilename[MAX_BLOODPATH], ext[5], outfile[MAX_BLOODPATH], maskfile[MAX_BLOODPATH], *c;
	short ofsx = -1, ofsy = -1;
	unsigned int type;
	tim_image_t *tim;
	int i = 1;
	FILE *f;

	printf("=== Tga2Tim ===\n");
	if (i < 1)
		Error("not enough parms");

	// get inner file
	strcpy(filename, argv[i]);
	StripFileExtension(filename, basefilename);
	ExtractFileExtension(filename, ext);
	i++;

	// get out file
	sprintf(outfile, "%s.tga", basefilename); 
	sprintf(maskfile, "%s_mask.tga", basefilename); 
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
		if (!strcmp(argv[i], "-bpp"))
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
					Error("parse commandline: bad bpp %i", argv[i]);
			}
		}
		else if(!strcmp(argv[i], "-ofs"))
		{
			i++;
			if (i < argc)
			{
				ofsx = (short)atof(argv[i]);
				i++;
				if (i < argc)
					ofsy = (short)atof(argv[i]);
			}
		}
		else if (!strcmp(argv[i], "-mask"))
		{
			i++;
			if (i < argc)
				strcpy(maskfile, argv[i]);
		}
	}

	// check if maskfile exist
	f = fopen(maskfile, "rb");
	if (f)
		fclose(f);
	else
		strcpy(maskfile, "");
	
	// open source file, try load it
	tim = TIM_LoadFromTarga(filename, type);

	// override offsets
	if (ofsx >= 0)
		tim->dim.xpos = ofsx;
	if (ofsy >= 0)
		tim->dim.ypos = ofsy;

	// print TIM stats
	printf("%s loaded\n", filename);
	if (tim->maskfile != NULL)
		printf("%s loaded\n", tim->maskfile);
	TimEmitStats(tim);

	printf("writing %s\n", outfile);
	f = SafeOpen(outfile, "wb");
	TIM_WriteToStream(tim, f);
	fclose(f);
	printf("done.");

	return 0;
}

/*
==========================================================================================

  RAW TIM UTIL
  for burrowing other picture formats

==========================================================================================
*/

void RawPalettedTGA(char *outfile, int width, int height, const char *colormapdata, int pixelbytes, const char *pixeldata)
{
	unsigned char *buffer, *out;
	const unsigned char *in, *end;
	int i;
	FILE *f;

	// create colormapped targa header
	buffer = qmalloc(pixelbytes + 768 + 18);
	memset(buffer, 0, 18);
	buffer[1] = 1;
	buffer[2] = 1;
	buffer[5] = (256 >> 0) & 0xFF;
	buffer[6] = (256 >> 8) & 0xFF;
	buffer[7] = 24; // colormap BPP
	buffer[12] = (width >> 0) & 0xFF;
	buffer[13] = (width >> 8) & 0xFF;
	buffer[14] = (height >> 0) & 0xFF;
	buffer[15] = (height >> 8) & 0xFF;
	buffer[16] = 8;

	// 24-bit colormap, swap bgr->rgb
	out = buffer + 18;
	for (i = 0;i < 256;i++)
	{
		in = colormapdata + i*3;
		*out++ = in[2];
		*out++ = in[1];
		*out++ = in[0];
	}

	// flip upside down, write
	out = buffer + 768 + 18;
	for (i = height - 1;i >= 0;i--)
	{
		in = pixeldata + i * width;
		end = in + width;
		for (;in < end; in++)
			*out++ = in[0];
	}

	f = SafeOpen(outfile, "wb");
	fwrite(buffer, pixelbytes + 768 + 18, 1, f);
	fclose(f);

	qfree(buffer);
}

#define RAWTAG_ITEM 0x00000001
#define RAWTAG_TILE 0x0000000C

void printfpos(FILE *f)
{
	fpos_t fpos;

	fgetpos(f, &fpos);
	printf("filepos: %i\n", fpos);
}

int RawTim_Main(int argc, char **argv)
{
	char filename[MAX_BLOODPATH], basefilename[MAX_BLOODPATH], ext[5], outfile[MAX_BLOODPATH], *c;
	unsigned char *pixeldata, *colormapdata;
	int i = 1, k;
	FILE *f;

	unsigned int objtag;
	short num1, num2, num3, num4;
	char objectname[MAX_BLOODPATH];
	int obj, objsize, objwidth, objheight;
	unsigned char objx, objy, temp;
	unsigned char *multiobjects, *out; // multiple objects data, ach 8 bytes
	int width, height;
	
	printf("=== TimRaw ===\n");
	printf("%s\n", argv[i]);

	// get inner file
	strcpy(filename, argv[i]);
	StripFileExtension(filename, basefilename);
	ExtractFileExtension(filename, ext);
	i++;

	// get out file
	sprintf(outfile, "%s.tga", basefilename); 
	if (i < argc)
	{
		c = argv[i];
		if (c[0] != '-')
			strcpy(outfile, c);
	}

	// parse cmdline
	width = -1;
	height = -1;	
	for (i = i; i < argc; i++)
	{
		if (!strcmp(argv[i], "-width"))
		{
			i++;
			if (i < argc)
			{
				width = atoi(argv[i]);
				printf(" force width = %i\n", width);
			}
		}
		else if(!strcmp(argv[i], "-height"))
		{
			i++;
			if (i < argc)
			{
				height = atoi(argv[i]);
				printf(" force height = %i\n", height);
			}
		}
	}

	// open file
	f = SafeOpen(filename, "rb");

	// number of objects
	fread(&objtag, 4, 1, f);
	printf("tag: 0x%.8X\n", objtag);
	if (objtag > 32)
		objtag = 1;
	// size
	fread(&objsize, 4, 1, f);
	printf("size = %i\n", objsize);
	// colormap data
	colormapdata = qmalloc(768);
	fread(colormapdata, 768, 1, f);
	// 8 unknown bytes
	fread(&num1, 4, 1, f);
	printf(" num1 = %i\n", num1);
	fread(&num2, 2, 1, f);
	printf(" num2 = %i\n", num2);
	fread(&num3, 1, 1, f);
	printf(" num3 = %i\n", num3);
	fread(&num4, 1, 1, f);
	printf(" num4 = %i\n", num4);
	// TYPE 1 - only one image, it seems items only have such
	if (objtag == 1)
	{
		printf("== single object ==\n");
		// width and height - 2 bytes
		fread(&temp, 1, 1, f);
		objwidth = (width > 0) ? width : (int)temp;
		fread(&temp, 1, 1, f);
		objheight = (height > 0) ? height : (int)temp;
		printf(" width = %i\n", objwidth);
		printf(" height = %i\n", objheight);
		// x and y - 2 bytes
		fread(&objx, 1, 1, f);
		printf(" x = %i\n", objx);
		fread(&objy, 1, 1, f);
		printf(" y = %i\n", objy);
		// read pixels
		if (objwidth*objheight > 0)
		{
			pixeldata = qmalloc(objwidth*objheight);
			fread(pixeldata, objwidth*objheight, 1, f);
			// save TGA
			sprintf(objectname, "%s.tga", outfile);
			printf(" name %s\n", objectname);
			RawPalettedTGA(objectname, (int)objwidth, (int)objheight, colormapdata, objwidth*objheight, pixeldata);
			qfree(pixeldata);
		}
	}
	else
	{
		// read multiple objects data, each consists of 8 bytes
		multiobjects = qmalloc(objtag * 8);
		out = multiobjects;
		printfpos(f);
		for (obj = 0; obj < (int)objtag; obj++)
		{
			fread(out, 8, 1, f);
			out += 8;
		}
		// write objects
		for (obj = 0; obj < (int)objtag; obj++)
		{
			printf("== multiobject #%i ==\n", obj);
			// print 8 bytes
			for (k = 0; k < 8; k++)
				printf(" parm %i = %i\n", k, multiobjects[obj*8 + k]);
			// width and height - already defined
			objwidth = multiobjects[obj*8 + 0];
			objheight = multiobjects[obj*8 + 1];
			printf(" width = %i\n", objwidth);
			printf(" height = %i\n", objheight);
			// ~150 bytes of yet unknown data (different size per object)
			printfpos(f);
			if (obj == 0)
			{
				for (k = 0; k < 173; k++)
				{
					fread(&temp, 1, 1, f);
					if (k < 8)
						printf(" %i = %i\n", k, temp);
				}
			}
			else if (obj == 1)
			{
				for (k = 0; k < 171; k++)
				{
					fread(&temp, 1, 1, f);
					if (k < 8)
						printf(" %i = %i\n", k, temp);
				}
			}
			else if (obj == 2)
			{
				for (k = 0; k < 173; k++)
				{
					fread(&temp, 1, 1, f);
					if (k < 8)
						printf(" %i = %i\n", k, temp);
				}
			}
			else if (obj == 3)
			{
				for (k = 0; k < 183; k++)
				{
					fread(&temp, 1, 1, f);
					if (k < 8)
						printf(" %i = %i\n", k, temp);
				}
			}
			else if (obj == 4)
			{
				for (k = 0; k < 172; k++)
				{
					fread(&temp, 1, 1, f);
					if (k < 8)
						printf(" %i = %i\n", k, temp);
				}
			}
			// read pixels (there is some last pixels defined that is black, they are not presented)
			if (objwidth*objheight > 0)
			{
				pixeldata = qmalloc(objwidth*objheight);
				memset(pixeldata, 0, objwidth*objheight);
				fread(pixeldata, objwidth*objheight, 1, f); 
				
				fseek(f, -416, SEEK_CUR);

				// save TGA
				sprintf(objectname, "%s_object%03i.tga", outfile, obj);
				printf(" write %s\n\n", objectname);
				RawPalettedTGA(objectname, (int)objwidth, (int)objheight, colormapdata, objwidth*objheight, pixeldata);
				qfree(pixeldata);
			}
		}
	}	
	// debug!
	//fgetpos(f, &fpos);
	//printf(" filepos: %i\n", fpos);
	//fsetpos(f, &fpos)
	qfree(colormapdata);
	fclose(f);

	return 0;
}
