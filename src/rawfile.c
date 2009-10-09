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

#include "bloodpill.h"
#include "rawfile.h"
#include "cmdlib.h"
#include "mem.h"

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
	if (!strcmp(str, "0") || !strcmp(str, "raw"))
		return RAW_TYPE_0;
	if (!strcmp(str, "1"))
		return RAW_TYPE_1;
	if (!strcmp(str, "2"))
		return RAW_TYPE_2;
	if (!strcmp(str, "3"))
		return RAW_TYPE_3;
	if (!strcmp(str, "4"))
		return RAW_TYPE_4;
	return RAW_TYPE_UNKNOWN;
}

char *UnparseRawType(rawtype_t rawtype)
{
	if (rawtype == RAW_TYPE_0)
		return "raw";
	if (rawtype == RAW_TYPE_1)
		return "1";
	if (rawtype == RAW_TYPE_2)
		return "2";
	if (rawtype == RAW_TYPE_3)
		return "3";
	if (rawtype == RAW_TYPE_4)
		return "4";
	return "unknown";
}

/*
==========================================================================================

  RAW FILE TGA EXPORTER

==========================================================================================
*/

void RawTGA(char *outfile, int width, int height, const char *colormapdata, int pixelbytes, const char *pixeldata, int bpp, rawinfo_t *rawinfo)
{
	unsigned char *buffer, *out;
	const unsigned char *in, *end;
	int i;
	FILE *f;

	// check bpp
	if (bpp != 8 && bpp != 16 && bpp != 24)
		Error("RawTGA: bad bpp (only 8, 16 and 24 are supported)!\n");

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
		buffer[12] = (width >> 0) & 0xFF;
		buffer[13] = (width >> 8) & 0xFF;
		buffer[14] = (height >> 0) & 0xFF;
		buffer[15] = (height >> 8) & 0xFF;
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
		for (i = height - 1;i >= 0;i--)
		{
			in = pixeldata + i * width;
			end = in + width;
			for (;in < end; in++)
				*out++ = in[0];
		}
		fwrite(buffer, pixelbytes + 768 + 18, 1, f);
	}
	else if (bpp == 16)
	{
		buffer[2] = 2; // uncompressed
		buffer[12] = (width >> 0) & 0xFF;
		buffer[13] = (width >> 8) & 0xFF;
		buffer[14] = (height >> 0) & 0xFF;
		buffer[15] = (height >> 8) & 0xFF;
		buffer[16] = 16;
		// flip upside down, write
		out = buffer + 18;
		for (i = height - 1;i >= 0;i--)
		{
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
		}
		fwrite(buffer, pixelbytes*2 + 18, 1, f);
	}
	else if (bpp == 24)
	{
		buffer[2] = 2; // uncompressed
		buffer[12] = (width >> 0) & 0xFF;
		buffer[13] = (width >> 8) & 0xFF;
		buffer[14] = (height >> 0) & 0xFF;
		buffer[15] = (height >> 8) & 0xFF;
		buffer[16] = 24;
		// flip upside down, write
		out = buffer + 18;
		for (i = height - 1;i >= 0;i--)
		{
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
		}
		fwrite(buffer, pixelbytes*3 + 18, 1, f);
	}
	fclose(f);
	qfree(buffer);
}

void RawTGAPalette(char *outfile, const byte *colormapdata)
{
	rawinfo_t rawinfo;
	
	FlushRawInfo(&rawinfo);
	RawTGA(outfile, 16, 16, NULL, 256, colormapdata, 24, &rawinfo);
}

/*
==========================================================================================

  RAW FILE TYPE 0
  a file with offset, width/height and BPP set externally
  thats just a useful way to "massage" files

==========================================================================================
*/

static byte *RawReadColormap(unsigned char *buffer, int filelen, int offset, int palbytes)
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

int RawExtract_Type0(char *basefilename, unsigned char *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	char name[MAX_BLOODPATH];
	int outputsize, i;
	byte *data, *colormapdata;

	if (rawinfo->bytes != 1 && rawinfo->bytes != 2 && rawinfo->bytes != 3)
		return -1; // bad bytes
	if (rawinfo->width*rawinfo->height < 0)
		return -2; // bad width/height
	if (rawinfo->offset < 0)
		return -3; // bad offset

	// get colormap data
	colormapdata = NULL;
	if (rawinfo->colormapbytes)
		colormapdata = RawReadColormap(buffer, filelen, rawinfo->colormapoffset, rawinfo->colormapbytes);

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

	// testing only?
	if (testonly == true)
	{
		rawinfo->type = RAW_TYPE_0;
		return 1;
	}

	// make pixel bytes
	if (filelen > outputsize)
		filelen = outputsize;
	data = qmalloc(outputsize);
	memset(data, 0, outputsize);
	for (i = 0; i < filelen; i++)
		data[i] = buffer[i];

	// write file
	sprintf(name, "%s.tga", basefilename);
	if (verbose == true)
		Print("writing %s\n", name);
	RawTGA(name, rawinfo->width, rawinfo->height, colormapdata, rawinfo->width*rawinfo->height, data, (int)8*rawinfo->bytes, rawinfo);
	if (colormapdata != NULL)
		qfree(colormapdata);
	qfree(data);
	return 1;
}

/*
==========================================================================================

  RAW FILE TYPE 1
  item card, single-object file

==========================================================================================
*/

// 000: 4 bytes - number of objects
// 004: 4 bytes - filesize
// 008: 768 bytes - colormap data (24-bit RGB)
// 776: 4 mystic bytes
// 784: 1 byte width
// 785: 1 byte height
// 786: 1 byte x
// 787: 1 byte y
// 788: pixels width*height
int RawExtract_Type1(char *basefilename, unsigned char *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	char name[MAX_BLOODPATH];
	unsigned char *chunk, *chunk2;

	if (buffer[0] != 1)
		return -1; // header not valid
	if (buffer[784] > 120 || buffer[785] > 120 || buffer[784]*buffer[785] < 0)
		return -2; // invalid width/height
	if (forced == false && (filelen - (788 + buffer[784]*buffer[785]) > 32))
		return -3; // file is bigger than required
	if (filelen < (781 + buffer[784]*buffer[785]))
		return -4; // file is smaller than required

	if (verbose == true)
		Print("size: %ix%i\n", buffer[784], buffer[785]);

	// if we only testing
	if (testonly == true)
	{
		rawinfo->type = RAW_TYPE_1;
		return 1;
	}

	// write
	chunk = buffer + 8; // colormap
	chunk2 = buffer + 788; // pixels
	sprintf(name, "%s.tga", basefilename);
	if (verbose == true)
		Print("writing %s\n", name);
	RawTGA(name, buffer[784], buffer[785], chunk, buffer[784]*buffer[785], chunk2, 8, rawinfo);
	return 1;
}

/*
==========================================================================================

  RAW FILE TYPE 2
  multiobject file with per-object palette

==========================================================================================
*/

// 4 bytes - number of objects
// 768 bytes - ??? (colormap data)
// 4 bytes - ???
// object headers:
//   768 bytes - colormap
//   8 bytes - ? ? ? ? width height ? ?
// object pixels:
//   realwidth = width * 2
//   realheight = height * 2
//   realwidth * realheight bytes - indexes into colormap
// function returns objects exported (or texted), -1, -2 etc. - error codes
int RawExtract_Type2(char *basefilename, unsigned char *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	char name[MAX_BLOODPATH];
	unsigned int numobjects;
	unsigned char *chunk, *chunk2;
	int i, pos1, pos2, pos, resmult;

	// number of objects
	numobjects = buffer[1] * 256 + buffer[0];
	if (verbose == true)
		Print("tag: %i\n", numobjects);

	// verify header
	if (buffer[2] != 0 || buffer[3] != 0)
		return -1;
	if (numobjects <= 0 || numobjects > 200)
		return -2;

	// unknown info (768 bytes of colormap and 4 mystic bytes)
	if (filelen < 776)
		return -4; // file is smaller than required
	if (verbose == true)
		Print("mystic: %3i %3i %3i %3i\n", buffer[772], buffer[773], buffer[774], buffer[775]);

	// print objects
	for (i = 0; i < (int)numobjects; i++)
	{
		pos = 776 + (768 + 8)*i + 768;
		if (filelen < (pos + 8))
			return -4; // file is smaller than required
		chunk = buffer + pos;
		if (verbose == true)
			Print("object %3i: %3i %3i %3i %3i %3i %3i %3i %3i\n", i + 1, chunk[0], chunk[1], chunk[2], chunk[3], chunk[4], chunk[5], chunk[6], chunk[7]);
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
				return -3; // file is bigger than required
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
		return -4; 

	// if we only testing
	if (testonly == true)
	{
		rawinfo->type = RAW_TYPE_2;
		rawinfo->doubleres = (resmult == 2) ? rtrue : rfalse;
		return i;
	}

	// write objects
	pos = 776 + (768 + 8)*numobjects;
	for (i = 0; i < (int)numobjects; i++)
	{		
		chunk = buffer + 776 + (768 + 8)*i; // colormap
		chunk2 = buffer + pos;
		// write, reject chunk if chunknum is set
		if (rawinfo->chunknum < 0 || i == rawinfo->chunknum)
		{
			sprintf(name, "%s_%03i.tga", basefilename, i);
			if (verbose == true)
				Print("writing %s\n", name);
			RawTGA(name, chunk[772]*resmult, chunk[773]*resmult, chunk, chunk[772]*resmult*chunk[773]*resmult, chunk2, 8, rawinfo);
		}
		// advance
		pos = pos + chunk[772]*resmult*chunk[773]*resmult;
	}
	return i;
}

/*
==========================================================================================

  RAW FILE TYPE 3

==========================================================================================
*/

// 4 bytes - number of objects
// 4 bytes - filesize
// 768 bytes - colormap data (24-bit RGB)
// 4 unknown bytes
// object headers:
//  4 bytes - offset after headers
//  1 byte - width
//  1 byte - height
//  1 byte - x
//  1 byte - y
// For each object:
//  x bytes - black-length compressed pixels:
//   read pixel, if it's 0 - read next pixel and make this number of black pixels, otherwise write as normal pixel
int RawExtract_Type3(char *basefilename, byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	int numobjects, filesize;
	int	i, objwidth, objheight, objsize, objoffset, pixelpos, chunkpos, lastoffset, nullpixels;
	unsigned char *chunk;
	byte *colormapdata;
	byte *objects;
	byte *pixels;
	byte mystic[4];
	byte pixel, p, rp;
	char name[MAX_BLOODPATH];

	if (filelen < 780)
		return -1; // file is smaller than required

	// number of objects
	numobjects = buffer[3]*16777216 + buffer[2]*65536 + buffer[1]*256 + buffer[0];
	if (numobjects <= 0 || numobjects > 400)
		return -2;
	if (verbose == true)
		Print("objects: %i\n", numobjects);

	// check if there is enough place for headers
	if (filelen < (780 + 8*numobjects))
		return -1; // file is smaller than required

	// file size
	filesize = buffer[7]*16777216 + buffer[6]*65536 + buffer[5]*256 + buffer[4];
	if (verbose == true)
	{
		Print("filesize: %i\n", filesize);
		Print("realfilesize: %i\n", filelen);
	}

	// colormap
	colormapdata = RawReadColormap(buffer, filelen, 8, 3);
	if (colormapdata == NULL)
		return -3;

	// mystic bytes
	mystic[0] = buffer[776];
	mystic[1] = buffer[777];
	mystic[2] = buffer[778];
	mystic[3] = buffer[779];
	if (verbose == true)
		Print("mystic bytes: %i %i %i %i\n", mystic[0], mystic[1], mystic[2], mystic[3]);

	if (filelen < (780 + 8*numobjects + 2))
		return -1; // file is smaller than required

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
			Print("%3i: %8i %3i %3i %3i\n", i, objoffset, chunk[4], chunk[5], chunk[6], chunk[7]);    
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
			return -4; // bad objects header
		if (objoffset <= lastoffset && !forced)
			return -5; // bad offsets
		lastoffset = objoffset;
	}

	// read pixels
	nullpixels = 0;
	p = 0;
	rp = 0;
	colormapdata[3] = 0;
	colormapdata[3+1] = 254;
	colormapdata[3+2] = 254;
	colormapdata[6] = 254;
	colormapdata[6+1] = 0;
	colormapdata[6+2] = 254;
	#define writepixel(f) if (pixelpos >= objsize) { if (!forced) return -6; } else { pixels[pixelpos]=f;pixelpos++; }
	#define readpixel() if (chunkpos >= filelen) { return -7; } else { pixel=buffer[chunkpos];chunkpos++; }
	for (i = 0; i < numobjects; i++)
	{
		if (rawinfo->chunknum >= 0 && i != rawinfo->chunknum)
			continue; // skip this chunk

		chunk = objects + i*8;
		objoffset = chunk[3]*16777216 + chunk[2]*65536 + chunk[1]*256 + chunk[0];
		objwidth = objects[i*8 + 4];
		objheight = objects[i*8 + 5];
		objsize = objwidth*objheight;
		pixels = qmalloc(objsize);
		memset(pixels, 255, objsize);
		chunkpos = 780 + numobjects*8 + objoffset; 

		// begin frame
		for(pixelpos = 0; pixelpos < objsize; )
		{
			// fill with nulls
			while(nullpixels > 0)
			{
				if (pixelpos >= objsize)
					break;
				writepixel(rp); 
				nullpixels--;
			}
			if (pixelpos >= objsize)
				break;
			// read pixel
			readpixel();
			if (!pixel)
			{	
				readpixel();
				Print("Null %i\n", pixel);
				if (!rp)
					rp = 2;
				else if (rp == 2)
					rp = 1;
				else if (rp == 1)
					rp = 2;
				p++;
				if (p == 12)
					pixel = 0;
				if (p == 14)
					break;
				nullpixels = pixel;
				continue;
			}
			writepixel(pixel);
			Print("Write %i\n", pixel);
		}
		// write image file
		if (testonly == false)
		{
			sprintf(name, "%s_%03i.tga", basefilename, i);
			RawTGA(name, objwidth, objheight, colormapdata, objsize, pixels, 8, rawinfo);
		}
		Print("endpos: %i\n", chunkpos - (780 + numobjects*8));
		qfree(pixels);
	}
	#undef writepixel
	#undef readpixel

	if (testonly == true)
		rawinfo->type = RAW_TYPE_3;
	else if (chunkpos != filelen && verbose) // warn if multiobject file
		Print("Warning! REading has ended on %i but filelen is %i, possible multiobject file.\n", chunkpos, filelen);

	qfree(objects);
	return numobjects;
}

/*
==========================================================================================

  RAW FILE TYPE 4

  Derivative work from Type3

==========================================================================================
*/

int RawExtract_Type4(char *basefilename, byte *buffer, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, qboolean forced)
{
	return -1;
}

/*
==========================================================================================

  MAIN

==========================================================================================
*/

int RawExtract(char *basefilename, char *filedata, int filelen, rawinfo_t *rawinfo, qboolean testonly, qboolean verbose, rawtype_t forcetype)
{
	rawtype_t rawtype;
	int code;

	rawtype = (forcetype == RAW_TYPE_UNKNOWN) ? rawinfo->type : forcetype;

	// try all
	if (rawtype == RAW_TYPE_UNKNOWN)
	{
		// type 1
		if ((code = RawExtract_Type1(basefilename, filedata, filelen, rawinfo, testonly, verbose, false)) >= 0)
			return code;
		// type 2
		if ((code = RawExtract_Type2(basefilename, filedata, filelen, rawinfo, testonly, verbose, false)) >= 0)
			return code;
		// type 3
		if ((code = RawExtract_Type3(basefilename, filedata, filelen, rawinfo, testonly, verbose, false)) >= 0)
			return code;
		// type 4
		if ((code = RawExtract_Type4(basefilename, filedata, filelen, rawinfo, testonly, verbose, false)) >= 0)
			return code;
		return -999;
	}

	// try certain
	code = -999;
	if (rawtype == RAW_TYPE_0)
		code = RawExtract_Type0(basefilename, filedata, filelen, rawinfo, testonly, verbose, (forcetype == RAW_TYPE_UNKNOWN) ? false : true);
	else if (rawtype == RAW_TYPE_1)
		code = RawExtract_Type1(basefilename, filedata, filelen, rawinfo, testonly, verbose, (forcetype == RAW_TYPE_UNKNOWN) ? false : true);
	else if (rawtype == RAW_TYPE_2)
		code = RawExtract_Type2(basefilename, filedata, filelen, rawinfo, testonly, verbose, (forcetype == RAW_TYPE_UNKNOWN) ? false : true);
	else if (rawtype == RAW_TYPE_3)
		code = RawExtract_Type3(basefilename, filedata, filelen, rawinfo, testonly, verbose, (forcetype == RAW_TYPE_UNKNOWN) ? false : true);
	else if (rawtype == RAW_TYPE_4)
		code = RawExtract_Type4(basefilename, filedata, filelen, rawinfo, testonly, verbose, (forcetype == RAW_TYPE_UNKNOWN) ? false : true);
	if (code < 0 && verbose == true)
		Print("Raw error: code %i\n", code);

	return code;
}
	
int Raw_Main(int argc, char **argv)
{
	char filename[MAX_BLOODPATH], basefilename[MAX_BLOODPATH], outfile[MAX_BLOODPATH], *c;
	unsigned char *filedata;
	int filelen;
	qboolean forced;
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
	}

	// open and extract file
	StripFileExtension(outfile, basefilename);
	filelen = LoadFile(filename, &filedata);
	RawExtract(basefilename, filedata, filelen, &rawinfo, false, true, (forced == true) ? rawinfo.type : RAW_TYPE_UNKNOWN);
	qfree(filedata);
	Print("done.\n");
	return 0;
}
