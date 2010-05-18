////////////////////////////////////////////////////////////////
//
// Blood Omen RAW files loader
// coded by Pavel [VorteX] Timofeyev and placed to public domain
// LZ77 compression algorithms on CMP/CTM spec by Ben Lincoln
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
#include "bigfile.h"
#include "cmdlib.h"
#include "mem.h"

// raw error messages
char *rawextractresultstrings[13] =
{
	"no errors occured",
	"header not valid",
	"implicit objects count",
	"error reading colormap",
	"implicit Width*Height",
	"file is bigger than required",
	"EOF while reading data",
	"bad object header",
	"bad object offset",
	"decompression run off file size",
	"decompession run of exported picture size",
	"wrong options given to extract",
	"file cannot be parsed as raw image"
};
char rescodestr[512];

char *RawStringForResult(int rescode)
{
	if (rescode > 0)
	{
		sprintf(rescodestr, "%s , endpos is %i", rawextractresultstrings[0], rescode);
		return rescodestr;
	}
	rescode = rescode * -1;
	return rawextractresultstrings[rescode];
}

void printfpos(FILE *f)
{
	fpos_t fpos;

	fgetpos(f, &fpos);
	Verbose("filepos: %i\n", fpos);
}

int getfpos(FILE *f)
{
	fpos_t fpos;

	fgetpos(f, &fpos);
	return (int)fpos;
}

void FlushRawInfo(rawinfo_t *rawinfo)
{
	rawinfo->type = RAW_TYPE_UNKNOWN;
	rawinfo->width = -1;
	rawinfo->height = -1;
	rawinfo->offset = 0;
	rawinfo->bytes = 1;
	rawinfo->doubleres = rauto;
	rawinfo->disableCLUT = false;
	rawinfo->dontSwapBgr = false;
	rawinfo->colormapoffset = 0;
	rawinfo->colormapbytes = 0;
	rawinfo->chunknum = -1;
	rawinfo->usecompression = false;
	rawinfo->compressionpixels[0] = 0;
	rawinfo->compressionpixels[1] = 0;
	rawinfo->compressionpixels[2] = 0;
	rawinfo->compressionpixels[3] = 0;
}

rawinfo_t *NewRawInfo()
{
	rawinfo_t *rawinfo;

	rawinfo = qmalloc(sizeof(rawinfo_t));
	FlushRawInfo(rawinfo);

	return rawinfo;
}

qboolean ReadRawInfo(char *line, rawinfo_t *rawinfo)
{
	char temp[256];
	int num;

	if (rawinfo == NULL)
		return false;

	if (sscanf(line, "raw.type=%s", &temp))
		rawinfo->type = ParseRawType(temp);
	else if (sscanf(line, "raw.width=%i", &num))
		rawinfo->width = num;
	else if (sscanf(line, "raw.height=%i", &num))
		rawinfo->height = num;
	else if (sscanf(line, "raw.offset=%i", &num))
		rawinfo->offset = num;
	else if (sscanf(line, "raw.bytes=%i", &num))
		rawinfo->bytes = num;
	else if (sscanf(line, "raw.doubleres=%s", &temp))
		rawinfo->doubleres = ParseRawSwitch(temp);
	else if (sscanf(line, "raw.chunknum=%s", &num))
		rawinfo->chunknum = num;
	else if (sscanf(line, "raw.colormapoffset=%i", &num))
		rawinfo->colormapoffset = num;
	else if (sscanf(line, "raw.colormapbytes=%i", &num))
		rawinfo->colormapbytes = num;
	else if (sscanf(line, "raw.disableCLUT"))
		rawinfo->disableCLUT = true;
	else if (sscanf(line, "raw.dontSwapBgr"))
		rawinfo->dontSwapBgr = true;
	else
		return false;
	
	return true;
}

void WriteRawInfo(FILE *f, rawinfo_t *rawinfo)
{
	fprintf(f, "raw.type=%s\n", UnparseRawType(rawinfo->type));
	if (rawinfo->width >= 0)
		fprintf(f, "raw.width=%i\n", rawinfo->width);
	if (rawinfo->height >= 0)
		fprintf(f, "raw.height=%i\n", rawinfo->height);
	if (rawinfo->offset > 0)
		fprintf(f, "raw.offset=%i\n", rawinfo->offset);
	if (rawinfo->bytes > 1)
		fprintf(f, "raw.bytes=%i\n", rawinfo->bytes);
	if (rawinfo->doubleres > 0)
		fprintf(f, "raw.doubleres=%s\n", UnparseRawSwitch(rawinfo->doubleres));
	if (rawinfo->chunknum >= 0)
		fprintf(f, "raw.chunknum=%i\n", rawinfo->chunknum);
	if (rawinfo->colormapoffset > 0)
		fprintf(f, "raw.colormapoffset=%i\n", rawinfo->colormapoffset);
	if (rawinfo->colormapbytes > 0)
		fprintf(f, "raw.colormapbytes=%i\n", rawinfo->colormapbytes);
	if (rawinfo->disableCLUT == true)
		fprintf(f, "raw.disableCLUT\n");
	if (rawinfo->dontSwapBgr == true)
		fprintf(f, "raw.dontSwapBgr\n");
}

rawswitch_t ParseRawSwitch(char *str)
{
	Q_strlower(str);
	if (!strcmp(str, "auto") || !strcmp(str, "automatic") || !strcmp(str, "a"))
		return rauto;
	if (!strcmp(str, "true") || !strcmp(str, "rtrue") || !strcmp(str, "yes") || !strcmp(str, "on") || !strcmp(str, "1"))
		return rtrue;
	return rfalse;
}

char *UnparseRawSwitch(rawswitch_t rawswitch)
{
	if (rawswitch == rauto)
		return "auto";
	if (rawswitch == rtrue)
		return "yes";
	return "no";
}

rawtype_t ParseRawType(char *str)
{
	Q_strlower(str);
	if (!strcmp(str, "type0") || !strcmp(str, "0") || !strcmp(str, "raw"))
		return RAW_TYPE_0;
	if (!strcmp(str, "type1") || !strcmp(str, "1"))
		return RAW_TYPE_1;
	if (!strcmp(str, "type2") || !strcmp(str, "2"))
		return RAW_TYPE_2;
	if (!strcmp(str, "type3") || !strcmp(str, "3"))
		return RAW_TYPE_3;
	if (!strcmp(str, "type4") || !strcmp(str, "4"))
		return RAW_TYPE_4;
	if (!strcmp(str, "type5") || !strcmp(str, "5"))
		return RAW_TYPE_5;
	if (!strcmp(str, "type6") || !strcmp(str, "6"))
		return RAW_TYPE_6;
	if (!strcmp(str, "type7") || !strcmp(str, "7"))
		return RAW_TYPE_7;
	if (!strcmp(str, "type8") || !strcmp(str, "8"))
		return RAW_TYPE_8;
	return RAW_TYPE_UNKNOWN;
}

char *UnparseRawType(rawtype_t rawtype)
{
	if (rawtype == RAW_TYPE_0)
		return "type0";
	if (rawtype == RAW_TYPE_1)
		return "type1";
	if (rawtype == RAW_TYPE_2)
		return "type2";
	if (rawtype == RAW_TYPE_3)
		return "type3";
	if (rawtype == RAW_TYPE_4)
		return "type4";
	if (rawtype == RAW_TYPE_5)
		return "type5";
	if (rawtype == RAW_TYPE_6)
		return "type6";
	if (rawtype == RAW_TYPE_7)
		return "type7";
	if (rawtype == RAW_TYPE_8)
		return "type8";
	return "unknown";
}

char *PathForRawType(rawtype_t rawtype)
{
	if (rawtype == RAW_TYPE_1)
		return "item/";
	if (rawtype == RAW_TYPE_2)
		return "graphics/";
	if (rawtype == RAW_TYPE_3 || rawtype == RAW_TYPE_5 || rawtype == RAW_TYPE_4)
		return "sprite/";
	if (rawtype == RAW_TYPE_7)
		return "tile/";
	return NULL;
}

/*
==========================================================================================

  RAW BLOCKS WORKER

  raw block = a transport entity between scanner and exporter
  i.e. we will read all raw images to common structure and ther pass it ti actual exporter

==========================================================================================
*/

rawblock_t *EmptyRawBlock(int numchunks)
{
	rawblock_t *block;
	int i;

	block = qmalloc(sizeof(rawblock_t));
	memset(block, 0, sizeof(rawblock_t));
	block->chunks = numchunks;
	block->colormap = NULL;
	block->colormapExternal = false;
	block->alphamap = NULL;
	block->alphamapExternal = false;
	if (numchunks > MAX_RAW_CHUNKS)
		Error("MAX_RAW_CHUNKS exceeded\n");
	if (numchunks > 0)
	{
		// nullify chunk data
		for (i = 0; i < numchunks; i++)
			memset(&block->chunk[i], 0, sizeof(rawchunk_t));
	}
	return block;
}

rawblock_t *RawErrorBlock(rawblock_t *block, rawextractresult_t errorcode)
{
	if (block == NULL)
		block = EmptyRawBlock(0);
	block->errorcode = errorcode;
	return block;
}

void RawBlockAllocateChunkSimple(rawblock_t *block, int chunknum, qboolean pixelsExternal)
{
	if (chunknum < 0 || chunknum >= block->chunks)
		Error("RawBlockAllocateChunk: bad chunk number %i", chunknum);
	if (block->chunk[chunknum].pixels != NULL)
		Error("RawBlockAllocateChunk: chunk %i already allocated", chunknum);

	block->chunk[chunknum].pixels = NULL;
	if (pixelsExternal == false)
	{
		block->chunk[chunknum].pixels = qmalloc(block->chunk[chunknum].width*block->chunk[chunknum].height);
		memset(block->chunk[chunknum].pixels, 0, block->chunk[chunknum].width*block->chunk[chunknum].height);
	}
	block->chunk[chunknum].colormap = NULL;
	block->chunk[chunknum].alphamap = NULL;
	block->chunk[chunknum].pixelsExternal = pixelsExternal;
}

void RawBlockAllocateChunk(rawblock_t *block, int chunknum, int width, int height, int x, int y, qboolean pixelsExternal)
{
	block->chunk[chunknum].width = width;
	block->chunk[chunknum].height = height;
	block->chunk[chunknum].size = width*height;
	block->chunk[chunknum].x = x;
	block->chunk[chunknum].y = y;
	RawBlockAllocateChunkSimple(block, chunknum, pixelsExternal);
}

void RawBlockFreeChunk(rawblock_t *block, int chunknum)
{
	if (block->chunk[chunknum].colormap != NULL && block->chunk[chunknum].colormapExternal == false)
		qfree(block->chunk[chunknum].colormap);
	if (block->chunk[chunknum].alphamap != NULL && block->chunk[chunknum].alphamapExternal == false)
		qfree(block->chunk[chunknum].alphamap);

	block->chunk[chunknum].colormap = NULL;
	block->chunk[chunknum].colormapExternal = false;
	block->chunk[chunknum].alphamap = NULL;
	block->chunk[chunknum].alphamapExternal = false;
	if (block->chunk[chunknum].pixelsExternal == false)
		qfree(block->chunk[chunknum].pixels);
	block->chunk[chunknum].pixels = NULL;
	block->chunk[chunknum].pixelsExternal = false;
}

void FreeRawBlock(rawblock_t *block)
{
	int i;

	if (block->colormap != NULL && block->colormapExternal == false)
		qfree(block->colormap);
	if (block->alphamap != NULL && block->alphamapExternal == false)
		qfree(block->alphamap);
	for (i = 0; i < block->chunks; i++)
		RawBlockFreeChunk(block, i);
	qfree(block);
}

// flip all chunks in rawblock, do not return new rawblock but update current
void RawblockFlip(rawblock_t *rawblock, qboolean flipoffset)
{
	rawchunk_t *chunk;
	int i, start, end, p, w;
	byte pixel;

	// all chunks..
	if (flipoffset)
		rawblock->posx = 0 - rawblock->posx;
	for (i = 0; i < rawblock->chunks; i++)
	{
		chunk = &rawblock->chunk[i];
		if (flipoffset)
			chunk->x = (0 - chunk->x - chunk->width);
		// all lines
		w = (int)chunk->width/2;
		for (start = 0; start < chunk->size; start += chunk->width)
		{
			end = start + chunk->width - 1;
			// flip line
			for (p = 0; p < w; p++)
			{
				pixel = chunk->pixels[start + p];
				chunk->pixels[start + p] = chunk->pixels[end - p];
				chunk->pixels[end - p] = pixel;
			}
		}
	}
}

// crop black pixels on raw blocl
// returns completely new rawblock (only colormap are derived)
rawblock_t *RawblockCrop(rawblock_t *rawblock, qboolean cropeachchunk, int margin)
{
	int i, r, cropx[2], cropy[2], mincropx[2], mincropy[2], halfwidth, halfheight;
	rawblock_t *cropblock;
	byte *buf, *srcbuf, ind;
	
	// create new block
	cropblock = EmptyRawBlock(rawblock->chunks);
	cropblock->colormap = rawblock->colormap;
	cropblock->colormapExternal = true;
	cropblock->alphamap = rawblock->alphamap;
	cropblock->alphamapExternal = true;
	cropblock->posx = rawblock->posx;
	cropblock->posy = rawblock->posy;

	// if not cropping each chunk - find minimal/maximal crops
	if (!cropeachchunk)
	{
		mincropx[0] = 100000000;
		mincropx[1] = 100000000;
		mincropy[0] = 100000000;
		mincropy[1] = 100000000;
		for (i = 0; i < rawblock->chunks; i++)
		{
			memset(&cropx, 0, sizeof(cropx));
			memset(&cropy, 0, sizeof(cropx));
			halfwidth = (int)(rawblock->chunk[i].width / 2);
			halfheight = (int)(rawblock->chunk[i].height / 2);
			// find left crop border
			while(cropx[0] < halfwidth)
			{
				for (r = 0; r < rawblock->chunk[i].height; r++)
				{
					ind = rawblock->chunk[i].pixels[r*rawblock->chunk[i].width + cropx[0]];
					if (ind != 0)
						break;
				}
				if (r < rawblock->chunk[i].height)
					break;
				cropx[0]++;
			}
			// right crop border
			while(cropx[1] < halfwidth)
			{
				for (r = 0; r < rawblock->chunk[i].height; r++)
				{
					ind = rawblock->chunk[i].pixels[r*rawblock->chunk[i].width + (rawblock->chunk[i].width - cropx[1] - 1)];
					if (ind != 0)
						break;
				}
				if (r < rawblock->chunk[i].height)
					break;
				cropx[1]++;
			}
			// up crop border
			while(cropy[0] < halfheight)
			{
				for (r = 0; r < rawblock->chunk[i].width; r++)
				{
					ind = rawblock->chunk[i].pixels[cropy[0]*rawblock->chunk[i].width + r];
					if (ind != 0)
						break;
				}
				if (r < rawblock->chunk[i].width)
					break;
				cropy[0]++;
			}
			// down crop border
			while(cropy[1] < halfheight)
			{
				for (r = 0; r < rawblock->chunk[i].width; r++)
				{
					ind = rawblock->chunk[i].pixels[(rawblock->chunk[i].height - cropy[1] - 1)*rawblock->chunk[i].width + r];
					if (ind != 0)
						break;
				}
				if (r < rawblock->chunk[i].width)
					break;
				cropy[1]++;
			}
			// get minimap crop
			mincropx[0] = min(cropx[0], mincropx[0]);
			mincropx[1] = min(cropx[1], mincropx[1]);
			mincropy[0] = min(cropy[0], mincropy[0]);
			mincropy[1] = min(cropy[1], mincropy[1]);
		}
	}

	// crop each chunk
	for (i = 0; i < rawblock->chunks; i++)
	{
		if (!cropeachchunk)
		{
			// apply margin
			cropx[0] = max(0, mincropx[0] - margin);
			cropx[1] = max(0, mincropx[1] - margin);
			cropy[0] = max(0, mincropy[0] - margin);
			cropy[1] = max(0, mincropy[1] - margin);
		}
		else
		{
			memset(&cropx, 0, sizeof(cropx));
			memset(&cropy, 0, sizeof(cropx));
			halfwidth = (int)(rawblock->chunk[i].width / 2);
			halfheight = (int)(rawblock->chunk[i].height / 2);
			// find left crop border
			while(cropx[0] < halfwidth)
			{
				for (r = 0; r < rawblock->chunk[i].height; r++)
				{
					ind = rawblock->chunk[i].pixels[r*rawblock->chunk[i].width + cropx[0]];
					if (ind != 0)
						break;
				}
				if (r < rawblock->chunk[i].height)
					break;
				cropx[0]++;
			}
			// right crop border
			while(cropx[1] < halfwidth)
			{
				for (r = 0; r < rawblock->chunk[i].height; r++)
				{
					ind = rawblock->chunk[i].pixels[r*rawblock->chunk[i].width + (rawblock->chunk[i].width - cropx[1] - 1)];
					if (ind != 0)
						break;
				}
				if (r < rawblock->chunk[i].height)
					break;
				cropx[1]++;
			}
			// up crop border
			while(cropy[0] < halfheight)
			{
				for (r = 0; r < rawblock->chunk[i].width; r++)
				{
					ind = rawblock->chunk[i].pixels[cropy[0]*rawblock->chunk[i].width + r];
					if (ind != 0)
						break;
				}
				if (r < rawblock->chunk[i].width)
					break;
				cropy[0]++;
			}
			// down crop border
			while(cropy[1] < halfheight)
			{
				for (r = 0; r < rawblock->chunk[i].width; r++)
				{
					ind = rawblock->chunk[i].pixels[(rawblock->chunk[i].height - cropy[0] - 1)*rawblock->chunk[i].width + r];
					if (ind != 0)
						break;
				}
				if (r < rawblock->chunk[i].width)
					break;
				cropy[1]++;
			}
			// apply margin
			cropx[0] = max(0, cropx[0] - margin);
			cropx[1] = max(0, cropx[1] - margin);
			cropy[0] = max(0, cropy[0] - margin);
			cropy[1] = max(0, cropy[1] - margin);
		}
		// create cropped image, copy lines
		RawBlockAllocateChunk(cropblock, i, rawblock->chunk[i].width - cropx[0] - cropx[1], rawblock->chunk[i].height - cropy[0] - cropy[1], 0, 0, false);
		cropblock->chunk[i].colormap = rawblock->chunk[i].colormap;
		cropblock->chunk[i].colormapExternal = true;
		cropblock->chunk[i].alphamap = rawblock->chunk[i].alphamap;
		cropblock->chunk[i].alphamapExternal = true;
		for (r = 0; r < cropblock->chunk[i].height; r++)
		{
			buf = cropblock->chunk[i].pixels + r*cropblock->chunk[i].width;
			srcbuf = rawblock->chunk[i].pixels + (r + cropy[0])*rawblock->chunk[i].width + cropx[0];
			memcpy(buf, srcbuf, cropblock->chunk[i].width);
		}
	}

	return cropblock;
}

// align all blocks in rawblock to same size
// returns completely new rawblock (only colormap are derived)
rawblock_t *RawblockAlign(rawblock_t *rawblock, int margin)
{
	int maxwidth, maxheight, i, c, ax, ay, bx, by;
	rawblock_t *newblock;
	byte *buf, *srcbuf;

	// detect maxwidth/maxheight
	maxwidth = maxheight = 0;
	for (i = 0; i < rawblock->chunks; i++)
	{
		maxwidth = max(maxwidth, (rawblock->chunk[i].width + rawblock->chunk[i].x));
		maxheight = max(maxheight, (rawblock->chunk[i].height + rawblock->chunk[i].y));
	}
	maxwidth = maxwidth + margin*2;
	maxheight = maxheight + margin*2;

	// create rawblock
	newblock = EmptyRawBlock(rawblock->chunks);
	newblock->colormap = rawblock->colormap;
	newblock->colormapExternal = true;
	newblock->alphamap = rawblock->alphamap;
	newblock->alphamapExternal = true;

	// align chunks
	for (i = 0; i < rawblock->chunks; i++)
	{
		// FIXME: allow negative offset
		bx = rawblock->chunk[i].x + margin;
		by = rawblock->chunk[i].y + margin;
		ax = max(0, maxwidth - rawblock->chunk[i].width - bx);
		ay = max(0, maxheight - rawblock->chunk[i].height - by);
		RawBlockAllocateChunk(newblock, i, maxwidth, maxheight, 0, 0, false);
		newblock->chunk[i].colormap = rawblock->chunk[i].colormap;
		newblock->chunk[i].colormapExternal = true;
		newblock->chunk[i].alphamap = rawblock->chunk[i].alphamap;
		newblock->chunk[i].alphamapExternal = true;
		// write before-lines
		buf = newblock->chunk[i].pixels;
		c = by*maxwidth;
		memset(buf, 0, c);
		buf += c;
		// write center
		for(c = 0; c < rawblock->chunk[i].height; c++)
		{
			// write before=rows
			memset(buf, 0, bx);
			buf += bx;
			// write pixels
			srcbuf = rawblock->chunk[i].pixels + c*rawblock->chunk[i].width;
			memcpy(buf, srcbuf, rawblock->chunk[i].width);
			buf += rawblock->chunk[i].width;
			// write after-rows
			memset(buf, 0, ax);
			buf += ax;
		}
		// write after-lines
		c = ay*maxwidth;
		memset(buf, 0, c);
		buf += c;
	}

	return newblock;
}

// returns new "sliced" rawblock using includelist
// includelist is order-dependent, same chunks could be added twice and more
// note that all pixeldata will be external, so after base rawblock get freed this rawblock will be invalid
// also all changes on this rawblock will affect pixels of base one
rawblock_t *RawblockPerturbate(rawblock_t *rawblock, list_t *includelist)
{
	char *buf;
	rawblock_t *newblock;
	int i, c, start, end, numchunks;

	// first pass - query number of records to add
	numchunks = 0;
	for (i = 0; i < includelist->items; i++)
	{
		buf = includelist->item[i];
		// start-end
		if (!sscanf(buf, "%i-%i", &start, &end))
			start = end = atoi(buf);
		// bound start/end
		if (start < 0)
			start = 0;
		if (end >= rawblock->chunks)
			end = rawblock->chunks - 1;
		if (start > end)
			continue;
		// increase number of chunks
		numchunks = numchunks + (end - start) + 1;
	}

	// second pass - make rawblock
	newblock = EmptyRawBlock(numchunks);
	newblock->colormap = rawblock->colormap;
	newblock->colormapExternal = true;
	newblock->alphamap = rawblock->alphamap;
	newblock->alphamapExternal = true;
	newblock->posx = rawblock->posx;
	newblock->posy = rawblock->posy;
	for (i = 0, c = 0; i < includelist->items && c < numchunks; i++)
	{
		buf = includelist->item[i];
		// start-end
		if (!sscanf(buf, "%i-%i", &start, &end))
			start = end = atoi(buf);
		// bound start/end
		if (start < 0)
			start = 0;
		if (end >= rawblock->chunks)
			end = rawblock->chunks - 1;
		if (start > end)
			continue;
		// add range to rawblock
		while(start <= end)
		{
			if (c >= numchunks) // should never happen
			{
				Warning("RawblockSlice: chunk index overflow!");
				break;
			}
			// copy chunk
			newblock->chunk[c].colormap = rawblock->chunk[start].colormap;
			newblock->chunk[c].colormapExternal = true;
			newblock->chunk[c].alphamap = rawblock->chunk[start].alphamap;
			newblock->chunk[c].alphamapExternal = true;
			newblock->chunk[c].flagbit = rawblock->chunk[start].flagbit;
			newblock->chunk[c].width = rawblock->chunk[start].width;
			newblock->chunk[c].height = rawblock->chunk[start].height;
			newblock->chunk[c].size = rawblock->chunk[start].size;
			newblock->chunk[c].offset = rawblock->chunk[start].offset;
			newblock->chunk[c].pixels = rawblock->chunk[start].pixels;
			newblock->chunk[c].pixelsExternal = true;
			newblock->chunk[c].x = rawblock->chunk[start].x;
			newblock->chunk[c].y = rawblock->chunk[start].y;

			c++;
			start++;
		}
	}

	return newblock;
}

// scale all rawblock images by a factor of 2 using nearest scaling
// this alters entire rawblock
rawblock_t *RawblockScale2x_Nearest(rawblock_t *rawblock)
{
	rawchunk_t *chunk, *newchunk;
	rawblock_t *newrawblock;
	byte *out;
	int i, r, c;

	newrawblock = EmptyRawBlock(rawblock->chunks);
	newrawblock->colormap = rawblock->colormap;
	newrawblock->colormapExternal = true;
	newrawblock->alphamap = rawblock->alphamap;
	newrawblock->alphamapExternal = true;
	newrawblock->posx = rawblock->posx * 2;
	newrawblock->posy = rawblock->posy * 2;

	for (i = 0; i < rawblock->chunks; i++)
	{
		chunk = &rawblock->chunk[i];
		RawBlockAllocateChunk(newrawblock, i, chunk->width*2, chunk->height*2, chunk->x*2, chunk->y*2, false);
		newrawblock->chunk[i].colormap = rawblock->chunk[i].colormap;
		newrawblock->chunk[i].colormapExternal = true;
		newrawblock->chunk[i].alphamap = rawblock->chunk[i].alphamap;
		newrawblock->chunk[i].alphamapExternal = true;
		newchunk = &newrawblock->chunk[i];
		// simple 2x nearest scale
		// fixme: optimize
		out = newchunk->pixels;
		for (r = 0; r < chunk->height; r++)
		{
			for (c = 0; c < chunk->width; c++)
			{
				*out++ = chunk->pixels[r*chunk->width + c];
				*out++ = chunk->pixels[r*chunk->width + c];
			}
			for (c = 0; c < chunk->width; c++)
			{
				*out++ = chunk->pixels[r*chunk->width + c];
				*out++ = chunk->pixels[r*chunk->width + c];
			}
		}
	}
	return newrawblock;
}

/*
==========================================================================================

  RAW FILE TGA EXPORTER

==========================================================================================
*/

void RawTGA(char *outfile, int width, int height, int bx, int by, int ax, int ay, const char *colormapdata, const char *pixeldata, int bpp, rawinfo_t *rawinfo)
{
	unsigned char *buffer, *out;
	const unsigned char *in, *end;
	int i, j, pixelbytes, realwidth, realheight;
	FILE *f;

	// get real width and height
	realwidth = width + bx + ax;
	realheight = height + by + ay;
	pixelbytes = realwidth*realheight;
	// check bpp
	if (bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)
		Error("RawTGA: bad bpp (only 8, 16, 24 and 32 are supported)!\n");

	// lineskippers
	#define skiplines1(lines) for (i = 0; i < lines; i++) for (j = 0; j < realwidth; j++) { *out++ = 0; }
	#define skiplines2(lines) for (i = 0; i < lines; i++) for (j = 0; j < realwidth; j++) { *out++ = 0; *out++ = 0; } 
	#define skiplines3(lines) for (i = 0; i < lines; i++) for (j = 0; j < realwidth; j++) { *out++ = 0; *out++ = 0; *out++ = 0; }
	#define skiprows1(rows) for (j = 0; j < rows; j++) { *out++ = 0; }
	#define skiprows2(rows) for (j = 0; j < rows; j++) { *out++ = 0; *out++ = 0; }
	#define skiprows3(rows) for (j = 0; j < rows; j++) { *out++ = 0; *out++ = 0; *out++ = 0; }

	// create targa header
	buffer = qmalloc(pixelbytes*(int)(bpp / 8) + ((bpp == 8) ? 768 : 0) + 18);
	memset(buffer, 0, 18);
	f = SafeOpenWrite(outfile);
	if (bpp == 8)
	{
		buffer[1] = 1;
		buffer[2] = 1;
		buffer[5] = (256 >> 0) & 0xFF;
		buffer[6] = (256 >> 8) & 0xFF;
		buffer[7] = 24; // colormap BPP
		buffer[12] = (realwidth >> 0) & 0xFF;
		buffer[13] = (realwidth >> 8) & 0xFF;
		buffer[14] = (realheight >> 0) & 0xFF;
		buffer[15] = (realheight >> 8) & 0xFF;
		buffer[16] = 8;
		// 24-bit colormap, swap bgr->rgb
		if ((rawinfo && rawinfo->disableCLUT == true) || colormapdata == NULL)
		{
			out = buffer + 18;
			for (i = 0;i < 256;i++)
			{
				*out++ = i;
				*out++ = i;
				*out++ = i;
			}
		}
		else
		{
			out = buffer + 18;
			for (i = 0;i < 256;i++)
			{
				in = colormapdata + i*3;
				if (rawinfo && rawinfo->dontSwapBgr == true)
				{
					*out++ = in[0];
					*out++ = in[1];
					*out++ = in[2];
				}
				else
				{
					*out++ = in[2];
					*out++ = in[1];
					*out++ = in[0];
				}
			}
		}
		// flip upside down, write
		out = buffer + 768 + 18;
		skiplines1(ay)
		for (i = height - 1;i >= 0;i--)
		{
			skiprows1(bx)
			in = pixeldata + i * width;
			end = in + width;
			for (;in < end; in++)
				*out++ = in[0];
			skiprows1(ax)
		}
		skiplines1(by)
		fwrite(buffer, pixelbytes + 768 + 18, 1, f);
	}
	else if (bpp == 16)
	{
		buffer[2] = 2; // uncompressed
		buffer[12] = (realwidth >> 0) & 0xFF;
		buffer[13] = (realwidth >> 8) & 0xFF;
		buffer[14] = (realheight >> 0) & 0xFF;
		buffer[15] = (realheight >> 8) & 0xFF;
		buffer[16] = 16;
		// flip upside down, write
		out = buffer + 18;
		skiplines2(ay)
		for (i = height - 1;i >= 0;i--)
		{
			skiprows2(bx)
			in = pixeldata + i * width * 2;
			end = in + width * 2;
			for (;in < end; in += 2)
			{
				// swap bgr->rgb
				if (rawinfo && rawinfo->dontSwapBgr == true)
				{
					*out++ = in[0]; 
					*out++ = in[1];
				}
				else
				{
					*out++ = (in[0] & 0xE0) + ((in[1] & 0x7C) >> 2); 
					*out++ = (in[1] & 0x03) + ((in[0] & 0x1F) << 2);
				}
			}
			skiprows2(ax)
		}
		skiplines2(by)
		fwrite(buffer, pixelbytes*2 + 18, 1, f);
	}
	else if (bpp == 24)
	{
		buffer[2] = 2; // uncompressed
		buffer[12] = (realwidth >> 0) & 0xFF;
		buffer[13] = (realwidth >> 8) & 0xFF;
		buffer[14] = (realheight >> 0) & 0xFF;
		buffer[15] = (realheight >> 8) & 0xFF;
		buffer[16] = 24;
		// flip upside down, write
		out = buffer + 18;
		skiplines3(ay)
		for (i = height - 1;i >= 0;i--)
		{
			skiprows3(bx)
			in = pixeldata + i * width * 3;
			end = in + width * 3;
			for (;in < end; in += 3)
			{
				// swap bgr->rgb
				if (rawinfo && rawinfo->dontSwapBgr == true)
				{
					*out++ = in[0];
					*out++ = in[1];
					*out++ = in[2];
				}
				else
				{
					*out++ = in[2];
					*out++ = in[1];
					*out++ = in[0];
				}
			}
			skiprows3(ax)
		}
		skiplines3(by)
		fwrite(buffer, pixelbytes*3 + 18, 1, f);
	}
	else if (bpp == 32)
	{
		buffer[2] = 2; // uncompressed
		buffer[12] = (realwidth >> 0) & 0xFF;
		buffer[13] = (realwidth >> 8) & 0xFF;
		buffer[14] = (realheight >> 0) & 0xFF;
		buffer[15] = (realheight >> 8) & 0xFF;
		buffer[16] = 32;
		// flip upside down, write
		out = buffer + 18;
		skiplines3(ay)
		for (i = height - 1;i >= 0;i--)
		{
			skiprows3(bx)
			in = pixeldata + i * width * 4;
			end = in + width * 4;
			for (;in < end; in += 4)
			{
				// swap bgr->rgb
				if (rawinfo && rawinfo->dontSwapBgr == true)
				{
					*out++ = in[0];
					*out++ = in[1];
					*out++ = in[2];
					*out++ = in[3];
				}
				else
				{
					*out++ = in[2];
					*out++ = in[1];
					*out++ = in[0];
					*out++ = in[3];
				}
			}
			skiprows3(ax)
		}
		skiplines3(by)
		fwrite(buffer, pixelbytes*4 + 18, 1, f);
	}
	fclose(f);
	qfree(buffer);

	#undef skiplines1
	#undef skiplines2
	#undef skiplines3
	#undef skiprows1
	#undef skiprows2
	#undef skiprows3
}

void RawTGAColormap(char *outfile, const byte *colormapdata, byte bytes, int width, int height)
{
	rawinfo_t rawinfo;

	if (width*height != 256)
		Error("RawTGAColormap: bad width/height!");
	
	FlushRawInfo(&rawinfo);
	RawTGA(outfile, width, height, 0, 0, 0, 0, NULL, colormapdata, bytes, &rawinfo);
}

/*
==========================================================================================

  RAW UTIL FUNCTIONS

  common formulas that are used on Blood Omen raw formats

==========================================================================================
*/

// round to nearest structure size
int RoundStruct(int size)
{
	int i;

	i = (int)(size / 4);
	i = i * 4;
	if (i < size)
		return i + 4;
	return i;
}

// read colormap out of stream
byte *ReadColormap(unsigned char *buffer, int filelen, int offset, int palbytes)
{
	byte *colormap;
	int i;

	// apply offset
	filelen = filelen - offset;
	if (filelen < 0)
		filelen = 0;

	palbytes = (palbytes == 3) ? 768 : 512;
	if (filelen < (palbytes + offset))
		return NULL;

	// read colormapdata
	colormap = qmalloc(palbytes);
	memset(colormap, 0, palbytes);
	if (palbytes == 768)
	{
		for (i = 0;i < 256;i++)
		{
			colormap[i*3] = buffer[offset + i*3];
			colormap[i*3 + 1] = buffer[offset + i*3 + 1];
			colormap[i*3 + 2] = buffer[offset + i*3 + 2];
		}
	}
	else
	{
		for (i = 0;i < 256;i++)
		{
			colormap[i*3] = (buffer[offset + i*2] & 0x1F) * 8; 
			colormap[i*3 + 1] = (((buffer[offset + i*2] & 0xE0) >> 5) + ((buffer[offset + i*2 + 1] & 0x3) << 3)) * 8;
			colormap[i*3 + 2] = ((buffer[offset + i*2 + 1] & 0x7C) >> 2) * 8;
		}
	}
	return colormap;
}

// read colormap from external tga file
// returns allocated 24-bit colormap
void ColormapFromTGA(char *filename, byte *colormap)
{
	byte *buf;
	FILE *f;
	int i;
	
	f = SafeOpen(filename, "rb");
	buf = qmalloc(18+768);
	if (fread(buf, 18+768, 1, f) < 1)
		Error("ColormapFromTGA: %s - file is too small", filename);
	if (!LoadFile(filename, &buf))
		Error("ColormapFromTGA: failed to open %s", filename);
	if (buf[1] != 1 || buf[2] != 1 || buf[16] != 8)
		Error("ColormapFromTGA: %s - only colormapped/uncompressed images supported", filename);
	if (buf[7] != 24)
		Error("ColormapFromTGA: %s - only 24-bit colormaps supported (found %i)", filename, buf[7]);
	// read colormal
	for (i = 0; i < 256; i++)
	{
		*colormap++ = buf[18 + i*3 + 2];
		*colormap++ = buf[18 + i*3 + 1];
		*colormap++ = buf[18 + i*3];
	}
	qfree(buf);
	fclose(f);
}

// read run-lenght encoded stream, return startpos, error codes are < 0
int ReadRLCompressedStream(byte *outbuf, byte *inbuf, int startpos, int buflen, int readpixels, qboolean decomress255, qboolean usehalfwidthcompression, qboolean forced)
{
	int pixelpos, nullpixels;
	byte pixel, nullpixelsi;

	#define readpixel() if (startpos >= buflen) { return RAWX_ERROR_COMPRESSED_READ_OVERFLOW; } else { pixel = inbuf[startpos]; startpos++; }
	#define writepixel(f) if (pixelpos >= readpixels) {  if (!forced) return RAWX_ERROR_COMPRESSED_UNPACK_OVERFLOW; } else { if (usehalfwidthcompression) { outbuf[pixelpos] = f - (int)(f/16)*16; pixelpos++; outbuf[pixelpos] = (int)(f/16); } else outbuf[pixelpos] = f; pixelpos++; }

	if (readpixels == 0)
	{
		startpos++;
		return startpos;
	}

	// read stream
	nullpixels = 0;
	for(pixelpos = 0; pixelpos < readpixels; )
	{
		// fill with nulls
		while(nullpixels > 0)
		{
			if (pixelpos >= readpixels)
				break;
			writepixel(nullpixelsi); 
			nullpixels--;
		}
		if (pixelpos >= readpixels)
			break;
		// read pixel
		readpixel();
		if (!pixel || (decomress255 && pixel == 255))
		{	
			nullpixelsi = pixel;
			readpixel();
			nullpixels = pixel;
			continue;
		}
		writepixel(pixel);
	}
	return startpos;
	#undef readpixel
	#undef writepixel
}

// same as above, test-only, no writes
int ReadRLCompressedStreamTest(byte *outbuf, byte *inbuf, int startpos, int buflen, int readpixels, qboolean decomress255, qboolean usehalfwidthcompression, qboolean forced)
{
	int pixelpos, nullpixels;
	byte pixel, nullpixelsi;

	#define readpixel() if (startpos >= buflen) { return RAWX_ERROR_COMPRESSED_READ_OVERFLOW; } else { pixel = inbuf[startpos]; startpos++; }
	#define writepixel(f) if (pixelpos >= readpixels) {  if (!forced) return RAWX_ERROR_COMPRESSED_UNPACK_OVERFLOW; } else { if (usehalfwidthcompression) pixelpos += 2; else pixelpos++; }

	if (readpixels == 0)
	{
		startpos++;
		return startpos;
	}
	// read stream
	nullpixels = 0;
	for(pixelpos = 0; pixelpos < readpixels; )
	{
		// fill with nulls
		while(nullpixels > 0)
		{
			if (pixelpos >= readpixels)
				break;
			writepixel(nullpixelsi); 
			nullpixels--;
		}
		if (pixelpos >= readpixels)
			break;
		// read pixel
		readpixel();
		if (!pixel || (decomress255 && pixel == 255))
		{	
			nullpixelsi = pixel;
			readpixel();
			nullpixels = pixel;
			continue;
		}
		writepixel(pixel);
	}
	return startpos;
	#undef readpixel
	#undef writepixel
}

// read decompressed LZ77 stream, thanks to Ben Lincoln for that function
// fixme: clean it up
#define DecompressLZ77_MaxIterations 4096000
//#define DecompressLZ77_MaxOutputSize 67108864 // BO never reach this
#define DecompressLZ77_MaxOutputSize 1048576
byte DecompressLZ77Out[DecompressLZ77_MaxOutputSize];
void *DecompressLZ77Stream(int *outbufsize, byte *inbuf, int startpos, int buflen)
{
	byte *outdata;
	int filesize;
	qboolean doneDecompressing = false;
	int numIterations, bytesWritten;
	unsigned short tempIndex, command;
	short offset;
	byte length, currentByte, currentByte1, currentByte2;
	byte tempBuffer[4114];
	int i;

	// compare file size
	if (buflen < 8)
		return NULL;
	filesize = ReadUInt(inbuf) + 4;
	startpos = 4;
	if (filesize != buflen)
		return NULL;

	// initialize decompressor
	tempIndex = 4078;
	numIterations = 0;
	bytesWritten = 0;
    command = 0;
    for (i = 0; i < 4078; i++)
		tempBuffer[i] = 0x20;

	// decompress
	while(!doneDecompressing)
    {
		// decompression errors
		if (numIterations > DecompressLZ77_MaxIterations)
			return NULL;
		if (bytesWritten > DecompressLZ77_MaxOutputSize)
			return NULL;
		if (startpos > buflen)
			return NULL;
		// read command and process it
		command = (unsigned short)(command >> 1);
		if ((command & 0x0100) == 0)
		{
			command = (unsigned short)((unsigned short)(inbuf[startpos]) | 0xFF00);
			startpos++;
		}
		if ((command & 0x01) == 0x01)
		{
			currentByte = inbuf[startpos];
			startpos++;
			DecompressLZ77Out[bytesWritten] = currentByte;
			bytesWritten++;
			tempBuffer[tempIndex] = currentByte;
			tempIndex++;
			tempIndex = (unsigned short)(tempIndex % 4096);
			if (startpos > buflen)
				break;
		}
		else
		{
			currentByte1 = inbuf[startpos];
			startpos++;
			currentByte2 = inbuf[startpos];
			startpos++;
			offset = (unsigned short)(currentByte1 | (currentByte2 & 0xF0) << 4);
			length = (byte)((currentByte2 & 0x0F) + 3);
			for (i = 0; i < length; i++)
			{
				DecompressLZ77Out[bytesWritten] = tempBuffer[(offset + i) % 4096];
				bytesWritten++;
				//counter--;
				//if (counter == 0)
				//{
				//    doneDecompressing = true;
				//    break;
				//}
				if (startpos > buflen)
				{
					doneDecompressing = true;
					break;
				}
				tempBuffer[tempIndex] = tempBuffer[(offset + i) % 4096];
				tempIndex++;
				tempIndex = (unsigned short)(tempIndex % 4096);
			}
		}
		numIterations++;
	}

	// Playstation files apparently need to be multiples of 1024 bytes in size
	if ((bytesWritten % 1024) > 0)
		bytesWritten = bytesWritten + 1024 - (bytesWritten % 1024);

	// allocate and return
	outdata = qmalloc(bytesWritten);
	memcpy(outdata, DecompressLZ77Out, bytesWritten); 
	*outbufsize = bytesWritten;
	return outdata;
}
 
/*
==========================================================================================

  RAW FILE TYPE 0
  a file with offset, width/height and BPP set externally
  thats just a useful way to "massage" files

==========================================================================================
*/

rawblock_t *RawExtract_Type0(unsigned char *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int outputsize, i, chunkpos;
	byte pixel, nullpixels, nullpixelsi;
	rawblock_t *rawblock;

	// read header
	if (rawinfo->bytes != 1 && rawinfo->bytes != 2 && rawinfo->bytes != 3 && rawinfo->bytes != 4 && rawinfo->bytes != 5)
		return RawErrorBlock(NULL, RAWX_ERROR_BAD_OPTIONS);
	if (rawinfo->width*rawinfo->height < 0)
		return RawErrorBlock(NULL, RAWX_ERROR_WIDTH_OR_HEIGHT_NOT_VALID);
	if (rawinfo->offset < 0)
		return RawErrorBlock(NULL, RAWX_ERROR_BAD_OBJECT_OFFSET);
	rawblock = EmptyRawBlock(1);

	// get colormap data
	if (rawinfo->colormapbytes)
		rawblock->colormap = ReadColormap(buffer, filelen, rawinfo->colormapoffset, rawinfo->colormapbytes);

	// apply offset
	buffer += rawinfo->offset;
	filelen = filelen - rawinfo->offset;
	if (filelen < 0)
		filelen = 0;

	// calc output size
	if (rawinfo->bytes == 4)
	{
		Print("%8i colors per byte\n", 4);
		rawinfo->width = rawinfo->width * 4;
		outputsize = rawinfo->width*rawinfo->height;
	}
	else if (rawinfo->bytes == 5)
	{
		Print("%8i colors per byte\n", 2);
		rawinfo->width = rawinfo->width * 2;
		outputsize = rawinfo->width*rawinfo->height;
	}
	else
		outputsize = rawinfo->width*rawinfo->height*rawinfo->bytes;

	// some info
	if (verbose == true)
	{
		Print("%8i bytes given\n", filelen);
		Print("%8i bytes required\n", outputsize);
	}

	// make pixel bytes
	RawBlockAllocateChunk(rawblock, 0, rawinfo->width, rawinfo->height, 0, 0, false);
	if (filelen > outputsize)
		filelen = outputsize;

	// direct or compressed reading
	nullpixels = 0;
	if (rawinfo->usecompression == true && rawinfo->bytes == 1) 
	{
		#define writepixel(f) if (i >= outputsize) {  if (!forced) return RawErrorBlock(rawblock, RAWX_ERROR_COMPRESSED_UNPACK_OVERFLOW); } else { 	rawblock->chunk[0].pixels[i]=f;i++; }
		#define readpixel() if (chunkpos >= outputsize) { return RawErrorBlock(rawblock, RAWX_ERROR_COMPRESSED_READ_OVERFLOW); } else { pixel = buffer[chunkpos]; chunkpos++; }
		// begin frame
		for (i = 0, chunkpos = 0; chunkpos < filelen; )
		{
			// fill with nulls
			while(nullpixels > 0)
			{
				if (i >= outputsize)
					break;
				writepixel(nullpixelsi); 
				nullpixels--;
			}
			if (i >= outputsize)
				break;
			// read pixel
			readpixel();
			if (pixel == rawinfo->compressionpixels[0] ||
				pixel == rawinfo->compressionpixels[1] ||
				pixel == rawinfo->compressionpixels[2] ||
				pixel == rawinfo->compressionpixels[3])
			{	
				nullpixelsi = pixel;
				readpixel();
				nullpixels = pixel;
				continue;
			}
			writepixel(pixel);
		}
		#undef writepixel
		#undef readpixel
	}
	else if (rawinfo->bytes == 4) // 4 colors per byte
	{
		for (i = 0; i < outputsize; i += 4)
		{
			if ((i/4) >= filelen)
				break;
			rawblock->chunk[0].pixels[i] = ((buffer[i/4] & 3))*85;
			rawblock->chunk[0].pixels[i+1] = ((buffer[i/4] >> 2) & 3)*85;
			rawblock->chunk[0].pixels[i+2] = ((buffer[i/4] >> 4) & 3)*85;
			rawblock->chunk[0].pixels[i+3] = ((buffer[i/4] >> 6) & 3)*85;
		/*
			rawblock->chunk[0].pixels[i] = ((buffer[i/4] & 0x7))*63;
			rawblock->chunk[0].pixels[i+1] = ((buffer[i/4] >> 2) & 0x7)*63;
			rawblock->chunk[0].pixels[i+2] = ((buffer[i/4] >> 4) & 0x7)*63;
			rawblock->chunk[0].pixels[i+3] = ((buffer[i/4] >> 6) & 0x7)*63;
		*/
		}
	}
	else if (rawinfo->bytes == 5) // 2 colors per byte
	{
		for (i = 0; i < outputsize; i += 2)
		{
			if ((i/2) >= filelen)
				break;
			rawblock->chunk[0].pixels[i] = ((buffer[i/2] & 15))*16;
			rawblock->chunk[0].pixels[i+1] = ((buffer[i/2] >> 4) & 15)*16;
		}
	}
	else
	{
		for (i = 0; i < filelen; i++)
			rawblock->chunk[0].pixels[i] = buffer[i];
	}
	return rawblock; // who care out eendpos that for type0?
}

/*
==========================================================================================

  RAW FILE TYPE 1

==========================================================================================
*/

// RAW FILE TYPE 1
// Platform: PC 
// Desc: item card, single-object file, really a raw type 3 but easier to parse
// Spec:
//  000: 4 bytes - always 001 000 000 000
//  004: 4 bytes - filesize
//  008: 768 bytes - colormap data (24-bit RGB)
//  776: 4 mystic bytes
//  784: 1 byte width
//  785: 1 byte height
//  786: 1 byte x
//  787: 1 byte y
//  788: pixels width*height

rawblock_t *RawExtract_Type1(unsigned char *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	rawblock_t *rawblock;

	if (!testonly && verbose)
		Print("extracting type1\n");
	if (buffer[0] != 1)
		return RawErrorBlock(NULL, RAWX_ERROR_HEADER_NOT_VALID); // header not valid
	if (buffer[784] > 120 || buffer[785] > 120 || buffer[784]*buffer[785] < 0)
		return RawErrorBlock(NULL, RAWX_ERROR_WIDTH_OR_HEIGHT_NOT_VALID); // invalid width/height
	if (forced == false && (filelen - (788 + buffer[784]*buffer[785]) > 32))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_BIGGER_THAN_REQUIRED); // file is bigger than required
	if (filelen < (781 + buffer[784]*buffer[785]))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED); // file is smaller than require

	rawblock = EmptyRawBlock(1);
	rawinfo->width = buffer[784];
	rawinfo->height = buffer[785];
	if (verbose == true)
		Print("size: %ix%i\n", rawinfo->width, rawinfo->height);
	RawBlockAllocateChunk(rawblock, 0, rawinfo->width, rawinfo->height, 0, 0, true);
	rawblock->colormap = buffer + 8;
	rawblock->colormapExternal = true;
	rawblock->chunk[0].pixels = buffer + 788;
	return rawblock;
}

/*
==========================================================================================

  RAW FILE TYPE 2
  multiobject file with per-object palette

==========================================================================================
*/

// RAW FILE TYPE 2
// Platform: PC 
// Desc: multiobject file with per-object palette
// Note: real width/height may be double of what written in headers
// Spec:
//   4 bytes - number of objects
//   4 bytes - size of file
//   768 bytes - unused colormap data
//   <object headers>
//      768 bytes - colormap
//      4 bytes - offset after colormaps
//      1 byte - width
//      1 byte - height
//      1 byte - x
//      1 byte - y
//   <object pixels>
//     width*height bytes - indexes into colormap
rawblock_t *RawExtract_Type2(unsigned char *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, i, pos1, pos2, chunkpos, resmult;
	rawblock_t *rawblock;
	byte *chunk;

	if (!testonly && verbose)
		Print("extracting type2\n");
	// read header
	numobjects = buffer[1] * 256 + buffer[0];
	if (verbose == true)
		Print("tag: %i\n", numobjects);
	if (buffer[2] != 0 || buffer[3] != 0)
		return RawErrorBlock(NULL, RAWX_ERROR_HEADER_NOT_VALID);
	if (numobjects <= 0 || numobjects > 200)
		return RawErrorBlock(NULL, RAWX_ERROR_IMPLICIT_OBJECTS_COUNT);
	rawblock = EmptyRawBlock(numobjects);

	// unknown info (mystic bytes and 768 bytes of colormap)
	if (filelen < 776)
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// global position
	rawblock->posx = ReadShort(buffer + 4);
	rawblock->posy = ReadShort(buffer + 6);
	if (verbose == true)
		Print("picture position: %03ix%03i\n", rawblock->posx, rawblock->posy);

	// print objects
	for (i = 0; i < (int)numobjects; i++)
	{
		chunkpos = 776 + (768 + 8)*i + 768;
		if (filelen < (chunkpos + 8))
			return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED); // file is smaller than required
		chunk = buffer + chunkpos;
		if (verbose == true)
			Print("object %03i: %03i %03i %03i %3i %03i %03i %03i %03i\n", i + 1, chunk[0], chunk[1], chunk[2], chunk[3], chunk[4], chunk[5], chunk[6], chunk[7]);
	}

	// test structure positions (for next code block)
	// get pixeldata start block
	pos2 = pos1 = 776 + (768 + 8)*numobjects;
	for (i = 0; i < (int)numobjects; i++)
	{
		chunk = buffer + 776 + (768 + 8)*i + 768;
		pos1 += chunk[4]*chunk[5];
		pos2 += chunk[4]*chunk[5]*4;
	}

	// hacks land
	// it seems BO has 2 types of such files with exactly same info, 
	// but one type has true width/height, other it's resulution doubled
	// our task is to determine 'a bigger' file: we simulate pixel reading
	// and if reached EOF - it is standart file, otherwise it is bigger file
	// also here we will protect against bad files which is bigger (and hence it
	if (rawinfo->doubleres == rauto)
	{
		if ((filelen - pos1) < 32)
		{
			resmult = 1;
			chunkpos = pos1;
		}
		else if ((filelen - pos2) < 32)
		{
			resmult = 2;
			chunkpos = pos2;
		}
		else 
		{	
			if (forced == false)
				return RawErrorBlock(rawblock, RAWX_ERROR_FILE_BIGGER_THAN_REQUIRED);
			else // multiobject?
			{
				resmult = 2;
				chunkpos = pos2;
			}
		}
	}	
	else if (rawinfo->doubleres == rtrue)
	{
		resmult = 2;
		chunkpos = pos2;
	}
	else 
	{
		resmult = 1;
		chunkpos = pos1;
	}

	// check if file is smaller than required
	if (filelen < chunkpos)
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED); 

	// read objects
	chunkpos = 776 + (768 + 8)*numobjects;
	for (i = 0; i < (int)numobjects; i++)
	{		
		chunk = buffer + 776 + (768 + 8)*i;
		RawBlockAllocateChunk(rawblock, i, chunk[772]*resmult, chunk[773]*resmult, 0, 0, true);
		rawblock->chunk[i].colormap = chunk;
		rawblock->chunk[i].colormapExternal = true;
		rawblock->chunk[i].pixels = buffer + chunkpos;
		chunkpos = chunkpos + chunk[772]*resmult*chunk[773]*resmult;
	}
	if (testonly == true)
		rawinfo->doubleres = (resmult == 2) ? rtrue : rfalse;
	rawblock->errorcode = chunkpos;

	// warn if multiobject file
	if (chunkpos != filelen)
	{
		rawblock->notEOF = true;
		if (!testonly && verbose)
			Print("Warning! Reading has ended on %i but filelen is %i, possible multiobject file.\n", chunkpos, filelen);
	}
	return rawblock;
}

/*
==========================================================================================

  RAW FILE TYPE 3

==========================================================================================
*/

// RAW FILE TYPE 3
// Desc: multiobject file with shared palette, with zero-length compression
// Note: could be half-width compressed (1 byte codes 2 pixels) hence colormap should be perturbated
// Note: also some files have 255 indexrun-length-compressed as well, this pixels are usually blue (shadow)
//   4 bytes - number of objects
//   4 bytes - filesize
//   768 bytes - colormap data (24-bit RGB)
//   2 bytes - x position
//   2 bytes - y position
// <object headers>
//    4 bytes - offset after headers
//    1 byte - width
//    1 byte - height
//    1 byte - x
//    1 byte - y
// <zero-length compressed pixels data>
//    read pixel, if it's 0 - read next pixel and make this number of black pixels, otherwise write as normal pixel
rawblock_t *RawExtract_Type3(byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, filesize, i, chunkpos, last;
	unsigned char *chunk;
	qboolean decompress255, halfres;
	rawchunk_t *rawchunk;
	rawblock_t *rawblock;

	if (!testonly && verbose)
		Print("extracting type3\n");
	// check headers
	if (filelen < 780)
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);
	numobjects = ReadUInt(buffer);
	if (numobjects <= 0 || numobjects > 1000)
		return RawErrorBlock(NULL, RAWX_ERROR_IMPLICIT_OBJECTS_COUNT);
	if (verbose == true)
		Print("objects: %i\n", numobjects);
	if (filelen < (780 + 8*numobjects))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// file size
	filesize = ReadUInt(buffer + 4);
	if (verbose == true)
	{
		Print("filesize: %i\n", filesize);
		Print("realfilesize: %i\n", filelen);
	}
	rawblock = EmptyRawBlock(numobjects);

	// colormap
	rawblock->colormap = ReadColormap(buffer, filelen, 8, 3);
	if (rawblock->colormap == NULL)
		return RawErrorBlock(rawblock, RAWX_ERROR_BAD_COLORMAP);

	// global position
	rawblock->posx = ReadShort(buffer + 776);
	rawblock->posy = ReadShort(buffer + 778);
	if (verbose == true)
		Print("picture position: %03ix%03i\n", rawblock->posx, rawblock->posy);

	// check filelen again
	if (filelen < (780 + 8*numobjects + 2))
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// read object headers, test them
	last = -1;
	for (i = 0; i < numobjects; i++)
	{
		chunk = buffer + 780 + i*8;

		rawchunk = &rawblock->chunk[i];
		rawchunk->offset = 780 + numobjects*8 + ReadUInt(chunk);
		rawchunk->width = chunk[4];
		rawchunk->height = chunk[5];
		rawchunk->size = chunk[4]*chunk[5];
		rawchunk->x = chunk[6];
		rawchunk->y = chunk[7];
		if (rawchunk->width < 0 || rawchunk->height < 0)
			return RawErrorBlock(rawblock, RAWX_ERROR_BAD_OBJECT_HEADER);
		if (rawchunk->offset <= last && !forced)
			return RawErrorBlock(rawblock, RAWX_ERROR_BAD_OBJECT_OFFSET);
		if (verbose == true)
			Print("%03i: %8i %03ix%03i %03i %03i\n", i, rawchunk->offset, rawchunk->width, rawchunk->height, rawchunk->x, rawchunk->y);    
		last = rawchunk->offset;
	}

	// VorteX: detect half-width compression and compression of 255 index, initial guess is that all is ON
	// test all variants and select one which fit
	// yes, i know this is mess
	halfres = true;
	decompress255 = true;
	#define detect()	for (i = 0; i < numobjects; i++) \
						{ \
							rawchunk = &rawblock->chunk[i]; \
							chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, decompress255, halfres, false); \
							last = ((i+1) < numobjects) ? rawblock->chunk[i+1].offset : filelen; \
							if (chunkpos != last) \
							{ \
								chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, (rawchunk->width + 256) * rawchunk->height , decompress255, halfres, false); \
								if (chunkpos == last) \
								{ \
									rawchunk->width = rawchunk->width + 256; \
									rawchunk->size = rawchunk->width*rawchunk->height; \
								} \
								else \
								{ \
									chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->width * (rawchunk->height + 256), decompress255, halfres, false); \
									if (chunkpos == last) \
									{ \
										rawchunk->height = rawchunk->height + 256; \
										rawchunk->size = rawchunk->width*rawchunk->height; \
									} \
									else \
									{ \
										chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, (rawchunk->width + 256 ) * (rawchunk->height + 256), decompress255, halfres, false); \
										if (chunkpos == last) \
										{ \
											rawchunk->width = rawchunk->width + 256; \
											rawchunk->height = rawchunk->height + 256; \
											rawchunk->size = rawchunk->width*rawchunk->height; \
										} \
										else \
										{ \
											break; \
										} \
									} \
								} \
							} \
						}
	detect()
	if (i < numobjects)
	{
		halfres = true;
		decompress255 = false;
		detect()
		if (i < numobjects)
		{
			halfres = false;
			decompress255 = true;
			detect()
			if (i < numobjects)
			{
				halfres = false;
				decompress255 = false;
			}
		}
	}
	#undef detect

	// read pixels
	for (i = 0; i < numobjects; i++)
	{
		rawchunk = &rawblock->chunk[i];
		RawBlockAllocateChunkSimple(rawblock, i, false);
		chunkpos = ReadRLCompressedStream(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, decompress255, halfres, forced);
		if (chunkpos < 0)
			return RawErrorBlock(rawblock, chunkpos);
	}
	rawblock->errorcode = chunkpos;

	// warn if multiobject file
	if (chunkpos != filelen)
	{
		rawblock->notEOF = true;
		if (!testonly && verbose)
			Print("Warning! Reading has ended on %i but filelen is %i, possible multiobject file.\n", chunkpos, filelen);
	}
	return rawblock;
}

/*
==========================================================================================

  RAW FILE TYPE 4

  DERIVATIVE FROM TYPE 3 WITH ADDITIONAL DATA

==========================================================================================
*/

// RAW FILE TYPE 4
// Desc: multiobject file with shared palette, with zero-length compression, with additional objects header
// Note: could be half-width compressed (1 byte codes 2 pixels) hence colormap should be perturbated
// Note: also some files have 255 index run-length-compressed as well, this pixels are usually blue (shadows)
// Note: some files has <mystic object headers> rounded to structure size, some not
//   4 bytes - number of objects
//   4 bytes - filesize
//   <mystic object headers> - 1 byte for each object
//   768 bytes - colormap data (24-bit RGB)
//   2 bytes - x position
//   2 bytes - y position
// <object headers>
//    4 bytes - offset after headers
//    1 byte - width
//    1 byte - height
//    1 byte - x
//    1 byte - y
// <zero-length compressed pixels data>
//    read pixel, if it's 0 - read next pixel and make this number of black pixels, otherwise write as normal pixel
rawblock_t *RawExtract_Type4(byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, filesize, i, objbitssize, chunkpos, last;
	rawchunk_t *rawchunk;
	rawblock_t *rawblock;
	qboolean halfres, decompress255;
	byte *chunk;

	if (!testonly && verbose)
		Print("extracting type4\n");
	// check header
	if (filelen < 782)
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);
	numobjects = ReadUInt(buffer);
	if (numobjects <= 0 || numobjects > 1000)
		return RawErrorBlock(NULL, RAWX_ERROR_IMPLICIT_OBJECTS_COUNT);
	if (verbose == true)
		Print("objects: %i\n", numobjects);
	if (filelen < (780 + 9*numobjects))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// file size
	filesize = ReadUInt(buffer + 4);
	if (verbose == true)
	{
		Print("filesize: %i\n", filesize);
		Print("realfilesize: %i\n", filelen);
	}
	rawblock = EmptyRawBlock(numobjects);

	// objectbits
	for (i = 0; i < numobjects; i++)
		rawblock->chunk[i].flagbit = buffer[8 + numobjects];
	objbitssize = RoundStruct(numobjects);

	// colormap
	rawblock->colormap = ReadColormap(buffer, filelen, 8 + objbitssize, 3);
	if (rawblock->colormap == NULL)
		return RawErrorBlock(rawblock, RAWX_ERROR_BAD_COLORMAP);

	// global position
	rawblock->posx = ReadShort(buffer + 776 + objbitssize);
	rawblock->posy = ReadShort(buffer + 778 + objbitssize);
	if (verbose == true)
		Print("picture position: %03ix%03i\n", rawblock->posx, rawblock->posy);

	// check filelen again
	if (filelen < (780 + objbitssize + 8*numobjects + 2))
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// read object headers, test them
	last = -1;
	for (i = 0; i < numobjects; i++)
	{
		chunk = buffer + 780 + objbitssize + i*8;
		rawchunk = &rawblock->chunk[i];
		rawchunk->offset = 780 + objbitssize + numobjects*8 + ReadUInt(chunk);
		rawchunk->width = chunk[4];
		rawchunk->height = chunk[5];
		rawchunk->size = rawchunk->width*rawchunk->height;
		rawchunk->x = chunk[6];
		rawchunk->y = chunk[7];
		if (rawchunk->width < 0 || rawchunk->height < 0)
		{
			if (verbose == true)
				Print("%03i: bad width/height!\n", i);
			break;
		}
		if (rawchunk->offset <= last && !forced)
		{
			if (verbose == true)
				Print("%03i: bad offset!\n", i);
			break;
		}
		if (verbose == true)
			Print("%03i: %8i %03ix%03i %03i %03i - %03i\n", i, rawblock->chunk[i].offset, rawblock->chunk[i].width, rawblock->chunk[i].height, rawblock->chunk[i].x, rawblock->chunk[i].y, rawblock->chunk[i].flagbit);    
		last = rawchunk->offset;
	}

	// VorteX: hackland
	// if failed - try with not-rounded objbitssize
	if (i < numobjects)
	{
		objbitssize = numobjects;

		// re-read colormap
		qfree(rawblock->colormap);
		rawblock->colormap = ReadColormap(buffer, filelen, 8 + objbitssize, 3);
		if (rawblock->colormap == NULL)
			return RawErrorBlock(rawblock, RAWX_ERROR_BAD_COLORMAP);

		// re-read global position
		rawblock->posx = buffer[776 + objbitssize] + buffer[777 + objbitssize]*256;
		rawblock->posy = buffer[778 + objbitssize] + buffer[779 + objbitssize]*256;
		if (verbose == true)
			Print("picture position: %03ix%03i\n", rawblock->posx, rawblock->posy);

		last = -1;
		for (i = 0; i < numobjects; i++)
		{
			chunk = buffer + 780 + objbitssize + i*8;
			rawchunk = &rawblock->chunk[i];
			rawchunk->offset = 780 + objbitssize + numobjects*8 + ReadUInt(chunk);
			rawchunk->width = chunk[4];
			rawchunk->height = chunk[5];
			rawchunk->size = rawchunk->width*rawchunk->height;
			rawchunk->x = chunk[6];
			rawchunk->y = chunk[7];
			// error?
			if (rawchunk->width < 0 || rawchunk->height < 0)
				return RawErrorBlock(rawblock, RAWX_ERROR_BAD_OBJECT_HEADER);
			if (rawchunk->offset <= last && !forced)
				return RawErrorBlock(rawblock, RAWX_ERROR_BAD_OBJECT_OFFSET);
			if (verbose == true)
				Print("%03i: %8i %03ix%03i %03i %03i - %03i\n", i, rawblock->chunk[i].offset, rawblock->chunk[i].width, rawblock->chunk[i].height, rawblock->chunk[i].x, rawblock->chunk[i].y, rawblock->chunk[i].flagbit);    
			last = rawchunk->offset;
		}
	}

	// VorteX: detect half-width compression
	// VorteX: hack! some widths and heights need to add +256, so check for that cases
	halfres = true;
	decompress255 = true;
	#define detect()	for (i = 0; i < numobjects; i++) \
						{ \
							rawchunk = &rawblock->chunk[i]; \
							chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, decompress255, halfres, false); \
							last = ((i+1) < numobjects) ? rawblock->chunk[i+1].offset : filelen; \
							if (chunkpos != last) \
							{ \
								chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, (rawchunk->width + 256) * rawchunk->height , decompress255, halfres, false); \
								if (chunkpos == last) \
								{ \
									rawchunk->width = rawchunk->width + 256; \
									rawchunk->size = rawchunk->width*rawchunk->height; \
								} \
								else \
								{ \
									chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->width * (rawchunk->height + 256), decompress255, halfres, false); \
									if (chunkpos == last) \
									{ \
										rawchunk->height = rawchunk->height + 256; \
										rawchunk->size = rawchunk->width*rawchunk->height; \
									} \
									else \
									{ \
										chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, (rawchunk->width + 256 ) * (rawchunk->height + 256), decompress255, halfres, false); \
										if (chunkpos == last) \
										{ \
											rawchunk->width = rawchunk->width + 256; \
											rawchunk->height = rawchunk->height + 256; \
											rawchunk->size = rawchunk->width*rawchunk->height; \
										} \
										else \
										{ \
											break; \
										} \
									} \
								} \
							} \
						}
	detect()
	if (i < numobjects)
		halfres = false;
	#undef detect
	
	// read pixels
	for (i = 0; i < numobjects; i++)
	{
	//	Verbose(" reading chunk %i...\n", i); 
		rawchunk = &rawblock->chunk[i];
		RawBlockAllocateChunkSimple(rawblock, i, false);
		chunkpos = ReadRLCompressedStream(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, decompress255, halfres, forced);
		if (chunkpos < 0)
			return RawErrorBlock(rawblock, chunkpos);
	}
	rawblock->errorcode = chunkpos;

	// warn if multiobject file
	if (chunkpos != filelen)
	{
		rawblock->notEOF = true;
		if (!testonly && verbose)
			Print("Warning! Reading has ended on %i but filelen is %i, possible multiobject file.\n", chunkpos, filelen);
	}
	return rawblock;
}

/*
==========================================================================================

  RAW FILE TYPE 5

  DERIVATIVE FROM TYPE 3 BUT NO POSITION BYTES

==========================================================================================
*/

// RAW FILE TYPE 5
// Desc: multiobject file with shared palette, with zero-length compression, no position info
// Note: some files have 255 indexrun-length-compressed as well, this pixels could be blue or red
// Note: some chunks may have broken width/height because they width is above 255 and it uses 1 byte 
//       to encode it, 255 + [width] should be used to correct this
// Spec:
//   4 bytes - number of objects
//   4 bytes - filesize
//   768 bytes - colormap data (24-bit RGB)
// <object headers>
//    4 bytes - offset after headers
//    1 byte - width
//    1 byte - height
//    1 byte - x
//    1 byte - y
// <zero-length compressed pixels data>
//    read pixel, if it's 0 - read next pixel and make this number of black pixels, otherwise write as normal pixel
rawblock_t *RawExtract_Type5(byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, filesize, i, chunkpos, last;
	signed int minx, miny;
	qboolean decompress255, halfres;
	rawchunk_t *rawchunk;
	rawblock_t *rawblock;
	byte *chunk;

	if (!testonly && verbose)
		Print("extracting type5\n");
	// check header
	if (filelen < 776)
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);
	numobjects = ReadUInt(buffer);
	if (numobjects <= 0 || numobjects > 1000)
		return RawErrorBlock(NULL, RAWX_ERROR_IMPLICIT_OBJECTS_COUNT);
	if (verbose == true)
		Print("objects: %i\n", numobjects);
	if (filelen < (776 + 8*numobjects))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// file size
	filesize = buffer[7]*16777216 + buffer[6]*65536 + buffer[5]*256 + buffer[4];
	if (verbose == true)
	{
		Print("filesize: %i\n", filesize);
		Print("realfilesize: %i\n", filelen);
	}
	rawblock = EmptyRawBlock(numobjects);

	// colormap
	rawblock->colormap = ReadColormap(buffer, filelen, 8, 3);
	if (rawblock->colormap == NULL)
		return RawErrorBlock(rawblock, RAWX_ERROR_BAD_COLORMAP);

	// check file size again
	if (filelen < (776 + 8*numobjects + 2))
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// VorteX: detect compression of 255 index
	decompress255 = true;
	halfres = false;
	#define detect()	for (i = 0; i < numobjects; i++) \
						{ \
							rawchunk = &rawblock->chunk[i]; \
							chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, decompress255, halfres, false); \
							last = ((i+1) < numobjects) ? rawblock->chunk[i+1].offset : filelen; \
							if (chunkpos != last) \
							{ \
								chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, (rawchunk->width + 256) * rawchunk->height , decompress255, halfres, false); \
								if (chunkpos == last) \
								{ \
									rawchunk->width = rawchunk->width + 256; \
									rawchunk->size = rawchunk->width*rawchunk->height; \
								} \
								else \
								{ \
									chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->width * (rawchunk->height + 256), decompress255, halfres, false); \
									if (chunkpos == last) \
									{ \
										rawchunk->height = rawchunk->height + 256; \
										rawchunk->size = rawchunk->width*rawchunk->height; \
									} \
									else \
									{ \
										chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, (rawchunk->width + 256 ) * (rawchunk->height + 256), decompress255, halfres, false); \
										if (chunkpos == last) \
										{ \
											rawchunk->width = rawchunk->width + 256; \
											rawchunk->height = rawchunk->height + 256; \
											rawchunk->size = rawchunk->width*rawchunk->height; \
										} \
										else \
										{ \
											break; \
										} \
									} \
								} \
							} \
						}
	detect()
	if (i < numobjects)
		decompress255 = false;
	#undef detect

	// read object headers, test them
	last = -1;
	for (i = 0; i < numobjects; i++)
	{
		chunk = buffer + 776 + i*8;

		rawchunk = &rawblock->chunk[i];
		rawchunk->offset = 776 + numobjects*8 + ReadUInt(chunk);
		rawchunk->width = chunk[4] * ((rawinfo->doubleres == true) ? 2 : 1);
		rawchunk->height = chunk[5] * ((rawinfo->doubleres == true) ? 2 : 1);
		rawchunk->size = rawchunk->width * rawchunk->height;
		rawchunk->x = ReadSignedByte(chunk + 6); // FIXME!
		rawchunk->y = ReadSignedByte(chunk + 7);
		if (rawchunk->width < 0 || rawchunk->height < 0)
			return RawErrorBlock(rawblock, RAWX_ERROR_BAD_OBJECT_HEADER);
		if (rawchunk->offset <= last && !forced)
			return RawErrorBlock(rawblock, RAWX_ERROR_BAD_OBJECT_OFFSET);
		last = rawchunk->offset;
	}

	// hackland: disallow negative offsets, by shifting all chunks
	// as negative offsets kills aligning math
	// this is not changed anything as withg shifting x/y for each chunk we are shifting global 
	// positions to compernsate as well
	minx = miny = 0;
	for (i = 0; i < numobjects; i++)
	{
		if (minx > rawblock->chunk[i].x)
			minx = rawblock->chunk[i].x;
		if (miny > rawblock->chunk[i].y)
			miny = rawblock->chunk[i].y;
	}
	if (minx || miny)
	{
		rawblock->posx = rawblock->posx + minx;
		rawblock->posy = rawblock->posy + miny;
		for (i = 0; i < numobjects; i++)
		{
			rawblock->chunk[i].x = rawblock->chunk[i].x - minx;
			rawblock->chunk[i].y = rawblock->chunk[i].y - miny;
		}
	}

	// print headers
	if (verbose == true)
	{
		for (i = 0; i < numobjects; i++)
			Print("%03i: %8i %03ix%03i %03i %03i - %03i\n", i, rawblock->chunk[i].offset, rawblock->chunk[i].width, rawblock->chunk[i].height, rawblock->chunk[i].x, rawblock->chunk[i].y, rawblock->chunk[i].flagbit);    
	}

	// read pixels
	// VorteX: there is a files without compression
	for (i = 0; i < numobjects; i++)
	{
		rawchunk = &rawblock->chunk[i];
		RawBlockAllocateChunkSimple(rawblock, i, false);
		chunkpos = ReadRLCompressedStream(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, decompress255, halfres, forced);
		if (chunkpos < 0)
			return RawErrorBlock(rawblock, chunkpos);
	}
	rawblock->errorcode = chunkpos;

	// warn if multiobject file
	if (chunkpos != filelen)
	{
		rawblock->notEOF = true;
		if (!testonly && verbose)
			Print("Warning! Reading has ended on %i but filelen is %i, possible multiobject file.\n", chunkpos, filelen);
	}
	return rawblock;
}

/*
==========================================================================================

  TYPE 6 - MAP

==========================================================================================
*/

// RAW FILE TYPE 6
// Description: compressed TIM image
typedef struct
{
	unsigned short used_tiles[40];
	unsigned char someinfo[18764];
	unsigned short layer1[80][80];
	unsigned short layer2[80][80];
	unsigned char layer3[80][80];
	unsigned char someinfo2[20068];
}bo_map_t;

rawblock_t *RawExtract_Type6(byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int decSize;
	byte *dec;
	bo_map_t map;
	unsigned short shortmap[80][80], shortmap2[80][80];
	int i, k;
	rawblock_t *rawblock;

	if (!testonly && verbose)
		Print("extracting type6\n");

	// try decompress
	dec = DecompressLZ77Stream(&decSize, buffer, 0, filelen);
	if (dec == NULL)
		return RawErrorBlock(NULL, RAWX_ERROR_NOT_INDENTIFIED);

	// should have fixed size
	if (decSize != 64512)
	{
		qfree(dec);
		return RawErrorBlock(NULL, RAWX_ERROR_NOT_INDENTIFIED);
	}

	// this is type6
	printf("groupfiles: ");
	memcpy(&map, dec, 64512);
	qfree(dec);
	for (i = 0; i < 40; i++)
	{
		if (map.used_tiles[i] == 0 || map.used_tiles[i] == 65535)
			continue;
		printf("%05i ", map.used_tiles[i]);
	}
	printf("\n");

	rawblock = EmptyRawBlock(0);
	return rawblock;


	RawTGA("layer1.tga", 160, 80, 0, 0, 0, 0, NULL, (byte *)&map.layer1, 8, NULL);
	RawTGA("layer2.tga", 160, 80, 0, 0, 0, 0, NULL, (byte *)&map.layer2, 8, NULL);
	RawTGA("layer3.tga", 80, 80, 0, 0, 0, 0, NULL, (byte *)&map.layer3, 8, NULL);

	// transpose a map
	for (i = 0; i < 80; i++)
		for (k = 0; k < 80; k++)
			shortmap[i][k] = map.layer1[79-k][i];
	RawTGA("layer4.tga", 160, 80, 0, 0, 0, 0, NULL, (byte *)&shortmap, 8, NULL);

	// transpose a map
	for (i = 0; i < 80; i++)
		for (k = 0; k < 80; k++)
			shortmap2[i][k] = map.layer2[79-k][i];
	RawTGA("layer5.tga", 160, 80, 0, 0, 0, 0, NULL, (byte *)&shortmap2, 8, NULL);

}

/*
==========================================================================================

  TYPE 7 - TILE

==========================================================================================
*/

// RAW FILE TYPE 7
// Description: compressed TIM image
rawblock_t *RawExtract_Type7(byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	byte *dec, *in, *out;
	rawblock_t *rawblock;
	tim_image_t *tim;
	int decSize, i;

	if (!testonly && verbose)
		Print("extracting type7\n");

	// try decompress
	dec = DecompressLZ77Stream(&decSize, buffer, 0, filelen);
	if (dec == NULL)
		return RawErrorBlock(NULL, RAWX_ERROR_NOT_INDENTIFIED);

	// read TIM file from that stream
	tim = TIM_LoadFromBuffer(dec, decSize);
	if (tim->type != TIM_8Bit)
	{
		qfree(dec);
		FreeTIM(tim);
		return RawErrorBlock(NULL, RAWX_ERROR_NOT_INDENTIFIED);
	}
	qfree(dec);

	// convert TIM to chunk
	rawblock = EmptyRawBlock(1);
	rawinfo->width = tim->dim.xsize;
	rawinfo->height = tim->dim.ysize;
	if (verbose == true)
		Print("size: %ix%i\n", rawinfo->width, rawinfo->height);
	RawBlockAllocateChunk(rawblock, 0, rawinfo->width, rawinfo->height, 0, 0, false);
	// read colormap, convert to 24-bit
	rawblock->colormap = qmalloc(768);
	in = tim->CLUT->data;
	out = rawblock->colormap;
	for (i = 0; i < 256; i++)
	{
		*out++ = (in[i*2] & 0x1F) * 8; 
		*out++ = (((in[i*2] & 0xE0) >> 5) + ((in[i*2 + 1] & 0x3) << 3)) * 8;
		*out++ = ((in[i*2 + 1] & 0x7C) >> 2) * 8;
	}
	// read indexes
	memcpy(rawblock->chunk[0].pixels, tim->pixels, rawblock->chunk[0].size);

	FreeTIM(tim);
	rawblock->errorcode = decSize;

	return rawblock;
}

/*
==========================================================================================

  UNFINISHED RAW FILE TYPES

==========================================================================================
*/

rawblock_t *RawExtract_Type8(byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	return RawErrorBlock(NULL, -1);
}

/*
==========================================================================================

  MAIN

==========================================================================================
*/

rawblock_t *RawExtract(byte *filedata, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, rawtype_t forcetype)
{
	rawtype_t rawtype, testtype;
	rawblock_t *rawblock;

	rawtype = (forcetype == RAW_TYPE_UNKNOWN) ? rawinfo->type : forcetype;
	rawblock = NULL;

	// pass or autoscan default types
	if (rawtype == RAW_TYPE_0) { rawblock = RawExtract_Type0(filedata, filelen, rawinfo, testonly, verbose, false); goto end; }
	#define trytype(t,f)	if (rawtype == RAW_TYPE_UNKNOWN || rawtype == t) { testtype = t; rawblock = f(filedata, filelen, rawinfo, testonly, verbose, false); if (rawblock->errorcode >= 0 || rawtype != RAW_TYPE_UNKNOWN) goto end; FreeRawBlock(rawblock); rawblock = NULL; }
	trytype(RAW_TYPE_1, RawExtract_Type1)
	trytype(RAW_TYPE_2, RawExtract_Type2)
	trytype(RAW_TYPE_4, RawExtract_Type4) 	// VorteX: scan type4 and type5 before type3, because they are derivations from type3
	trytype(RAW_TYPE_5, RawExtract_Type5)
	trytype(RAW_TYPE_3, RawExtract_Type3)
	trytype(RAW_TYPE_6, RawExtract_Type6)
	trytype(RAW_TYPE_7, RawExtract_Type7)
	trytype(RAW_TYPE_8, RawExtract_Type8)
	#undef trytype
end:
	if (rawblock == NULL)
		return RawErrorBlock(rawblock, RAWX_ERROR_NOT_INDENTIFIED);
	// export
	if (testonly)
		rawinfo->type = testtype;
	return rawblock;
}
	
int Raw_Main(int argc, char **argv)
{
	char filename[MAX_BLOODPATH], basefilename[MAX_BLOODPATH], outfile[MAX_BLOODPATH], *c;
	unsigned char *filedata;
	int filelen;
	qboolean forced, rawnoalign;
	rawinfo_t rawinfo;
	rawblock_t *rawblock;
	int i;

	FlushRawInfo(&rawinfo);
	i = 1;
	Verbose("=== Raw Image ===\n");
	Verbose("%s\n", argv[i]);

	// get inner file
	strcpy(filename, argv[i]);
	StripFileExtension(filename, basefilename);
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
	forced = false;
	rawnoalign = false;
	for (i = i; i < argc; i++)
	{
		if (!strcmp(argv[i], "-width"))
		{
			i++;
			if (i < argc)
			{
				rawinfo.width = atoi(argv[i]);
				Verbose("Width: %i\n", rawinfo.width);
			}
		}
		else if(!strcmp(argv[i], "-height"))
		{
			i++;
			if (i < argc)
			{
				rawinfo.height = atoi(argv[i]);
				Verbose("Height: %i\n", rawinfo.height);
			}
		}
		else if(!strcmp(argv[i], "-type"))
		{
			i++;
			if (i < argc)
			{
				rawinfo.type = ParseRawType(argv[i]);
				Verbose("Raw type: %s\n", UnparseRawType(rawinfo.type));
			}
		}
		else if(!strcmp(argv[i], "-forcetype"))
		{
			i++;
			if (i < argc)
			{
				forced = true;
				rawinfo.type = ParseRawType(argv[i]);
				Verbose("Forcing raw type: %s\n", UnparseRawType(rawinfo.type));
			}
		}
		else if(!strcmp(argv[i], "-chunk"))
		{
			i++;
			if (i < argc)
			{
				rawinfo.chunknum = atoi(argv[i]);
				Verbose("Only read chunk #: %i\n", rawinfo.chunknum);
			}
		}
		else if(!strcmp(argv[i], "-doubleres"))
		{
			i++;
			if (i < argc)
			{
				rawinfo.doubleres = ParseRawSwitch(argv[i]);
				Verbose("Doubleres: %s\n", UnparseRawSwitch(rawinfo.doubleres));
			}
		}
		else if(!strcmp(argv[i], "-offset"))
		{
			i++;
			if (i < argc)
			{
				rawinfo.offset = atoi(argv[i]);
				Verbose("Offset: %i bytes\n", rawinfo.offset);
			}
		}
		else if(!strcmp(argv[i], "-bytes"))
		{
			i++;
			if (i < argc)
			{
				rawinfo.bytes = atoi(argv[i]);
				Verbose("Color bytes: %i\n", rawinfo.bytes);
			}
		}
		else if(!strcmp(argv[i], "-colormapoffset"))
		{
			i++;
			if (i < argc)
				rawinfo.colormapoffset = atoi(argv[i]);
			Verbose("Colormap offset: %i\n", rawinfo.colormapoffset);
		}
		else if(!strcmp(argv[i], "-colormapbytes"))
		{
			i++;
			if (i < argc)
				rawinfo.colormapbytes = atoi(argv[i]);
			Verbose("Colormap bytes: %i\n", rawinfo.colormapbytes);
		}
		else if(!strcmp(argv[i], "-noswap"))
		{
			rawinfo.dontSwapBgr = true;
			Verbose("BGR->RGB swap: disabled\n");
		}
		else if(!strcmp(argv[i], "-noclut"))
		{
			rawinfo.disableCLUT = true;
			Verbose("CLUT: disabled\n");
		}
		else if(!strcmp(argv[i], "-cmpr1") || !strcmp(argv[i], "-cmpr2") || !strcmp(argv[i], "-cmpr3") || !strcmp(argv[i], "-cmpr4"))
		{
			i++;
			if (i < argc)
			{
				rawinfo.usecompression = true;
				if (!strcmp(argv[i], "-cmpr1"))
					rawinfo.compressionpixels[0] = atoi(argv[i]);
				else if (!strcmp(argv[i], "-cmpr2"))
					rawinfo.compressionpixels[1] = atoi(argv[i]);
				else if (!strcmp(argv[i], "-cmpr3"))
					rawinfo.compressionpixels[2] = atoi(argv[i]);
				else
					rawinfo.compressionpixels[3] = atoi(argv[i]);
			}
			Verbose("RLE compression on index #%i\n", atoi(argv[i]));
		}
		else if(!strcmp(argv[i], "-noalign"))
		{
			rawnoalign = true;
			Verbose("Disable aligning of RAW images\n");
		}	
	}

	// open and extract file
	filelen = LoadFile(filename, &filedata);
	rawblock = RawExtract(filedata, filelen, &rawinfo, false, true, (forced == true) ? rawinfo.type : RAW_TYPE_UNKNOWN);
	qfree(filedata);
	if (rawblock->errorcode < 0)
		Print("Raw error code %i: %s\n", rawblock->errorcode, RawStringForResult(rawblock->errorcode));
	else
		TGAfromRAW(rawblock, &rawinfo, outfile, rawnoalign, true, false);
	FreeRawBlock(rawblock);
	Print("done.\n");
	return 0;
}
