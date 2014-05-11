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

#include "../bloodpill.h"
#include "../rawfile.h"
#include "../mem.h"
#include "dpspr32file.h"

// sprite header
typedef struct
{ 
	char  name[4];    // 4 (+0) "IDSP"
	long  ver1;       // 4 (+4) Version = 1
	long  type;       // 4 (+8) See bove
	float radius;     // 4 (+12) Bounding Radius
	long  maxwidth;   // 4 (+16) Width of the largest frame
	long  maxheight;  // 4 Height of the largest frame
	long  nframes;    // 4 Number of frames
	float beamlength; // 4 pushs the sprite away, strange legacy from DOOM?
	long  synchtype;  // 4 0=synchron 1=random
} spr_t;

/*
==========================================================================================

  Backend

==========================================================================================
*/

void FPutLittleInt(int num, FILE* file)
{
	fputc(num & 0xFF, file);
	fputc((num >> 8) & 0xFF, file);
	fputc((num >> 16) & 0xFF, file);
	fputc((num >> 24) & 0xFF, file);
}

void FPutLittleFloat(float num, FILE* file)
{
	union {int i; float f;} in;
	in.f = num;
	FPutLittleInt(in.i, file);
}

int ReadLittleInt(byte *buffer)
{
	return buffer[3]*16777216 + buffer[2]*65536 + buffer[1]*256 + buffer[0];
}

float ReadLittleFloat (byte *buffer)
{
	union {byte b[4]; float f;} in;

	in.b[0] = buffer[0];
	in.b[1] = buffer[1];
	in.b[2] = buffer[2];
	in.b[3] = buffer[3];
	return in.f;
}

/*
==========================================================================================

  WRITE

==========================================================================================
*/

bool SPR_ReadHeader(byte *buf, int bufsize, spr_t *sprheader) 
{
	memset(sprheader, 0, sizeof(spr_t));
	if (buf[0] != 'I' || buf[1] != 'D' || buf[2] != 'S' || buf[3] != 'P')
		return false;
	if (bufsize < sizeof(spr_t))
		return false;
	sprheader->ver1 = ReadLittleInt(buf + 4);
	sprheader->type = ReadLittleInt(buf + 8);
	sprheader->radius = ReadLittleFloat(buf + 12);
	sprheader->maxwidth = ReadLittleInt(buf + 16);
	sprheader->maxheight = ReadLittleInt(buf + 20);
	sprheader->nframes = ReadLittleInt(buf + 24);
	sprheader->beamlength = ReadLittleFloat(buf + 28);
	sprheader->synchtype = ReadLittleInt(buf + 32);
	return true;
}

FILE *SPR_BeginFile(char *outfile, QuakeSpriteVersion_t version, QuakeSpriteType_t type, int maxwidth, int maxheight, int numframes, bool mergeintoexistingfile)
{
	spr_t mergespr;
	byte mergedata[36];
	float boundradius;
	FILE *f;

	boundradius = (float)sqrt((maxwidth*0.5)*(maxwidth*0.5)+(maxheight*0.5)*(maxheight*0.5));

	// check if need to rewrite file
	if (mergeintoexistingfile)
	{
		f = OpenReadWrite(outfile);
		// file exists
		if (f)
		{
			fseek(f, 0, SEEK_SET);
			// file at least 36 bytes
			if (fread(&mergedata, sizeof(spr_t), 1, f))
			{
				// file is sprite of same version
				mergeintoexistingfile = SPR_ReadHeader(mergedata, sizeof(spr_t), &mergespr);
				if (mergeintoexistingfile && mergespr.ver1 == version && mergespr.type == type)
				{
					// allow merged write
					boundradius = max(boundradius, mergespr.radius);
					maxwidth = max(maxwidth, mergespr.maxwidth);
					maxheight = max(maxheight, mergespr.maxheight);
					numframes = numframes + mergespr.nframes;
					fseek(f, 0, SEEK_SET);
					goto writeheader;
				}
			}
			WriteClose(f);
		}
	}
	// write empty file
	mergeintoexistingfile = false;
	f = SafeOpenWrite(outfile);

writeheader:
	// IDSP
	fputc('I', f);fputc('D', f);fputc('S', f);fputc('P', f); 
	// version
	FPutLittleInt(version, f); // 32bit color
	// type
	FPutLittleInt(type, f); 
	// boundingradius
	FPutLittleFloat(boundradius, f); 
	// maxwidth/maxheight
	FPutLittleInt(maxwidth, f); 
	FPutLittleInt(maxheight, f);
	// numframes
	FPutLittleInt(numframes, f);
	// beamlength
	FPutLittleFloat(0.0f, f); 
	// synctype
	FPutLittleInt(0, f);

	// append if merging
	if (mergeintoexistingfile)
		fseek(f, 0, SEEK_END);

	return f;
}

void SPR_WriteFrameHeader(FILE *f, QuakeSpriteFrameType_t frametype, int width, int height, int cx, int cy)
{
	FPutLittleInt(frametype, f);
	FPutLittleInt(cx, f);
	FPutLittleInt(cy, f);
	FPutLittleInt(width, f);
	FPutLittleInt(height, f);
}

// fixme: this needs cleanup
void SPR_WriteFromRawblock(rawblock_t *rawblock, char *outfile, QuakeSpriteType_t type, int cx, int cy, float alpha, int flags, bool mergeintoexistingfile, list_t *tailfiles)
{
	int i, d, r, w, h, maxwidth, maxheight, cropx[2], cropy[2], addx[2], addy[2], cropwidth, cropheight, realwidth, realheight, numframes;
	QuakeSpriteVersion_t version = SPR_SPRITE32;
	byte *colormap, *buf, normalalpha, color[4], *mergedata[16];
	size_t mergedatalen[16];
	double cdiv, cd[3];
	rawchunk_t *chunk;
	FILE *f;

	// calc whole alpha
	normalalpha = (byte)(255 * alpha);

	// calc max width/height, write header
	maxwidth = maxheight = 0;
	for (i = 0; i < rawblock->chunks; i++)
	{
		maxwidth = max(maxwidth, (rawblock->chunk[i].width + rawblock->chunk[i].x));
		maxheight = max(maxheight, (rawblock->chunk[i].height + rawblock->chunk[i].y));
	}

	// calc num frames
	numframes = rawblock->chunks;
	if (tailfiles)
	{
		if (tailfiles->items > 16)
			Error("SPR_WriteFromRawblock: too many tail files to merge, should be not more than 16\n");
		buf = (byte *)mem_alloc(sizeof(spr_t));
		for (i = 0; i < tailfiles->items; i++)
		{
			
			// read header
			f = SafeOpen(tailfiles->item[i], "rb");
			if (fread(buf, sizeof(spr_t), 1, f) < 1)
				Error("SPR_WriteFromRawblock: merge tailfiles: broken file\n");
			// check type
			if (buf[0] != 'I' && buf[1] != 'D' && buf[2] != 'S' && buf[3] != 'P')
				Error("SPR_WriteFromRawblock: merge tailfiles: %s is not IDSP file\n", tailfiles->item[i]);
			// check version
			if ((QuakeSpriteVersion_t)ReadLittleInt(buf + 4) != version)
				Error("SPR_WriteFromRawblock: merge tailfiles: %s is not same sprite version\n", tailfiles->item[i]);
			// check type
			if ((QuakeSpriteType_t)ReadLittleInt(buf + 8) != type)
				Error("SPR_WriteFromRawblock: merge tailfiles: %s is bad type %i, should be %i\n", tailfiles->item[i], ReadLittleInt(buf + 8), type);
			// calc bounds
			maxwidth = max(maxwidth, ReadLittleInt(buf + 16));
			maxheight = max(maxheight, ReadLittleInt(buf + 20));
			numframes = numframes + ReadLittleInt(buf + 24);
			// save merge data
			mergedatalen[i] = Q_filelength(f) - sizeof(spr_t);
			mergedata[i] = (byte *)mem_alloc(mergedatalen[i]);
			if (fread(mergedata[i], mergedatalen[i], 1, f) < 1)
				Error("SPR_WriteFromRawblock: merge tailfiles: error reading file %s\n", tailfiles->item[i]);
			fclose(f);
		}
		mem_free(buf);
		buf = NULL;
	}

	// write sprite header
	f = SPR_BeginFile(outfile, version, type, maxwidth, maxheight, numframes, mergeintoexistingfile);

	//54
	//printf("Exporting %s: \n", outfile);
	// write frames
	for (i = 0; i < rawblock->chunks; i++)
	{
		chunk = &rawblock->chunk[i];
		colormap = (chunk->colormap != NULL) ? chunk->colormap : rawblock->colormap;
		if (version == SPR_SPRITE32) // 32bit RGBA8 raw, ready for OpenGL
		{
			// create initial image
			// in Blood Omen, black pixels (0) were transparent
			// also we optionally threating shadow pixel as transparent
			buf = (byte *)mem_alloc(chunk->size * 4);
			for (h = 0; h < chunk->height; h++)
			for (w = 0; w < chunk->width; w++)
			{
				d = chunk->pixels[h*chunk->width + w];
				color[0] = colormap[d*3];
				color[1] = colormap[d*3 + 1];
				color[2] = colormap[d*3 + 2];
				color[3] = (rawblock->alphamap) ? (byte)(rawblock->alphamap[d] * alpha) : normalalpha;
				// fill transparent pixels from neighbours so sprites will have correct outlines
				#define fill(ox,oy,weight) { r = chunk->pixels[(h+oy)*chunk->width + w + ox] * 3; if (r != 0) { cd[0] += colormap[r]*weight; cd[1] += colormap[r + 1]*weight; cd[2] += colormap[r + 2]*weight; cdiv++; } }
				if (d == 0)
				{
					cd[0] = 0; cd[1] = 0; cd[2] = 0; cdiv = 0; 
					// sample:
					// up left, up center, up right
					// left, right
					// down left, down center, down right
					if (h > 0)
					{
						if (w > 0)                fill(-1, -1, 0.25)
						                          fill( 0, -1, 1.00)
						if ((w+1) < chunk->width) fill( 1, -1, 0.25)
					}
					if (w > 0)                    fill(-1,  0, 1.00)
					if ((w+1) < chunk->width)     fill( 1,  0, 1.00)
					if ((h+1) < chunk->height)
					{
						if (w > 0)                fill(-1,  1, 0.25)
						                          fill( 0,  1, 1.00)
						if ((w+1) < chunk->width) fill( 1,  1, 0.25)
					}
					// average
					if (cdiv != 0)
					{
						color[0] = (byte)((cd[0] / cdiv) * 0.75 + colormap[d*3] * 0.2);
						color[1] = (byte)((cd[1] / cdiv) * 0.75 + colormap[d*3 + 1] * 0.2);
						color[2] = (byte)((cd[2] / cdiv) * 0.75 +colormap[d*3 + 2] * 0.2);
					}
					else
					{
						color[0] = colormap[d*3];
						color[1] = colormap[d*3 + 1];
						color[2] = colormap[d*3 + 2];
					}
				}
				else // normal colors
				{
					color[0] = colormap[d*3];
					color[1] = colormap[d*3 + 1];
					color[2] = colormap[d*3 + 2];
				}
				*buf++ = color[0];
				*buf++ = color[1];
				*buf++ = color[2];
				*buf++ = color[3];
				#undef fill
			}
			buf -= chunk->size * 4;

			// get crop borders
			memset(&cropx, 0, sizeof(cropx));
			memset(&cropy, 0, sizeof(cropx));
			cropwidth = chunk->width / 2;
			cropheight = chunk->height / 2;
			// find left crop border
			while(cropx[0] < cropwidth)
			{
				for (r = 0; r < chunk->height; r++)
					if (buf[r*chunk->width*4 + cropx[0]*4 + 3])
						break;
				if (r < chunk->height)
					break;
				cropx[0]++;
			}
			// right crop border
			while(cropx[1] < cropwidth)
			{
				for (r = 0; r < chunk->height; r++)
					if (buf[r*chunk->width*4 + (chunk->width - cropx[1] - 1)*4 + 3])
						break;
				if (r < chunk->height)
					break;
				cropx[1]++;
			}
			// up crop border
			while(cropy[0] < cropheight)
			{
				for (r = 0; r < chunk->width; r++)
					if (buf[cropy[0]*chunk->width*4 + r*4 + 3])
						break;
				if (r < chunk->width)
					break;
				cropy[0]++;
			}
			// down crop border
			while(cropy[1] < cropheight)
			{
				for (r = 0; r < chunk->width; r++)
					if (buf[(chunk->height - cropy[1] - 1)*chunk->width*4 + r*4 + 3])
						break;
				if (r < chunk->width)
					break;
				cropy[1]++;
			}

			// apply 1px margin
			cropx[0] = max(0, cropx[0] - 1);
			cropx[1] = max(0, cropx[1] - 1);
			cropy[0] = max(0, cropy[0] - 1);
			cropy[1] = max(0, cropy[1] - 1);
			addx[0] = abs(min(0, cropx[0] - 1));
			addx[1] = abs(min(0, cropx[1] - 1));
			addy[0] = abs(min(0, cropy[0] - 1));
			addy[1] = abs(min(0, cropy[1] - 1));

			// write header
			cropwidth = chunk->width - cropx[0] - cropx[1];
			cropheight = chunk->height - cropy[0] - cropy[1];
			realwidth = cropwidth + addx[0] + addx[1];
			realheight = cropheight + addy[0] + addy[1];

			// show sprite center
			/*
			printf("chunk origin %i %i %i %i\n", chunk->width, chunk->height, chunk->x + rawblock->posx + cx + cropx[1] - addx[1], chunk->y + rawblock->posy + cy - cropy[0] + addy[0]);
			if (chunk->x <= 0 && abs(chunk->x) < chunk->width && chunk->y >= 0 && chunk->y < chunk->height)
			{
				buf[chunk->width * chunk->y * 4 + abs(chunk->x) * 4]     = 255;
				buf[chunk->width * chunk->y * 4 + abs(chunk->x) * 4 + 1] = 0;
				buf[chunk->width * chunk->y * 4 + abs(chunk->x) * 4 + 2] = 255;
				buf[chunk->width * chunk->y * 4 + abs(chunk->x) * 4 + 3] = 255;
			}
			*/

			//printf(" frame %i cropped from %ix%i to %ix%i (saving %.2f kb)\n", i, chunk->width, chunk->height, cropwidth, cropheight, (float)(chunk->width*chunk->height - cropwidth*cropheight)/256);
			SPR_WriteFrameHeader(f, SPR_FRAME_SINGLE, realwidth, realheight, chunk->x + rawblock->posx + cx + cropx[1] - addx[1], chunk->y + rawblock->posy + cy - cropy[0] + addy[0]);
			
			/*
			// test TGA output
			char *out, *fin, *fout;
			char sn[1024];
			out = (byte *)mem_alloc(cropwidth*cropheight*4);
			for (r = 0; r < cropheight; r++)
			{
				fin = buf + (cropy[0] + r)*chunk->width*4 + cropx[0]*4;
				fout = out + r*cropwidth*4;
				printf("copy: %i\n", fin[1]);
				memcpy(fout, fin, cropwidth * 4);
			}
			sprintf(sn, "%s_%i.tga", outfile, i);
			RawTGA(sn, cropwidth, cropheight, 0, 0, 0, 0, NULL, out, 32, NULL);
			mem_free(out);
			*/

			// write added/cropped pic
			if (addx[0] > 0 || addx[1] > 0 || addy[0] > 0 || addy[1] > 0)
			{
				// VorteX: dumb code!
				byte *e;
				int es;
				es = max( (addx[0] + addx[1]) * 4 , realwidth * max(addy[0], addy[1]) * 4);
				e = (byte *)mem_alloc(es);
				memset(e, 0, es);
				if (addy[0] > 0)
					fwrite(e, realwidth * addy[0] * 4, 1, f);
				for (r = 0; r < cropheight; r++)
				{	
					if (addx[0] > 0)
						fwrite(e, addx[0] * 4, 1, f);
					fwrite(buf + (cropy[0] + r)*chunk->width*4 + cropx[0]*4, cropwidth * 4, 1, f);
					if (addx[1] > 0)
						fwrite(e, addx[1] * 4, 1, f);
				}
				if (addy[1] > 0)
					fwrite(e, realwidth * addy[1] * 4, 1, f);
				mem_free(e);
			}
			else 
			{
				for (r = 0; r < cropheight; r++)
					fwrite(buf + (cropy[0] + r)*chunk->width*4 + cropx[0]*4, cropwidth * 4, 1, f);
			}
			mem_free(buf);
		}
		else
		{
			Error("SPR_WriteFromRawblock: cannot write quake sprites yet\n");
		}
	}

	// write mergedata
	if (tailfiles)
	{
		for (i = 0; i < tailfiles->items; i++)
		{
			fwrite(mergedata[i], mergedatalen[i], 1, f);
			mem_free(mergedata[i]);
		}
	}
	WriteClose(f);
}