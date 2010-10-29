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
	int *data;
	data = (void*) &num; // note: this is *NOT* byte order dependent
	fput_littleint(*data, file);
}

int LittleInt(byte *buffer)
{
	return buffer[3]*16777216 + buffer[2]*65536 + buffer[1]*256 + buffer[0];
}

float LittleFloat (byte *buffer)
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

qboolean SPR_ReadHeader(byte *buf, int bufsize, spr_t *sprheader) 
{
	memset(sprheader, 0, sizeof(spr_t));
	if (buf[0] != 'I' || buf[1] != 'D' || buf[2] != 'S' || buf[3] != 'P')
		return false;
	if (bufsize < sizeof(spr_t))
		return false;
	sprheader->ver1 = LittleInt(buf + 4);
	sprheader->type = LittleInt(buf + 8);
	sprheader->radius = LittleFloat(buf + 12);
	sprheader->maxwidth = LittleInt(buf + 16);
	sprheader->maxheight = LittleInt(buf + 20);
	sprheader->nframes = LittleInt(buf + 24);
	sprheader->beamlength = LittleFloat(buf + 28);
	sprheader->synchtype = LittleInt(buf + 32);
	return true;
}

FILE *SPR_BeginFile(char *outfile, sprversion_t version, sprtype_t type, int maxwidth, int maxheight, int numframes, qboolean mergeintoexistingfile)
{
	spr_t mergespr;
	byte mergedata[36];
	float boundradius;
	FILE *f;

	boundradius = (float) sqrt((maxwidth*0.5)*(maxwidth*0.5)+(maxheight*0.5)*(maxheight*0.5));

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
	fput_littleint(version, f); // 32bit color
	// type
	fput_littleint(type, f); 
	// boundingradius
	fput_littlefloat(boundradius, f); 
	// maxwidth/maxheight
	fput_littleint(maxwidth, f); 
	fput_littleint(maxheight, f);
	// numframes
	fput_littleint(numframes, f);
	// beamlength
	fput_littlefloat(0.0f, f); 
	// synctype
	fput_littleint(0, f);

	// append if merging
	if (mergeintoexistingfile)
		fseek(f, 0, SEEK_END);

	return f;
}

void SPR_WriteFrameHeader(FILE *f, sprframetype_t frametype, int width, int height, int cx, int cy)
{
	fput_littleint(frametype, f);
	fput_littleint(cx, f);
	fput_littleint(cy, f);
	fput_littleint(width, f);
	fput_littleint(height, f);
}

void SPR_WriteFromRawblock(rawblock_t *rawblock, char *outfile, sprversion_t version, sprtype_t type, int cx, int cy, float alpha, byte shadowpixel, byte shadowalpha, int flags, qboolean mergeintoexistingfile)
{
	int i, d, r, w, h, maxwidth, maxheight, cropx[2], cropy[2], addx[2], addy[2], cropwidth, cropheight, realwidth, realheight;
	byte *colormap, *buf, normalalpha, color[4];
	double cdiv, cd[3];
	rawchunk_t *chunk;
	FILE *f;
	
	// calc whole alpha
	normalalpha = (byte)(255 * alpha);
	shadowalpha = (byte)(shadowalpha * alpha);

	// calc max width/height, write header
	maxwidth = maxheight = 0;
	for (i = 0; i < rawblock->chunks; i++)
	{
		maxwidth = max(maxwidth, (rawblock->chunk[i].width + rawblock->chunk[i].x));
		maxheight = max(maxheight, (rawblock->chunk[i].height + rawblock->chunk[i].y));
	}

	// write sprite header
	f = SPR_BeginFile(outfile, version, type, maxwidth, maxheight, rawblock->chunks, mergeintoexistingfile);

	//54
	//printf("Exporting %s: \n", outfile);
	// write frames
	for (i = 0; i < rawblock->chunks; i++)
	{
		chunk = &rawblock->chunk[i];
		colormap = (chunk->colormap != NULL) ? chunk->colormap : rawblock->colormap;
		if (version == SPR_DARKPLACES) // 32bit RGBA8 raw, ready for OpenGL
		{
			// create initial image
			// in Blood Omen, black pixels (0) were transparent
			// also we optionally threating shadow pixel as transparent
			buf = qmalloc(chunk->size * 4);
			for (h = 0; h < chunk->height; h++)
			for (w = 0; w < chunk->width; w++)
			{
				d = chunk->pixels[h*chunk->width + w];
				color[0] = colormap[d*3];
				color[1] = colormap[d*3 + 1];
				color[2] = colormap[d*3 + 2];
				if (shadowpixel >= 0 && d == shadowpixel)
					color[3] = shadowalpha;
				else if (d == 0) // null pixel always transparent
					color[3] = 0;
				else
					color[3] = normalalpha;
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
			// printf(" frame %i cropped from %ix%i to %ix%i (saving %.2f kb)\n", i, chunk->width, chunk->height, cropwidth, cropheight, (float)(chunk->width*chunk->height - cropwidth*cropheight)/256);
			SPR_WriteFrameHeader(f, SPR_FRAME_SINGLE, realwidth, realheight, chunk->x + rawblock->posx + cx + cropx[1] - addx[1], chunk->y + rawblock->posy + cy - cropy[0] - addy[0]);
			
			/*
			// test TGA output
			char *out, *fin, *fout;
			char sn[1024];
			out = qmalloc(cropwidth*cropheight*4);
			for (r = 0; r < cropheight; r++)
			{
				fin = buf + (cropy[0] + r)*chunk->width*4 + cropx[0]*4;
				fout = out + r*cropwidth*4;
				printf("copy: %i\n", fin[1]);
				memcpy(fout, fin, cropwidth * 4);
			}
			sprintf(sn, "%s_%i.tga", outfile, i);
			RawTGA(sn, cropwidth, cropheight, 0, 0, 0, 0, NULL, out, 32, NULL);
			qfree(out);
			*/

			// write added/cropped pic
			if (addx[0] > 0 || addx[1] > 0 || addy[0] > 0 || addy[1] > 0)
			{
				// VorteX: dumb code!
				byte *e;
				int es;
				es = max( (addx[0] + addx[1]) * 4 , realwidth * max(addy[0], addy[1]) * 4);
				e = qmalloc(es);
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
				qfree(e);
			}
			else 
			{
				for (r = 0; r < cropheight; r++)
					fwrite(buf + (cropy[0] + r)*chunk->width*4 + cropx[0]*4, cropwidth * 4, 1, f);
			}
			qfree(buf);
		}
		else
		{
			Error("SPR_WriteFromRawblock: cannot write quake sprites yet\n");
		}
	}
	WriteClose(f);
}

// todo: add proper framegroups
void SPR32_DecompileSprite(char *file)
{
	int bufsize, i, framehead[5], y;
	char scriptfile[MAX_BLOODPATH], framepic[MAX_BLOODPATH], outfolder[MAX_BLOODPATH];
	byte *b, *buf, *buffer, *out, *in, *end;
	spr_t sprheader;
	FILE *f, *f2;

	// open scriptfile
	ExtractFilePath(file, outfolder);
	if (outfolder[0])
		sprintf(scriptfile, "%s/sprinfo.txt", outfolder);
	else
		sprintf(scriptfile, "sprinfo.txt", outfolder);
	printf("open %s...\n", scriptfile);
	f = SafeOpenWrite(scriptfile);

	// load spr header
	bufsize = LoadFile(file, &b);
	buf = b;
	if (!SPR_ReadHeader(buf, bufsize, &sprheader))
		Error("%s: not IDSP file\n", file);
	if (sprheader.ver1 != SPR_DARKPLACES)
		Error("%s: not Darkplaces SPR32 sprite\n", file);
	fprintf(f, "TYPE %i\n", sprheader.type);
	fprintf(f, "SYNCTYPE %i\n", sprheader.synchtype);
	// write frames
	buf+=sizeof(spr_t);
	bufsize-=sizeof(spr_t);
	for (i = 0; i < sprheader.nframes; i++)
	{
		if (bufsize < 20)
			Error("unexpected EOF");
		// read frame header
		framehead[0] = LittleInt(buf);
		framehead[1] = LittleInt(buf + 4);
		framehead[2] = LittleInt(buf + 8);
		framehead[3] = LittleInt(buf + 12);
		framehead[4] = LittleInt(buf + 16);
		buf+=20;
		bufsize-=20;
		// write frame
		fprintf(f, "FRAME %i %i %i\n", framehead[0], framehead[1], framehead[2]);
		if (bufsize < framehead[3]*framehead[4]*4)
			Error("unexpected EOF");
		if (framehead[3]*framehead[4] <= 0)
			Error("bad frame width/height");
		// write tga image
		sprintf(framepic, "frame%04i.tga", i);
		Verbose("writing frame %s\n", framepic);
		f2 = SafeOpenWrite(framepic);
		buffer = qmalloc(framehead[3]*framehead[4]*4 + 18);
		memset(buffer, 0, 18);
		buffer[2] = 2; // uncompressed
		buffer[12] = (framehead[3] >> 0) & 0xFF;
		buffer[13] = (framehead[3] >> 8) & 0xFF;
		buffer[14] = (framehead[4] >> 0) & 0xFF;
		buffer[15] = (framehead[4] >> 8) & 0xFF;
		buffer[16] = 32;
		out = buffer + 18;
		for (y = framehead[4] - 1;y >= 0;y--)
		{
			in = buf + y * framehead[3] * 4;
			end = in + framehead[3] * 4;
			for (;in < end;in += 4)
			{
				*out++ = in[2];
				*out++ = in[1];
				*out++ = in[0];
				*out++ = in[3];
			}
		}
		fwrite(buffer, framehead[3]*framehead[4]*4 + 18, 1, f2);
		qfree(buffer);
		WriteClose(f2);
		// advance
		buf+=framehead[3]*framehead[4]*4;
		bufsize-=framehead[3]*framehead[4]*4;
	}
	WriteClose(f);
	qfree(b);
}

int Spr32_Main(int argc, char **argv)
{
	char infile[MAX_BLOODPATH];
	qboolean delmerged;
	list_t *mergelist;
	int i;

	// in file
	if (argc < 3)
		Error("not enough parms");
	strcpy(infile, argv[1]);
	Verbose("File: %s\n", argv[1]);

	// commandline actions
	delmerged = true;
	mergelist = NewList();
	ListAdd(mergelist, infile, false);
	for(i = 2; i < argc; i++)
	{
		if (!strcmp(argv[i], "-decompile"))
			continue;
		if (!strcmp(argv[i], "-compile"))
			continue;
		if (!strcmp(argv[i], "-merge"))
		{
			i++; 
			while (i < argc && argv[i][0] != '-')
			{
				ListAdd(mergelist, argv[i], false);
				i++; 
			}
			continue;
		}
		if (!strcmp(argv[i], "-keepfiles"))
		{
			delmerged = false;
			Verbose("Option: keeping merged files undeleted\n");
			continue;
		}
		if (i != 0)
			Warning("unknown parameter '%s'",  argv[i]);
	}

	// do action
	if (!strcmp(argv[2], "-decompile"))
		SPR32_DecompileSprite(argv[1]);
	Verbose("Done.\n");
	return 0;
}