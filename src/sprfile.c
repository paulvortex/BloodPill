////////////////////////////////////////////////////////////////
//
// Darkplaces engine/Quake SPR/SPR32 sprites format
// parts of code borrowed from util called lhfire
// written by Forest [LordHavoc] Hale
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
#include "sprfile.h"
#include "rawfile.h"
#include "mem.h"

/*
==========================================================================================

  Backend

==========================================================================================
*/

void fput_littleint(int num, FILE* file)
{
	fputc(num & 0xFF, file);
	fputc((num >> 8) & 0xFF, file);
	fputc((num >> 16) & 0xFF, file);
	fputc((num >> 24) & 0xFF, file);
}

void fput_littlefloat(float num, FILE* file)
{
	/*
	// although this is guarenteed to write out IEEE compliant floats,
	// it is slow and silly unless the machine uses a non-IEEE compliant float...
	int exponent;
	double mantissa;
	int mantissabits;
	int data, *test;
	mantissa = frexp(num, &exponent);
	mantissabits = (int) (mantissa * 16777216.0);
	mantissabits &= 0x007FFFFF;
	if (exponent < -127) exponent = -127;
	if (exponent > 127) exponent = 127;
	data = ((exponent + 126) << 23) | mantissabits; // 126 seems a bit odd to me, but worked...
	if (num < 0.0f)
		data |= 0x80000000;
//	test = (void*) &num; // just for testing the results if needed
//	if (data != *test)
//		data = *test;
	*/
	int *data;
	data = (void*) &num; // note: this is *NOT* byte order dependent
	fput_littleint(*data, file);
}

/*
==========================================================================================

  WRITE

==========================================================================================
*/

void SPR_WriteHeader(FILE *f, rawblock_t *rawblock, sprversion_t version, sprtype_t type)
{
	int maxwidth, maxheight, i;

	// calc max width/height
	maxwidth = maxheight = 0;
	for (i = 0; i < rawblock->chunks; i++)
	{
		maxwidth = max(maxwidth, (rawblock->chunk[i].width + rawblock->chunk[i].x));
		maxheight = max(maxheight, (rawblock->chunk[i].height + rawblock->chunk[i].y));
	}

	// write ident IDSP
	fputc('I', f);fputc('D', f);fputc('S', f);fputc('P', f); 
	// version
	fput_littleint(version, f); // 32bit color
	// type
	fput_littleint(type, f); 
	// boundingradius
	fput_littlefloat((float) sqrt((maxwidth*0.5)*(maxwidth*0.5)+(maxheight*0.5)*(maxheight*0.5)), f); 
	// maxwidth/maxheight
	fput_littleint(maxwidth, f); 
	fput_littleint(maxheight, f);
	// numframes
	fput_littleint(rawblock->chunks, f);
	// beamlength
	fput_littlefloat(0.0f, f); 
	// synctype
	fput_littleint(0, f);
}

void SPR_WriteFrameHeader(FILE *f, sprframetype_t frametype, rawchunk_t *chunk, int cx, int cy)
{
	fput_littleint(frametype, f);
	fput_littleint(chunk->x + cx, f);
	fput_littleint(chunk->y + cy, f);
	fput_littleint(chunk->width, f);
	fput_littleint(chunk->height, f);
}

void SPR_WriteFromRawblock(rawblock_t *rawblock, char *outfile, sprversion_t version, sprtype_t type, int cx, int cy, byte shadowpixel, byte shadowalpha, int flags)
{
	int i, p, d;
	byte *colormap, *buf, color[4];
	rawchunk_t *chunk;
	FILE *f;

	f = SafeOpenWrite(outfile);
	SPR_WriteHeader(f, rawblock, version, type);

	// write frames
	for (i = 0; i < rawblock->chunks; i++)
	{
		chunk = &rawblock->chunk[i];
		colormap = (chunk->colormap != NULL) ? chunk->colormap : rawblock->colormap;
		SPR_WriteFrameHeader(f, SPR_FRAME_SINGLE, chunk, chunk->x + cx, chunk->y + cy);
		if (version == SPR_DARKPLACES) // 32bit RGBA8 raw, ready for OpenGL
		{
			buf = qmalloc(chunk->size * 4);
			// in Blood Omen, black pixels (0) were transparent
			// also we optionally threating shadow pixel as transparent
			for (p = 0; p < chunk->size; p++) 
			{
				d = chunk->pixels[p];
				color[0] = colormap[d*3];
				color[1] = colormap[d*3 + 1];
				color[2] = colormap[d*3 + 2];
				//if (shadowpixel >= 0 && !memcmp(colormap + shadowpixel*3, color, 3))
				if (shadowpixel >= 0 && (d == 15 || d == 31 || d == 47 || d == 63 || d == 79 || d == 95 || d == 111 || d == 127 || d == 143 || d == 159 || d == 175 || d == 191 || d == 207 || d == 223 || d == 239 || d == 255))
					color[3] = shadowalpha;
				else if (!memcmp(colormap, color, 3))
					color[3] = 0;
				else
					color[3] = 255;
				color[0] = colormap[d*3];
				color[1] = colormap[d*3 + 1];
				color[2] = colormap[d*3 + 2];
				// todo: fix for texture filtering - fill transparent pixels from neighbours 
				// so we don't see a black outlines on objects
				*buf++ = color[0];
				*buf++ = color[1];
				*buf++ = color[2];
				*buf++ = color[3];
			}
			buf -= chunk->size * 4;
			fwrite(buf, chunk->size * 4, 1, f);
			qfree(buf);
		}
		else
		{
			Error("SPR_WriteFromRawblock: cannot write quake sprites yet\n");
		}
	}
}
