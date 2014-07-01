////////////////////////////////////////////////////////////////
//
// TIM files loader/writer
// coded by Pavel [VorteX] Timofeyev and placed to public domain
// thanks to Klarth (stevemonaco@hotmail.com) and  Raul Sobon (Cheekyboy@2-hot.com) for TIM spec's
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////


#include "bloodpill.h"
#include "timfile.h"
#include "filter.h"

/*
==========================================================================================

  General loader/writer

==========================================================================================
*/

tim_image_t *EmptyTIM(unsigned int type)
{
	tim_image_t *tim;

	tim = (tim_image_t *)mem_alloc(sizeof(tim_image_t));
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
		mem_free(tim->CLUT); 
	if (tim->pixels != NULL)
		mem_free(tim->pixels); 
	if (tim->pixelmask != NULL) 
		mem_free(tim->pixelmask); 
	if (tim->maskfile != NULL)
		mem_free(tim->maskfile);
	mem_free(tim); 
}

tim_image_t *TimError(tim_image_t *tim, char *error, ...)
{
	va_list argptr;

	va_start(argptr, error);
	tim->error = true;
	tim->errorstr = (char *)mem_alloc(strlen(error)+1);
	vsprintf(tim->errorstr, error, argptr);
	va_end(argptr);

	return tim;
}

tim_image_t *TIM_LoadFromBuffer(byte *buf, int buflen)
{
	tim_image_t *tim;
	long nextobjlen;
	unsigned char *out;
	int filepos = 0, y;

	tim = EmptyTIM(0);

	// 0x10 should be at beginning of standart TIM
	if (buflen < 4)
		return TimError(tim, "unexpected EOF at tag");
	tim->tag = ReadUInt(buf);
	if (tim->tag != TIM_TAG)
		return TimError(tim, "funky tag %.8X", tim->tag);
	buf += 4;
	buflen -= 4;

	// second uint is type
	if (buflen < 4)
		return TimError(tim, "unexpected EOF at type");
	tim->type = ReadUInt(buf);
	buf += 4;
	buflen -= 4;
	
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
	if (buflen < 4)
		return TimError(tim, "unexpected EOF at CLUT/image");
	nextobjlen = ReadUInt(buf);
	buf += 4;
	buflen -= 4;

	// load CLUT if presented
	if (tim->type == TIM_4Bit || tim->type == TIM_8Bit)
	{
		tim->CLUT = (tim_clutinfo_t *)mem_alloc(sizeof(tim_clutinfo_t));
		memcpy(tim->CLUT, buf, nextobjlen-4);
		buf += nextobjlen-4;
		buflen -= nextobjlen-4;
		nextobjlen = ReadUInt(buf);
		buf += 4;
		buflen -= 4;
	}	

	// read dimension info
	if (buflen < sizeof(tim_diminfo_t))
		return TimError(tim, "unexpected EOF at dimension info");
	memcpy(&tim->dim, buf, sizeof(tim_diminfo_t));
	buf += sizeof(tim_diminfo_t);
	buflen -= sizeof(tim_diminfo_t);

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
	if (buflen < tim->pixelbytes)
		return TimError(tim, "unexpected EOF at pixel data (%i bytes)", tim->pixelbytes);
	tim->pixels = (byte *)mem_alloc(tim->pixelbytes);
	memcpy(tim->pixels, buf, tim->pixelbytes);
	buf += tim->pixelbytes;
	buflen -= tim->pixelbytes;

	// extract pixel mask for 16-bit TIM
	if (tim->type == TIM_16Bit)
	{
		tim->pixelmask = (byte *)mem_alloc(tim->dim.xsize*tim->dim.ysize);
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
		tim->CLUT = (tim_clutinfo_t *)mem_alloc(sizeof(tim_clutinfo_t));
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
	tim->pixels = (byte *)mem_alloc(tim->pixelbytes);
	if (fread(tim->pixels, tim->pixelbytes, 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at pixel data (%i bytes)" : "unable to read pixel data (%i bytes)", tim->pixelbytes);

	// extract pixel mask for 16-bit TIM
	if (tim->type == TIM_16Bit)
	{
		tim->pixelmask = (byte *)mem_alloc(tim->dim.xsize*tim->dim.ysize);
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
		data = (byte *)mem_alloc(tim->pixelbytes);
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
		mem_free(data);
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
	pixeldata = (byte *)mem_alloc(width * height);
	if (fread(pixeldata, width * height, 1, f) < 1)
		return TimError(tim, feof(f) ? "unexpected EOF at maskfile pixeldata" : "unable to read TGA pixeldata");

	tim->pixelmask = (byte *)mem_alloc(width * height);
	// fill pixels, flip upside down
	out = tim->pixelmask;
	for (y = height - 1;y >= 0;y--)
	{
		in = pixeldata + y * width;
		end = in + width;
		for (;in < end; in++)
			*out++ = (in[0]) ? 1 : 0;
	}
	mem_free(pixeldata);

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
			colormapdata = (byte *)mem_alloc(colormaplen * ((targaheader[7] == 16) ? 2 : 3));
			if (fread(colormapdata, colormaplen * ((targaheader[7] == 16) ? 2 : 3), 1, f) < 1)
				return TimError(tim, feof(f) ? "unexpected EOF at TGA colormap" : "unable to read TGA colormap");

			// load pixel data
			pixeldata = (byte *)mem_alloc(width * height);
			if (fread(pixeldata, width * height, 1, f) < 1)
				return TimError(tim, feof(f) ? "unexpected EOF at TGA pixeldata" : "unable to read TGA pixeldata");

			// fill TIM header
			tim->type = TIM_8Bit;
			tim->dim.xpos = targaheader[8] + targaheader[9]*256;
			tim->dim.ypos = targaheader[10] + targaheader[11]*256;
			tim->dim.xsize = width;
			tim->dim.ysize = height;
			tim->CLUT = (tim_clutinfo_t *)mem_alloc(sizeof(tim_clutinfo_t));
			memset(tim->CLUT, 0, sizeof(tim_clutinfo_t));
			tim->pixelbytes = tim->dim.xsize*tim->dim.ysize;
			tim->pixels = (byte *)mem_alloc(tim->pixelbytes);
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

			mem_free(colormapdata);
			mem_free(pixeldata);
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
			pixeldata = (byte *)mem_alloc(width * height * ((targaheader[16] == 16) ? 2 : 3));
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
			tim->pixels = (byte *)mem_alloc(tim->pixelbytes);
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

			mem_free(pixeldata);
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
			pixeldata = (byte *)mem_alloc(width * height * ((targaheader[16] == 16) ? 2 : 3));
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
			tim->pixels = (byte *)mem_alloc(tim->pixelbytes);
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
			mem_free(pixeldata);
			break;
		default:
			return TimError(tim, "unknown TIM type %.8X requested", type);
	}

	return tim;
}

tim_image_t *TIM_LoadFromTarga(char *filename, unsigned int type)
{
	tim_image_t *tim;
	char basename[MAX_OSPATH], ext[15], maskfile[MAX_OSPATH];
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

	tim->maskfile = (char *)mem_alloc(MAX_OSPATH);
	strcpy(tim->maskfile, maskfile);
	TIM_LoadPixelmaskFromTargaStream(f, tim);
	if (tim->error)
		Error("Error loading maskfile %s: %s", maskfile, tim->errorstr);
	fclose(f);

	return tim;
}

void TIM_WriteTargaGrayscale(byte *data, short width, short height, char *savefile)
{
	byte *buffer, *out;
	byte *in, *end;
	int i;
	FILE *f;

	// create grayscale targa header
	buffer = (byte *)mem_alloc(width*height + 18);
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
	f = SafeOpenWrite(savefile);
	fwrite(buffer, width*height + 18, 1, f);
	WriteClose(f);
	mem_free(buffer);
}

void TIM_WriteTarga(tim_image_t *tim, char *savefile, bool bpp16to24, bool bpp8to32, bool keep_palette, imgfilter_t scaler, float colorscale, int colorsub)
{
	unsigned char *buffer, *out;
	const unsigned char *in, *end, *clut;
	unsigned int width, height;
	FILE *f;
	int y;

	if (tim->error)
		return; // don't write erroneous TIM

	width = ImgFilter_Size(tim->dim.xsize, scaler);
	height = ImgFilter_Size(tim->dim.ysize, scaler);

	// write header
	f = SafeOpenWrite(savefile);
	switch(tim->type)
	{
		case TIM_4Bit:
			break;
		case TIM_8Bit:	
			if (!keep_palette) // convert 8-bit to 32-bit
			{
				// todo: bpp16to24, bpp8to32?
				buffer = (byte *)mem_alloc(width*height*4 + 18);
				memset(buffer, 0, 18);
				buffer[2] = 2; // uncompressed
				buffer[8] = (tim->dim.xpos >> 0) & 0xFF;
				buffer[9] = (tim->dim.xpos >> 8) & 0xFF;
				buffer[10] = (tim->dim.ypos >> 0) & 0xFF;
				buffer[11] = (tim->dim.ypos >> 8) & 0xFF;
				buffer[12] = (width >> 0) & 0xFF;
				buffer[13] = (width >> 8) & 0xFF;
				buffer[14] = (height >> 0) & 0xFF;
				buffer[15] = (height >> 8) & 0xFF;
				buffer[16] = 32;
				buffer[17] = 8; // has alpha flag
				// swap bgr->rgb, flip upside down
				out = buffer + 18;
				for (y = tim->dim.ysize - 1;y >= 0;y--)
				{
					in = tim->pixels + y * tim->dim.xsize;
					end = in + tim->dim.xsize;
					for (;in < end; in++)
					{
						clut = tim->CLUT->data + in[0]*2;
						*out++ = ((clut[1] & 0x7C) >> 2) * 8;
						*out++ = (((clut[0] & 0xE0) >> 5) + ((clut[1] & 0x3) << 3)) * 8;
						*out++ = (clut[0] & 0x1F) * 8; 
						*out++ = (in[0] == 0) ? 0 : ((in[0] == 255) ? 128 : 255);
					}
				}
				// transform
				ImgFilter_ColorTransform(tim->dim.xsize, tim->dim.ysize, 4, buffer + 18, colorscale, colorsub);
				ImgFilter(tim->dim.xsize, tim->dim.ysize, 4, buffer + 18, NULL, 0, NULL, scaler);
				// write file
				fwrite(buffer, width*height*4 + 18, 1, f);
				mem_free(buffer);
			}
			else
			{
				buffer = (byte *)mem_alloc(width*height + (bpp8to32 ? 1024 : (bpp16to24 ? 768 : 512)) + 18);
				memset(buffer, 0, 18);
				buffer[1] = 1; // colormapped
				buffer[2] = 1; // uncompressed, colormapped
				buffer[5] = (256 >> 0) & 0xFF;
				buffer[6] = (256 >> 8) & 0xFF;
				buffer[7] = (bpp8to32 ? 32 : (bpp16to24 ? 24 : 16)); // colormap BPP
				buffer[8] = (tim->dim.xpos >> 0) & 0xFF;
				buffer[9] = (tim->dim.xpos >> 8) & 0xFF;
				buffer[10] = (tim->dim.ypos >> 0) & 0xFF;
				buffer[11] = (tim->dim.ypos >> 8) & 0xFF;
				buffer[12] = (width >> 0) & 0xFF;
				buffer[13] = (width >> 8) & 0xFF;
				buffer[14] = (height >> 0) & 0xFF;
				buffer[15] = (height >> 8) & 0xFF;
				buffer[16] = 8;
				// write 16 or 24-bit colormap from 15-bit CLUT, swap bgr->rgb
				out = buffer + 18;
				for (y = 0;y < 256;y++)
				{
					in = tim->CLUT->data + y*2;
					if (bpp8to32)
					{
						*out++ = ((in[1] & 0x7C) >> 2) * 8;
						*out++ = (((in[0] & 0xE0) >> 5) + ((in[1] & 0x3) << 3)) * 8;
						*out++ = (in[0] & 0x1F) * 8; 
						*out++ = (y == 0) ? 0 : ((y == 255) ? 128 : 255); 
					}
					else if (bpp16to24)
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
				out = buffer + (bpp8to32 ? 1024 : (bpp16to24 ? 768 : 512)) + 18;
				for (y = tim->dim.ysize - 1;y >= 0;y--)
				{
					in = tim->pixels + y * tim->dim.xsize;
					end = in + tim->dim.xsize;
					for (;in < end; in++)
						*out++ = in[0];
				}
				// transform
				if (bpp8to32)
					ImgFilter_ColorTransform(16, 16, 4, buffer + 18, colorscale, colorsub);
				else if (bpp16to24)
					ImgFilter_ColorTransform(16, 16, 3, buffer + 18, colorscale, colorsub);
				ImgFilter(tim->dim.xsize, tim->dim.ysize, 1, buffer + (bpp8to32 ? 1024 : (bpp16to24 ? 768 : 512)) + 18, buffer + 18, (bpp8to32 ? 4 : (bpp16to24 ? 3 : 2)), NULL, scaler);
				// write file
				fwrite(buffer, width*height + (bpp8to32 ? 1024 : (bpp16to24 ? 768 : 512)) + 18, 1, f);
				mem_free(buffer);
			}
			break;
		case TIM_16Bit:
			buffer = (byte *)mem_alloc(width*height*(bpp16to24 ? 3 : 2) + 18);
			memset(buffer, 0, 18);
			buffer[2] = 2; // uncompressed
			buffer[8] = (tim->dim.xpos >> 0) & 0xFF;
			buffer[9] = (tim->dim.xpos >> 8) & 0xFF;
			buffer[10] = (tim->dim.ypos >> 0) & 0xFF;
			buffer[11] = (tim->dim.ypos >> 8) & 0xFF;
			buffer[12] = (width >> 0) & 0xFF;
			buffer[13] = (width >> 8) & 0xFF;
			buffer[14] = (height >> 0) & 0xFF;
			buffer[15] = (height >> 8) & 0xFF;
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
			// transform
			if (bpp16to24)
				ImgFilter_ColorTransform(tim->dim.xsize, tim->dim.ysize, 3, buffer + 18, colorscale, colorsub);
			ImgFilter(tim->dim.xsize, tim->dim.ysize, bpp16to24 ? 3 : 2, buffer + 18, NULL, 0, NULL, scaler);
			// write file
			fwrite(buffer, width*height*(bpp16to24 ? 3 : 2) + 18, 1, f);
			mem_free(buffer);
			break;
		case TIM_24Bit:
			buffer = (byte *)mem_alloc(width*height*3 + 18);
			memset(buffer, 0, 18);
			buffer[2] = 2; // uncompressed
			buffer[8] = (tim->dim.xpos >> 0) & 0xFF;
			buffer[9] = (tim->dim.xpos >> 8) & 0xFF;
			buffer[10] = (tim->dim.ypos >> 0) & 0xFF;
			buffer[11] = (tim->dim.ypos >> 8) & 0xFF;
			buffer[12] = (width >> 0) & 0xFF;
			buffer[13] = (width >> 8) & 0xFF;
			buffer[14] = (height >> 0) & 0xFF;
			buffer[15] = (height >> 8) & 0xFF;
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
			// transform
			ImgFilter_ColorTransform(tim->dim.xsize, tim->dim.ysize, 3, buffer + 18, colorscale, colorsub);
			ImgFilter(tim->dim.xsize, tim->dim.ysize, 3, buffer + 18, NULL, 0, NULL, scaler);
			// write file
			fwrite(buffer, width*height*3 + 18, 1, f);
			mem_free(buffer);
			break;
		default:
			break;
	}
	WriteClose(f);
}

/*
==========================================================================================

  UTIL MAIN

==========================================================================================
*/

void TimEmitStats(tim_image_t *tim)
{
	Verbose("        type: %i-bit TIM\n", tim->bpp);
	Verbose(" orientation: %ix%i\n", tim->dim.xpos, tim->dim.ypos);
	Verbose("        size: %ix%i\n", tim->dim.xsize, tim->dim.ysize);
	if (tim->CLUT == NULL)
		Verbose("        CLUT: no\n");
	else
	{
		Verbose("        CLUT: yes\n");
		Verbose("   CLUT flag: %i\n", tim->CLUT->flags);
		Verbose("   CLUT cols: %i\n", tim->CLUT->columns);
		Verbose("   CLUT pals: %i\n", tim->CLUT->palettes);
	}
	Verbose(" pixel bytes: %i\n", tim->pixelbytes);
	Verbose(" file length: %i\n", tim->filelen);
}

int Tim2Targa_Main(int argc, char **argv)
{
	int i = 1, colorsub;
	char filename[MAX_OSPATH], ext[5], outfile[MAX_OSPATH], *c;
	tim_image_t *tim;
	imgfilter_t scaler;
	float colorscale;
	bool bpp16to24;
	FILE *f; 

	Print("=== Tim2Tga ===\n");
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
	scaler = FILTER_NONE;
	colorscale = 1.0f;
	colorsub = 0;
	for (i = i; i < argc; i++)
	{
		if (!strcmp(argv[i], "-16to24"))
		{
			Verbose("Option: targa compatibility mode (converting 16-bit to 24-bit)\n");
			bpp16to24 = true;
			continue;
		}
		if (!strcmp(argv[i], "-scale2x"))
		{
			Verbose("Option: scaling the image to 200% with Scale2X filter\n");
			scaler = FILTER_SCALE2X;
			continue;
		}
		if (!strcmp(argv[i], "-scale4x"))
		{
			Verbose("Option: scaling the image to 400% with Scale2X filter\n");
			scaler = FILTER_SCALE4X;
			continue;
		}
		if (!strcmp(argv[i], "-xbrz2x"))
		{
			Verbose("Option: scaling the image to 200% with xBRz 2X filter\n");
			scaler = FILTER_XBRZ2X;
			continue;
		}
		if (!strcmp(argv[i], "-xbrz4x"))
		{
			Verbose("Option: scaling the image to 400% with xBRz 4X filter\n");
			scaler = FILTER_XBRZ4X;
			continue;
		}
		if (!strcmp(argv[i], "-colorscale"))
		{
			i++; 
			if (i < argc)
			{
				colorscale = (float)atof(argv[i]);
				Verbose("Option: scale colors by %f\n", colorscale);
			}
			continue;
		}
		if (!strcmp(argv[i], "-colorsub"))
		{
			i++; 
			if (i < argc)
			{
				colorsub = atoi(argv[i]);
				Verbose("Option: subtract colors by %i\n", colorsub);
			}
			continue;
		}
		Warning("unknown parameter '%s'", argv[i]);
	}

	// open source file, try load it
	f = SafeOpen(filename, "rb");
	tim = TIM_LoadFromStream(f);
	fclose(f);
	if (tim->error)
		Error("Error loading %s: %s", filename, tim->errorstr);

	// print TIM stats
	Verbose("%s loaded\n", filename);
	TimEmitStats(tim);

	// write basefile
	Verbose("writing %s\n", outfile);
	TIM_WriteTarga(tim, outfile, bpp16to24, false, true, scaler, colorscale, colorsub);

	// write maskfile
	if (tim->pixelmask != NULL)
	{
		ExtractFileExtension(outfile, ext);
		StripFileExtension(outfile, outfile);
		sprintf(outfile, "%s_mask.%s", outfile, ext); 
		Verbose("writing %s\n", outfile);
		TIM_WriteTargaGrayscale(tim->pixelmask, tim->dim.xsize, tim->dim.ysize, outfile);
	}
	Print("done.\n");

	return 0;
}

int Targa2Tim_Main(int argc, char **argv)
{
	char filename[MAX_OSPATH], basefilename[MAX_OSPATH], ext[5], outfile[MAX_OSPATH], maskfile[MAX_OSPATH], *c;
	short ofsx = -1, ofsy = -1;
	unsigned int type;
	tim_image_t *tim;
	int i = 1, colorsub;
	float colorscale;
	FILE *f;

	Print("=== Tga2Tim ===\n");
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
	colorscale = 1.0f;
	colorsub = 0;
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
			continue;
		}
		if(!strcmp(argv[i], "-ofs"))
		{
			i++;
			if (i < argc)
			{
				ofsx = (short)atof(argv[i]);
				i++;
				if (i < argc)
					ofsy = (short)atof(argv[i]);
			}
			continue;
		}
		if (!strcmp(argv[i], "-mask"))
		{
			i++;
			if (i < argc)
				strcpy(maskfile, argv[i]);
			continue;
		}
		if (!strcmp(argv[i], "-colorscale"))
		{
			i++; 
			if (i < argc)
			{
				colorscale = (float)atof(argv[i]);
				Verbose("Option: scale colors by %f\n", colorscale);
			}
			continue;
		}
		if (!strcmp(argv[i], "-colorsub"))
		{
			i++; 
			if (i < argc)
			{
				colorsub = atoi(argv[i]);
				Verbose("Option: subtract colors by %i\n", colorsub);
			}
			continue;
		}
		Warning("unknown parameter '%s'", argv[i]);
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
	Verbose("%s loaded\n", filename);
	if (tim->maskfile != NULL)
		Verbose("%s loaded\n", tim->maskfile);
	TimEmitStats(tim);

	Print("writing %s\n", outfile);
	f = SafeOpenWrite(outfile);
	TIM_WriteToStream(tim, f);
	WriteClose(f);
	Print("done.\n");

	return 0;
}


