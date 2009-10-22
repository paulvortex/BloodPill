////////////////////////////////////////////////////////////////
//
// Blood Omen RAW files loader
// coded by Pavel [VorteX] Timofeyev and placed to public domain
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

// 05106F48 - трансформация в волка, 14050761 - в летучую мышь.

#include "bloodpill.h"
#include "rawfile.h"
#include "cmdlib.h"
#include "mem.h"

// raw error messages
char *rawextractresultstrings[12] =
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
	"decompression stack overflow",
	"decormression buffer jumped out of limits",
	"wrong options given to extract"
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
		fprintf(f, "raw.doubleres=%i\n", UnparseRawSwitch(rawinfo->doubleres));
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
		return "items/";
	if (rawtype == RAW_TYPE_2)
		return "gfx/";
	if (rawtype == RAW_TYPE_3 || rawtype == RAW_TYPE_5)
		return "sprites/";
	if (rawtype == RAW_TYPE_4)
		return "sprites/";
	if (rawtype == RAW_TYPE_6)
		return "sprites/";
	if (rawtype == RAW_TYPE_7)
		return "tiles/";
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
	block->chunk[chunknum].colormap = NULL;
	block->chunk[chunknum].colormapExternal = false;
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
	for (i = 0; i < block->chunks; i++)
		RawBlockFreeChunk(block, i);
	qfree(block);
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
	if (bpp != 8 && bpp != 16 && bpp != 24)
		Error("RawTGA: bad bpp (only 8, 16 and 24 are supported)!\n");

	// lineskippers
	#define skiplines1(c) for (i = 0; i < c; i++) for (j = 0; j < realwidth; j++) { *out++ = 0; }
	#define skiplines2(c) for (i = 0; i < c; i++) for (j = 0; j < realwidth; j++) { *out++ = 0; *out++ = 0; } 
	#define skiplines3(c) for (i = 0; i < c; i++) for (j = 0; j < realwidth; j++) { *out++ = 0; *out++ = 0; *out++ = 0; }
	#define skiprows1(c) for (j = 0; j < c; j++) { *out++ = 0; }
	#define skiprows2(c) for (j = 0; j < c; j++) { *out++ = 0; *out++ = 0; }
	#define skiprows3(c) for (j = 0; j < c; j++) { *out++ = 0; *out++ = 0; *out++ = 0; }

	// create targa header
	buffer = qmalloc(pixelbytes*(int)(bpp / 8) + ((bpp == 8) ? 768 : 0) + 18);
	memset(buffer, 0, 18);
	f = SafeOpen(outfile, "wb");
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
		if (rawinfo->disableCLUT == true || colormapdata == NULL)
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
				if (rawinfo->dontSwapBgr == true)
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
			skiprows1(by)
			in = pixeldata + i * width;
			end = in + width;
			for (;in < end; in++)
				*out++ = in[0];
			skiprows1(ay)
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
			skiprows2(by)
			in = pixeldata + i * width * 2;
			end = in + width * 2;
			for (;in < end; in += 2)
			{
				// swap bgr->rgb
				if (rawinfo->dontSwapBgr == true)
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
			skiprows2(ay)
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
			skiprows3(by)
			in = pixeldata + i * width * 3;
			end = in + width * 3;
			for (;in < end; in += 3)
			{
				// swap bgr->rgb
				if (rawinfo->dontSwapBgr == true)
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
			skiprows3(ay)
		}
		skiplines3(by)
		fwrite(buffer, pixelbytes*3 + 18, 1, f);
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

void RawTGAPalette(char *outfile, const byte *colormapdata, byte bytes)
{
	rawinfo_t rawinfo;
	
	FlushRawInfo(&rawinfo);
	RawTGA(outfile, 16, 16, 0, 0, 0, 0, NULL, colormapdata, bytes, &rawinfo);
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

// read integer from buffer stream
int ReadInt(byte *buffer)
{
	return buffer[3]*16777216 + buffer[2]*65536 + buffer[1]*256 + buffer[0];
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

// perturbate colormap for halfwidth compression used for type4 and type6
void PerturbateColormapForHWCompression(byte *colormap)
{
	byte i, j;

	for (i = 1; i < 16; i++)
	for (j = 0; j < 16; j++)
	{
		/*
		rawblock->colormap[i*16*3 + pixelpos*3] = (int)(rawblock->colormap[pixelpos*3]*0.2) + (int)(rawblock->colormap[i*3]*0.8);
		rawblock->colormap[i*16*3 + pixelpos*3 + 1] = (int)(rawblock->colormap[pixelpos*3 + 1]*0.2) + (int)(rawblock->colormap[i*3 + 1]*0.8);
		rawblock->colormap[i*16*3 + pixelpos*3 + 2] = (int)(rawblock->colormap[pixelpos*3 + 2]*0.2) + (int)(rawblock->colormap[i*3 + 2]*0.8);
		*/
		colormap[i*16*3 + j*3] = colormap[i*3];
		colormap[i*16*3 + j*3 + 1] = colormap[i*3 + 1];
		colormap[i*16*3 + j*3 + 2] = colormap[i*3 + 2];
	}
}

// read run-lenght encoded stream, return startpos, error codes are < 0
int ReadRLCompressedStream(byte *outbuf, byte *inbuf, int startpos, int buflen, int readpixels, qboolean decomress255, qboolean usehalfwidthcompression, qboolean forced)
{
	int pixelpos, nullpixels;
	byte pixel, nullpixelsi;

	#define readpixel() if (startpos >= buflen) { return RAWX_ERROR_COMPRESSED_READ_OVERFLOW; } else { pixel = inbuf[startpos]; startpos++; }
	#define writepixel(f) if (pixelpos >= readpixels) {  if (!forced) return RAWX_ERROR_COMPRESSED_UNPACK_OVERFLOW; } else { if (usehalfwidthcompression) { outbuf[pixelpos] = f - (int)(f/16)*16; pixelpos++; } outbuf[pixelpos] = f; pixelpos++; }

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

/*
==========================================================================================

  RAW FILE TYPE 0
  a file with offset, width/height and BPP set externally
  thats just a useful way to "massage" files

==========================================================================================
*/

rawblock_t *RawExtract_Type0(char *basefilename, unsigned char *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int outputsize, i, chunkpos;
	byte pixel, nullpixels, nullpixelsi;
	rawblock_t *rawblock;

	// read header
	if (rawinfo->bytes != 1 && rawinfo->bytes != 2 && rawinfo->bytes != 3)
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
	outputsize = rawinfo->width*rawinfo->height*rawinfo->bytes;
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
		for (chunkpos = 0; chunkpos < filelen; )
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
	else
	{
		for (i = 0; i < filelen; i++)
			rawblock->chunk[0].pixels[i] = (buffer[i] == 255) ? 255 : 0;
	}
	return rawblock; // who care out eendpos that for type0?
}

/*
==========================================================================================

  RAW FILE TYPE 1

==========================================================================================
*/

// RAW FILE TYPE 1
// Description: item card, single-object file
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
rawblock_t *RawExtract_Type1(char *basefilename, unsigned char *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	rawblock_t *rawblock;

	if (buffer[0] != 1)
		return RawErrorBlock(NULL, RAWX_ERROR_HEADER_NOT_VALID); // header not valid
	if (buffer[784] > 120 || buffer[785] > 120 || buffer[784]*buffer[785] < 0)
		return RawErrorBlock(NULL, RAWX_ERROR_WIDTH_OR_HEIGHT_NOT_VALID); // invalid width/height
	if (forced == false && (filelen - (788 + buffer[784]*buffer[785]) > 32))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_BIGGER_THAN_REQUIRED); // file is bigger than required
	if (filelen < (781 + buffer[784]*buffer[785]))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED); // file is smaller than require

	rawblock = EmptyRawBlock(1);
	if (verbose == true)
		Print("size: %ix%i\n", buffer[784], buffer[785]);
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
// Description: multiobject file with per-object palette
// Notes: real width/height may be double than written in headers
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
rawblock_t *RawExtract_Type2(char *basefilename, unsigned char *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, i, pos1, pos2, pos, resmult;
	rawblock_t *rawblock;
	byte *chunk;

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
	if (verbose == true)
		Print("mystic: %03i %03i %03i %03i\n", buffer[4], buffer[5], buffer[6], buffer[7]);

	// print objects
	for (i = 0; i < (int)numobjects; i++)
	{
		pos = 776 + (768 + 8)*i + 768;
		if (filelen < (pos + 8))
			return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED); // file is smaller than required
		chunk = buffer + pos;
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
			pos = pos1;
		}
		else if ((filelen - pos2) < 32)
		{
			resmult = 2;
			pos = pos2;
		}
		else 
		{	
			if (forced == false)
				return RawErrorBlock(rawblock, RAWX_ERROR_FILE_BIGGER_THAN_REQUIRED);
			else // multiobject?
			{
				resmult = 2;
				pos = pos2;
			}
		}
	}	
	else if (rawinfo->doubleres == rtrue)
	{
		resmult = 2;
		pos = pos2;
	}
	else 
	{
		resmult = 1;
		pos = pos1;
	}

	// check if file is smaller than required
	if (filelen < pos)
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED); 

	// write objects
	pos = 776 + (768 + 8)*numobjects;
	for (i = 0; i < (int)numobjects; i++)
	{		
		chunk = buffer + 776 + (768 + 8)*i;
		RawBlockAllocateChunk(rawblock, i, chunk[772]*resmult, chunk[773]*resmult, 0, 0, true);
		rawblock->chunk[i].colormap = chunk;
		rawblock->chunk[i].colormapExternal = true;
		rawblock->chunk[i].pixels = buffer + pos;
		pos = pos + chunk[772]*resmult*chunk[773]*resmult;
	}
	if (testonly == true)
		rawinfo->doubleres = (resmult == 2) ? rtrue : rfalse;
	return rawblock;
}

/*
==========================================================================================

  RAW FILE TYPE 3

==========================================================================================
*/

// RAW FILE TYPE 3
// Description: multiobject file with shared palette, with zero-length compression
// Notes: unfinished
//   4 bytes - number of objects
//   4 bytes - filesize
//   768 bytes - colormap data (24-bit RGB)
//   4 mystic bytes
// <object headers>
//    4 bytes - offset after headers
//    1 byte - width
//    1 byte - height
//    1 byte - x
//    1 byte - y
// <zero-length compressed pixels data>
//    read pixel, if it's 0 - read next pixel and make this number of black pixels, otherwise write as normal pixel
rawblock_t *RawExtract_Type3(char *basefilename, byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, filesize, i, chunkpos, last;
	unsigned char *chunk;
	qboolean detect255;
	rawchunk_t *rawchunk;
	rawblock_t *rawblock;

	// check headers
	if (filelen < 780)
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);
	numobjects = ReadInt(buffer);
	if (numobjects <= 0 || numobjects > 1000)
		return RawErrorBlock(NULL, RAWX_ERROR_IMPLICIT_OBJECTS_COUNT);
	if (verbose == true)
		Print("objects: %i\n", numobjects);
	if (filelen < (780 + 8*numobjects))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// file size
	filesize = ReadInt(buffer + 4);
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

	// check last colormap pixel for blue to detect shadow pixel compression
	if (rawblock->colormap[255*3] < rawblock->colormap[255*3 + 2] || 
		max(max(rawblock->colormap[255*3], rawblock->colormap[255*3 + 1]), rawblock->colormap[255*3 + 2]) < 60)
		detect255 = true;
	else
		detect255 = false;

	// mystic bytes
	if (verbose == true)
		Print("mystic bytes: %03i %03i %03i %03i\n", buffer[776],  buffer[777],  buffer[778],  buffer[779]);
	if (filelen < (780 + 8*numobjects + 2))
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// read object headers, test them
	last = -1;
	for (i = 0; i < numobjects; i++)
	{
		chunk = buffer + 780 + i*8;

		rawchunk = &rawblock->chunk[i];
		rawchunk->offset = 780 + numobjects*8 + ReadInt(chunk);
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
			Print("%03i: %8i %03ix%03i %03i %03i\n", i, rawblock->chunk[i].offset, rawblock->chunk[i].width, rawblock->chunk[i].height, rawblock->chunk[i].x, rawblock->chunk[i].y);    
		last = rawchunk->offset;
	}

	// read pixels
	for (i = 0; i < numobjects; i++)
	{
		rawchunk = &rawblock->chunk[i];
		RawBlockAllocateChunkSimple(rawblock, i, false);
		chunkpos = ReadRLCompressedStream(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, detect255, false, forced);
		if (chunkpos < 0)
			return RawErrorBlock(rawblock, chunkpos);
	}

	// warn if multiobject file
	if (!testonly && chunkpos != filelen && verbose)
		Print("Warning! Reading has ended on %i but filelen is %i, possible multiobject file.\n", chunkpos, filelen);

	return rawblock;
}

/*
==========================================================================================

  RAW FILE TYPE 4

  DERIVATIVE FROM TYPE 3 WITH ADDITIONAL DATA

==========================================================================================
*/

// RAW FILE TYPE 4
// Description: multiobject file with shared palette, with zero-length compression, with additional objects header
// Notes: contains a special colormap-pixels compression (some objects are halfwidth, therefore 1 byte codes 2 pixels)
// there may be chunks with width or height = 0
//   4 bytes - number of objects
//   4 bytes - filesize
// structure: ? bytes - 1 byte for each object
//   768 bytes - colormap data (24-bit RGB)
//   4 mystic bytes
// structure: <object headers>
//    4 bytes - offset after headers
//    1 byte - width
//    1 byte - height
//    1 byte - x
//    1 byte - y
// <zero-length compressed pixels data>
//    read pixel, if it's 0 - read next pixel and make this number of black pixels, otherwise write as normal pixel
rawblock_t *RawExtract_Type4(char *basefilename, byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, filesize, i, objbitssize, chunkpos, last;
	rawchunk_t *rawchunk;
	rawblock_t *rawblock;
	qboolean halfres;
	byte *chunk;

	// check header
	if (filelen < 782)
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);
	numobjects = ReadInt(buffer);
	if (numobjects <= 0 || numobjects > 1000)
		return RawErrorBlock(NULL, RAWX_ERROR_IMPLICIT_OBJECTS_COUNT);
	if (verbose == true)
		Print("objects: %i\n", numobjects);
	if (filelen < (780 + 9*numobjects))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// file size
	filesize = ReadInt(buffer + 4);
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

	// VorteX: hack - check if there is many repeating indexes
	// if so, we do perturbate colormap
	chunkpos = 0;
	for (i = 1, last = 0; i < 768; i++)
	{
		if (rawblock->colormap[i] == chunkpos)
			last++;
		else
		{
			chunkpos = rawblock->colormap[i];
			last = 0;
		}
		if (last > 200)
		{
			PerturbateColormapForHWCompression(rawblock->colormap);
			break;
		}
	}

	// mystic bytes, check filelen again
	if (verbose == true)
		Print("mystic bytes: %03i %03i %03i %03i\n", buffer[776 + objbitssize], buffer[777 + objbitssize], buffer[778 + objbitssize], buffer[779 + objbitssize]);
	if (filelen < (780 + objbitssize + 8*numobjects + 2))
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// read object headers, test them
	last = -1;
	for (i = 0; i < numobjects; i++)
	{
		chunk = buffer + 780 + objbitssize + i*8;

		rawchunk = &rawblock->chunk[i];
		rawchunk->offset = 780 + objbitssize + numobjects*8 + ReadInt(chunk);
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
			Print("%03i: %8i %03ix%03i %03i %03i - %03i\n", i, rawblock->chunk[i].offset, rawblock->chunk[i].width, rawblock->chunk[i].height, rawblock->chunk[i].x, rawblock->chunk[i].y, rawblock->chunk[i].flagbit);    
		last = rawchunk->offset;
	}

	// VorteX: detect half-width compression, by reading first entry and checking offsets
	rawchunk = &rawblock->chunk[0];
	chunkpos = ReadRLCompressedStreamTest(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, true, true, forced);
	if (chunkpos == rawblock->chunk[1].offset)
		halfres = true;
	else
		halfres = false;

	// read pixels
	for (i = 0; i < numobjects; i++)
	{
		rawchunk = &rawblock->chunk[i];
		RawBlockAllocateChunkSimple(rawblock, i, false);
		chunkpos = ReadRLCompressedStream(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, true, halfres, forced);
		if (chunkpos < 0)
			return RawErrorBlock(rawblock, chunkpos);
	}

	// warn if multiobject file
	if (!testonly && chunkpos != filelen && verbose)
		Print("Warning! Reading has ended on %i but filelen is %i, possible multiobject file.\n", chunkpos, filelen);

	return rawblock;
}

/*
==========================================================================================

  RAW FILE TYPE 5

  DERIVATIVE FROM TYPE 3 BUT NO MYSTIC BYTES

==========================================================================================
*/

// RAW FILE TYPE 5
// Description: multiobject file with shared palette, with zero-length compression, with no "mystic" bytes right after colormap
// Format structure:
//   4 bytes - number of objects
//   4 bytes - filesize
//   768 bytes - colormap data (24-bit RGB)
// structure: <object headers>
//    4 bytes - offset after headers
//    1 byte - width
//    1 byte - height
//    1 byte - x
//    1 byte - y
// <zero-length compressed pixels data>
//    read pixel, if it's 0 - read next pixel and make this number of black pixels, otherwise write as normal pixel
rawblock_t *RawExtract_Type5(char *basefilename, byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, filesize, i, chunkpos, last;
	qboolean detect255;
	rawchunk_t *rawchunk;
	rawblock_t *rawblock;
	byte *chunk;

	// check header
	if (filelen < 776)
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);
	numobjects = ReadInt(buffer);
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
	
	// check last colormap pixel for blue to detect shadow pixel compression
	if (rawblock->colormap[255*3] < rawblock->colormap[255*3 + 2] || 
		max(max(rawblock->colormap[255*3], rawblock->colormap[255*3 + 1]), rawblock->colormap[255*3 + 2]) < 60)
		detect255 = true;
	else
		detect255 = false;
	if (filelen < (776 + 8*numobjects + 2))
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	// read object headers, test them
	last = -1;
	for (i = 0; i < numobjects; i++)
	{
		chunk = buffer + 776 + i*8;

		rawchunk = &rawblock->chunk[i];
		rawchunk->offset = 776 + numobjects*8 + ReadInt(chunk);
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
			Print("%03i: %8i %03ix%03i %03i %03i - %03i\n", i, rawblock->chunk[i].offset, rawblock->chunk[i].width, rawblock->chunk[i].height, rawblock->chunk[i].x, rawblock->chunk[i].y, rawblock->chunk[i].flagbit);    
		last = rawchunk->offset;
	}

	// read pixels
	for (i = 0; i < numobjects; i++)
	{
		rawchunk = &rawblock->chunk[i];
		RawBlockAllocateChunkSimple(rawblock, i, false);
		chunkpos = ReadRLCompressedStream(rawchunk->pixels, buffer, rawchunk->offset, filelen, rawchunk->size, detect255, false, forced);
		if (chunkpos < 0)
			return RawErrorBlock(rawblock, chunkpos);
	}

	// warn if multiobject file
	if (!testonly && chunkpos != filelen && verbose)
		Print("Warning! Reading has ended on %i but filelen is %i, possible multiobject file.\n", chunkpos, filelen);

	return rawblock;
}

/*
==========================================================================================

  RAW FILE TYPE 6

  HYBRID OF TYPE 3 AND TYPE 5, REALLY IT's TYPE 5 WITH 4 MYSTIC BYTES AFTER COLORMAP
  UNLIKE TYPE 3 IT HAS COLORMAP REPEATED (ONLY 16 COLORS ARE FILLED, REST ARE 248 WHITE)
  + WIDTH SHOULD BE HALFED

==========================================================================================
*/

// RAW FILE TYPE 6
// Description: multiobject file with shared palette, with zero-length compression
// evil hybrid of type3,type4,type5, really it's type 5 with 4 mystic bytes after colormap and repeated colormap
// unlike type 5 (and like type4) it has colormap repeated (only 16 colors are filled, rest are 248 white)
// unlike type 3 (and like type4) width should be halfed
// Notes: contains a special colormap-pixels compression (some objects are halfwidth, therefore 1 byte codes 2 pixels)
// Spec:
//   4 bytes - number of objects
//   4 bytes - filesize
//   768 bytes - colormap data (24-bit RGB)
//   4 bytes - mystic bytes
// structure: <object headers>
//    4 bytes - offset after headers
//    1 byte - width
//    1 byte - height
//    1 byte - x
//    1 byte - y
// <zero-length compressed pixels data>
//    read pixel, if it's 0 - read next pixel and make this number of black pixels, otherwise write as normal pixel
rawblock_t *RawExtract_Type6(char *basefilename, byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, filesize;
	int	i, objwidth, objheight, objsize, objoffset, pixelpos, chunkpos, lastoffset, nullpixels, nullpixelsi;
	unsigned char *chunk;
	qboolean detect255;
	byte *objects;
	byte mystic[4];
	byte pixel;
	rawblock_t *rawblock;

	// check headers
	if (filelen < 780)
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);
	numobjects = buffer[3]*16777216 + buffer[2]*65536 + buffer[1]*256 + buffer[0];
	if (numobjects <= 0 || numobjects > 1000)
		return RawErrorBlock(NULL, RAWX_ERROR_IMPLICIT_OBJECTS_COUNT);
	if (verbose == true)
		Print("objects: %i\n", numobjects);
	if (filelen < (780 + 8*numobjects))
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

	// perturbate colormap
	for (i = 1; i < 16; i++)
	{
		for (pixelpos = 0; pixelpos < 16; pixelpos++)
		{
			/*
			rawblock->colormap[i*16*3 + pixelpos*3] = (int)(rawblock->colormap[pixelpos*3]*0.2) + (int)(rawblock->colormap[i*3]*0.8);
			rawblock->colormap[i*16*3 + pixelpos*3 + 1] = (int)(rawblock->colormap[pixelpos*3 + 1]*0.2) + (int)(rawblock->colormap[i*3 + 1]*0.8);
			rawblock->colormap[i*16*3 + pixelpos*3 + 2] = (int)(rawblock->colormap[pixelpos*3 + 2]*0.2) + (int)(rawblock->colormap[i*3 + 2]*0.8);
			*/
			rawblock->colormap[i*16*3 + pixelpos*3] = rawblock->colormap[i*3];
			rawblock->colormap[i*16*3 + pixelpos*3 + 1] = rawblock->colormap[i*3 + 1];
			rawblock->colormap[i*16*3 + pixelpos*3 + 2] = rawblock->colormap[i*3 + 2];
		}
	}

	// check last colormap pixel for blue to detect shadow pixel compression
	if (rawblock->colormap[255*3] < rawblock->colormap[255*3 + 2])
		detect255 = true;
	else
		detect255 = false;

	// mystic bytes 2
	mystic[0] = buffer[776];
	mystic[1] = buffer[777];
	mystic[2] = buffer[778];
	mystic[3] = buffer[779];
	if (verbose == true)
		Print("mystic bytes: %03i %03i %03i %03i\n", mystic[0], mystic[1], mystic[2], mystic[3]);

	if (filelen < (780 + 8*numobjects + 2))
		return RawErrorBlock(rawblock, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED); // file is smaller than required

	// read object headers
	objects = qmalloc(8*numobjects);
	for (i = 0; i < 8*numobjects; i++)
		objects[i] = buffer[780 + i];

	// print object headers
	if (verbose == true)
	{
		for (i = 0; i < numobjects; i++)
		{
			chunk = objects + i*8;
			objoffset = chunk[3]*16777216 + chunk[2]*65536 + chunk[1]*256 + chunk[0];
			Print("%03i: %08i %03ix%03i %03i\n", i, objoffset, chunk[4], chunk[5], chunk[6], chunk[7]);    
		}
	}

	// test objects, check width and height, offset
	lastoffset = -1;
	for (i = 0; i < numobjects; i++)
	{
		chunk = objects + i*8;

		objwidth = objects[i*8 + 4];
		objheight = objects[i*8 + 5];
		objoffset = chunk[3]*16777216 + chunk[2]*65536 + chunk[1]*256 + chunk[0];
		if (objwidth*objheight <= 0)
			return RawErrorBlock(rawblock, RAWX_ERROR_BAD_OBJECT_HEADER); // bad objects header
		if (objoffset <= lastoffset && !forced)
			return RawErrorBlock(rawblock, RAWX_ERROR_BAD_OBJECT_OFFSET); // bad offsets
		lastoffset = objoffset;
	}

	// read pixels
	nullpixels = 0;
	#define writepixel(f) if (pixelpos >= objsize) { if (!forced) return RawErrorBlock(rawblock, RAWX_ERROR_COMPRESSED_READ_OVERFLOW); } else { rawblock->chunk[i].pixels[pixelpos]=f - (int)(f/16)*16;pixelpos++; rawblock->chunk[i].pixels[pixelpos]=f;pixelpos++; }
	#define readpixel() if (chunkpos >= filelen) { return RawErrorBlock(rawblock, RAWX_ERROR_COMPRESSED_UNPACK_OVERFLOW); } else { pixel=buffer[chunkpos];chunkpos++; }
	for (i = 0; i < numobjects; i++)
	{
		chunk = objects + i*8;
		objoffset = chunk[3]*16777216 + chunk[2]*65536 + chunk[1]*256 + chunk[0];
		objwidth = objects[i*8 + 4];
		objheight = objects[i*8 + 5];
		objsize = objwidth*objheight;
		chunkpos = 780 + numobjects*8 + objoffset; 
		RawBlockAllocateChunk(rawblock, i, objwidth, objheight, objects[i*8 + 6], objects[i*8 + 7], false);

		// begin frame
		for(pixelpos = 0; pixelpos < objsize; )
		{
			// fill with nulls
			while(nullpixels > 0)
			{
				if (pixelpos >= objsize)
					break;
				writepixel(nullpixelsi); 
				nullpixels--;
			}
			if (pixelpos >= objsize)
				break;
			// read pixel
			readpixel();
			if (!pixel || (detect255 && pixel == 255))
			{	
				nullpixelsi = pixel;
				readpixel();
				nullpixels = pixel;
				continue;
			}
			writepixel(pixel);
		}
	}
	#undef writepixel
	#undef readpixel

	// warn if multiobject file
	if (!testonly && chunkpos != filelen && verbose)
		Print("Warning! Reading has ended on %i but filelen is %i, possible multiobject file.\n", chunkpos, filelen);

	qfree(objects);
	return rawblock;
}

/*
==========================================================================================

  TYPE 7 - TILE

==========================================================================================
*/

rawblock_t *RawExtract_Type7(char *basefilename, byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	unsigned int datasize;
	unsigned short data1, data2, data3, data4, data5;
	int chunkpos, i;
	byte *colormapdata;
	byte *in, *out;
	char name[MAX_BLOODPATH];

	return RawErrorBlock(NULL, -1);

	if (filelen < (26 + 512 + 32))
		return RawErrorBlock(NULL, RAWX_ERROR_FILE_SMALLER_THAN_REQUIRED);

	datasize = buffer[3]*16777216 + buffer[2]*65536 + buffer[1]*256 + buffer[0];
	data1 = buffer[7]*16777216 + buffer[6]*65536 + buffer[5]*256 + buffer[4];
	data2 = buffer[11]*16777216 + buffer[10]*65536 + buffer[9]*256 + buffer[8];
	data3 = buffer[15]*16777216 + buffer[14]*65536 + buffer[13]*256 + buffer[12];
	data4 = buffer[19]*16777216 + buffer[18]*65536 + buffer[17]*256 + buffer[16];
	data5 = buffer[23]*16777216 + buffer[22]*65536 + buffer[21]*256 + buffer[20];
	if (verbose == true)
	{
		Print("datasize: %i\n", datasize);
		Print("filesize: %i\n", filelen);
		Print("data1: %i\n", data1);
		Print("data2: %i\n", data2);
		Print("data3: %i\n", data3);
		Print("data4: %i\n", data4);
		Print("data5: %i\n", data5);
		Print("more: %i %i\n", buffer[24], buffer[25]);
	}

	// read colormap
	colormapdata = qmalloc(768);
	in = buffer + 26;
	out = colormapdata;
	for (chunkpos = 0; chunkpos < 256; chunkpos += 4)
	{
		for (i = 0; i < 4; i++)
		{
			*out++ = (in[0] & 0x1F) * 8; 
			*out++ = (((in[0] & 0xE0) >> 5) + ((in[1] & 0x3) << 3)) * 8;
			*out++ = ((in[1] & 0x7C) >> 2) * 8;
			in += 2;
		}
		in++;
	}

	sprintf(name, "%s_palette.tga", basefilename);
	RawTGAPalette(name, colormapdata, 24);

	in = buffer + 26 + 544;
	sprintf(name, "%s.tga", basefilename);
	RawTGA(name, 41, 140, 0, 0, 0, 0, colormapdata, in, 8, rawinfo);

	return RawErrorBlock(NULL, -1);
}

/*
==========================================================================================

  UNFINISHED RAW FILE TYPES

==========================================================================================
*/

rawblock_t *RawExtract_Type8(char *basefilename, byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	return RawErrorBlock(NULL, -1);
}

/*
==========================================================================================

  MAIN

==========================================================================================
*/

int RawExtract(char *basefilename, char *filedata, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, rawtype_t forcetype, qboolean rawnoalign)
{
	char name[MAX_BLOODPATH];
	rawtype_t rawtype, testtype;
	rawblock_t *rawblock;
	int code, i, maxwidth, maxheight;

	rawtype = (forcetype == RAW_TYPE_UNKNOWN) ? rawinfo->type : forcetype;
	rawblock = NULL;

	// pass or autoscan default types
	if (rawtype == RAW_TYPE_0) { rawblock = RawExtract_Type0(basefilename, filedata, filelen, rawinfo, testonly, verbose, false); goto end; }
	#define trytype(t,f)	if (rawtype == RAW_TYPE_UNKNOWN || rawtype == t) { testtype = t; rawblock = f(basefilename, filedata, filelen, rawinfo, testonly, verbose, false); if (rawblock->errorcode >= 0 || rawtype != RAW_TYPE_UNKNOWN) goto end; FreeRawBlock(rawblock); rawblock = NULL; }
	trytype(RAW_TYPE_1, RawExtract_Type1)
	trytype(RAW_TYPE_2, RawExtract_Type2)
	trytype(RAW_TYPE_4, RawExtract_Type4) 	// VorteX: scan type4 and type5 before type3, because they are derivations from type3
	trytype(RAW_TYPE_5, RawExtract_Type5)
	trytype(RAW_TYPE_3, RawExtract_Type3)
//	trytype(RAW_TYPE_6, RawExtract_Type6)
//	trytype(RAW_TYPE_7, RawExtract_Type7)
//	trytype(RAW_TYPE_8, RawExtract_Type8)
	#undef trytype
end:
	if (rawblock == NULL)
		return -999;
	if (rawblock->errorcode < 0 && verbose)
		Print("Raw error code %i: %s\n", rawblock->errorcode, RawStringForResult(rawblock->errorcode));
	// export
	code = rawblock->errorcode;
	if (testonly)
		rawinfo->type = testtype;
	else if (rawblock->chunks && code >= 0)
	{
		// detect maxwidth/maxheight for alignment
		maxwidth = maxheight = 0;
		for (i = 0; i < rawblock->chunks; i++)
		{
			maxwidth = max(maxwidth, rawblock->chunk[i].width + rawblock->chunk[i].x);
			maxheight = max(maxheight, rawblock->chunk[i].height + rawblock->chunk[i].y);
		}
		// export all chunks
		for (i = 0; i < rawblock->chunks; i++)
		{
			if (rawinfo->chunknum != -1 && i != rawinfo->chunknum)
				continue; // skip this chunk
			if (rawblock->chunks == 1)
				sprintf(name, "%s.tga", basefilename);
			else
				sprintf(name, "%s_%03i.tga", basefilename, i);
			if (verbose == true)
				Print("writing %s.\n", name);
			if (rawnoalign)
				RawTGA(name, rawblock->chunk[i].width, rawblock->chunk[i].height, 0, 0, 0, 0, rawblock->chunk[i].colormap ? rawblock->chunk[i].colormap : rawblock->colormap, rawblock->chunk[i].pixels, 8, rawinfo);
			else
				RawTGA(name, rawblock->chunk[i].width, rawblock->chunk[i].height, rawblock->chunk[i].x, rawblock->chunk[i].y, max(0, maxwidth - rawblock->chunk[i].width - rawblock->chunk[i].x), max(0, maxheight - rawblock->chunk[i].height - rawblock->chunk[i].y), rawblock->chunk[i].colormap ? rawblock->chunk[i].colormap : rawblock->colormap, rawblock->chunk[i].pixels, 8, rawinfo);
		 }
	}
	FreeRawBlock(rawblock);
	return code;
}
	
int Raw_Main(int argc, char **argv)
{
	char filename[MAX_BLOODPATH], basefilename[MAX_BLOODPATH], outfile[MAX_BLOODPATH], *c;
	unsigned char *filedata;
	int filelen;
	qboolean forced, rawnoalign;
	rawinfo_t rawinfo;
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
				rawinfo.bytes = atoi(argv[i]);
			Verbose("Color bytes: %i\n", rawinfo.bytes);
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
	StripFileExtension(outfile, basefilename);
	filelen = LoadFile(filename, &filedata);
	RawExtract(basefilename, filedata, filelen, &rawinfo, false, true, (forced == true) ? rawinfo.type : RAW_TYPE_UNKNOWN, rawnoalign);
	qfree(filedata);
	Print("done.\n");
	return 0;
}
