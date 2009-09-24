////////////////////////////////////////////////////////////////
//
// Blood Omen RAW files loader/writer
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

/*
==========================================================================================

  RAW FILE TGA EXPORTER

==========================================================================================
*/

void RawTGA(char *outfile, int width, int height, const char *colormapdata, int pixelbytes, const char *pixeldata, int bpp, qboolean noswap)
{
	unsigned char *buffer, *out;
	const unsigned char *in, *end;
	int i;
	FILE *f;

	// check bpp
	if (bpp != 8 && bpp != 16 && bpp != 24)
		Error("RawTGA: bad bpp (only 8, 16 and 24 are supported)!\n");

	Print("writing %s\n", outfile);
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
		if (colormapdata == NULL)
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
				if (noswap == true)
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
				if (noswap == true)
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
				if (noswap == true)
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

/*
==========================================================================================

  RAW FILE TYPE 0
  a file with offset, width/height and BPP set externally
  thats just a useful way to "massage" files

==========================================================================================
*/

void RawExtract_Type0(char *basefilename, char *outfile, FILE *f, int width, int height, int offset, int bytes, qboolean noswap)
{
	unsigned char *data;
	char name[MAX_BLOODPATH];

	Verbose("== raw object ==\n");

	if (bytes != 1 && bytes != 2 && bytes != 3)
		Error("Bad bytes %i!\n", bytes); 

	if (width*height < 0)
		Error("Bad width/height\n"); 

	// load file contents
	fseek(f, offset, SEEK_SET);
	data = qmalloc(width*height*bytes);
	fread(data, width*height*bytes, 1, f);

	// write file
	sprintf(name, "%s.tga", outfile);
	RawTGA(name, width, height, NULL, width*height, data, (int)8*bytes, noswap);
	qfree(data);
}

// multiobject RAW tim
// 4 bytes - number of objects
// 4 bytes - filesize
// 768 bytes - colormap data (24-bit RGB)
// 8 unknown bytes
// if number_of_objects == 1:
//   object1 width - 1 byte
//   object1 height - 1 byte
//   object1 pos.x - 1 byte
//   object1 pos.y - 1 byte
//   object1 pixels
// if number_of_objects > 1:
//   object headers:
//     8 bytes per each object
//   objects:
//     variable sized unknown data
//     object pixels
void RawExtract_Type1(char *basefilename, char *outfile, FILE *f, int forcewidth, int forceheight, qboolean nopalette, qboolean noswap)
{
	unsigned char *data, *pixeldata, *colormapdata;
	char objectname[MAX_BLOODPATH];
	unsigned int objtag;
	short num1, num2, num3, num4;
	int obj, objsize, objwidth, objheight;
	unsigned char objx, objy, temp, *multiobjects, *out;

	// number of objects
	fread(&objtag, 4, 1, f);
	Verbose("tag: 0x%.8X\n", objtag);
	if (objtag > 32)
		objtag = 1;

	// size
	fread(&objsize, 4, 1, f);
	Verbose("size = %i\n", objsize);

	// colormap data
	colormapdata = qmalloc(768);
	fread(colormapdata, 768, 1, f);

	// 8 unknown bytes
	fread(&num1, 4, 1, f);
	Verbose(" num1 = %i\n", num1);
	fread(&num2, 2, 1, f);
	Verbose(" num2 = %i\n", num2);
	fread(&num3, 1, 1, f);
	Verbose(" num3 = %i\n", num3);
	fread(&num4, 1, 1, f);
	Verbose(" num4 = %i\n", num4);

	// only one image, it seems items only have such
	if (objtag == 1)
	{
		Verbose("== single object ==\n");

		// width and height - 2 bytes
		fread(&temp, 1, 1, f);
		objwidth = (forcewidth > 0) ? forcewidth : (int)temp;
		fread(&temp, 1, 1, f);
		objheight = (forceheight > 0) ? forceheight : (int)temp;
		Verbose(" size = %ix%i\n", objwidth, objheight);

		// x and y - 2 bytes
		fread(&objx, 1, 1, f);
		Verbose(" x = %i\n", objx);
		fread(&objy, 1, 1, f);
		Verbose(" y = %i\n", objy);

		// read pixels
		printfpos(f);
		if (objwidth*objheight > 0)
		{
			pixeldata = qmalloc(objwidth*objheight);
			fread(pixeldata, objwidth*objheight, 1, f);

			// save TGA
			sprintf(objectname, "%s.tga", outfile);
			RawTGA(objectname, (int)objwidth, (int)objheight, (nopalette) ? NULL : colormapdata, objwidth*objheight, pixeldata, 8, noswap);
			qfree(pixeldata);
		}
		qfree(colormapdata);
		return;
	}

	// read multiple objects data, each consists of 8 bytes
	multiobjects = qmalloc(objtag * 8);
	out = multiobjects;
	Verbose("== objects (%i) ==\n", getfpos(f));
	for (obj = 0; obj < (int)objtag; obj++)
	{
		fread(out, 8, 1, f);
		Verbose(" #%2i: %3i %3i + 13 %3i %3i %6i %3i %3i offset %i\n", obj, out[0], out[1] - 13, out[2], out[3], out[4] + out[5]*256, out[6], out[7], getfpos(f));
		out += 8;
	}

	// write objects
	for (objsize = 0, obj = 0; obj < (int)objtag; obj++)
	{
		Verbose("== multiobject #%i (%i) ==\n", obj, getfpos(f) );

		// get width and height
		objwidth = multiobjects[obj*8 + 0];
		objheight = multiobjects[obj*8 + 1] - 13;
		objsize = multiobjects[obj*8 + 4] + multiobjects[obj*8 + 5]*256 - objsize; // object length in bytes
		Verbose(" width = %i\n", objwidth);
		Verbose(" height = %i\n", objheight);
		Verbose(" pixels = %i\n", objwidth*objheight);
		Verbose(" size = %i\n", objsize);

		// skip unknown data
		printfpos(f);	
		Verbose("seek: %i\n", objsize - objwidth*objheight);
		fseek(f, objsize - objwidth*objheight, SEEK_CUR); 
		fseek(f, 2, SEEK_CUR); 
	
		// read pixels
		if (objwidth*objheight > 0)
		{
			data = qmalloc(objwidth*objheight);
			memset(data, 0, objwidth*objheight);
			fread(data, objwidth*objheight, 1, f); 

			// save TGA
			sprintf(objectname, "%s_object%03i.tga", outfile, obj);
			RawTGA(objectname, (int)objwidth, (int)objheight, colormapdata, objwidth*objheight, data, 8, noswap);
			qfree(data);
		}
	}
	qfree(colormapdata);
}	

// multiobject paletted tiles
// 4 bytes - number of objects
// 768 bytes - ??? (colormap data)
// 4 bytes - ???
// object headers:
//   768 bytes - colormap
//   8 bytes - ? ? ? width height ? ? ?
// object pixels:
//   realwidth = width * 2
//   realheight = height * 2
//   realwidth * realheight bytes - indexes into colormap
void RawExtract_Type2(char *basefilename, char *outfile, FILE *f, qboolean nopalette, qboolean noswap)
{
	unsigned int numobjects;
	unsigned char sbytes[2];
	unsigned char *colormapdata;
	unsigned char *objectshead;
	unsigned char *pixeldata;
	unsigned char *in;
	char name[MAX_BLOODPATH];
	int i, width, height, resolutionmultiplier;
	fpos_t fpos;

	// number of objects
	fread(&sbytes, 4, 1, f);
	numobjects = sbytes[1] * 256 + sbytes[0];
	Verbose("tag: %i\n", numobjects);
	if (numobjects > 200 || numobjects <= 0)
		Error("bad file\n");
	if (numobjects > 50)
		numobjects = 2;

	colormapdata = qmalloc(768 * numobjects);
	objectshead = qmalloc(8 * numobjects);

	// unknown info
	fseek(f, 768, SEEK_CUR);
	in = objectshead + 0;
	fread(in, 4, 1, f);
	Verbose("mystic: %3i %3i %3i %3i\n", in[0], in[1], in[2], in[3] );

	// read colormapdata/head_info for number_of_objects + 1
	for (i = 0; i < (int)numobjects; i++)
	{
		in = colormapdata + 768*i;
		fread(in, 768, 1, f);

		in = objectshead + 8*i;
		fread(in, 8, 1, f);
		Verbose("object %3i: %3i %3i %3i %3i %3i %3i %3i %3i\n", i + 1, in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
	}

	printfpos(f);

	// hacks land
	// it seems BO has 2 types of such files with exactly same info, 
	// but one type has true width/height, other it's resulution doubled
	// our task is to determine 'a bigger' file: we simulate pixel reading
	// and if reached EOF - it is standart file, otherwise it is bigger file
	fgetpos(f, &fpos);
	for (i = 0; i < (int)numobjects; i++)
		fseek(f, objectshead[8*i + 4]*objectshead[8*i + 5], SEEK_CUR);
	resolutionmultiplier = 1;
	Verbose("Check for double-res...\n");
	for (i = 0; i < 32; i++)
	{
		fread(&width, 1, 1, f);
		if (!feof(f)) // small file
		{
			Verbose("Double-res file!\n");
			resolutionmultiplier = 2;
			break;
		}
	}
	fsetpos(f, &fpos);

	// write objects
	for (i = 0; i < (int)numobjects; i++)
	{	
		width = objectshead[8*i + 4] * resolutionmultiplier;
		height = objectshead[8*i + 5] * resolutionmultiplier;
		pixeldata = qmalloc(width*height);
		fread(pixeldata, width*height, 1, f);
		sprintf(name, "%s_%i.tga", basefilename, i);
		in = colormapdata + 768*i;
		RawTGA(name, width, height, in, width*height, pixeldata, 8, noswap);
		qfree(pixeldata);
	}

	qfree(colormapdata);
	qfree(objectshead);
}

/*
==========================================================================================

  MAIN

==========================================================================================
*/
	
int Raw_Main(int argc, char **argv)
{
	char filename[MAX_BLOODPATH], basefilename[MAX_BLOODPATH], ext[5], outfile[MAX_BLOODPATH], *c;
	int i = 1, type, forcewidth, forceheight, offset, bytes;
	qboolean nopalette, noswap;
	FILE *f;


	Verbose("=== TimRaw ===\n");
	Verbose("%s\n", argv[i]);

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
	nopalette = false;
	noswap = false;
	bytes = 1;
	forcewidth = -1;
	forceheight = -1;	
	offset = 0;
	for (i = i; i < argc; i++)
	{
		if (!strcmp(argv[i], "-width"))
		{
			i++;
			if (i < argc)
			{
				forcewidth = atoi(argv[i]);
				Verbose("force width = %i\n", forcewidth);
			}
		}
		else if(!strcmp(argv[i], "-height"))
		{
			i++;
			if (i < argc)
			{
				forceheight = atoi(argv[i]);
				Verbose("force height = %i\n", forceheight);
			}
		}
		else if(!strcmp(argv[i], "-type"))
		{
			i++;
			if (i < argc)
				type = atoi(argv[i]);
		}
		else if(!strcmp(argv[i], "-offset"))
		{
			i++;
			if (i < argc)
				offset = atoi(argv[i]);
		}
		else if(!strcmp(argv[i], "-noswap"))
		{
			noswap = true;
			Verbose("BGR->RGB swapping disabled\n");
		}
		else if(!strcmp(argv[i], "-bytes"))
		{
			i++;
			if (i < argc)
				bytes = atoi(argv[i]);
			Verbose("bytes set to %i\n", bytes);
		}
		else if(!strcmp(argv[i], "-noclut"))
		{
			nopalette = true;
			Verbose("CLUT disabled\n");
		}
	}

	// open and convert
	f = SafeOpen(filename, "rb");
	switch(type)
	{
		case 0:
			RawExtract_Type0(basefilename, outfile, f, forcewidth, forceheight, offset, bytes, noswap);
			break;
		case 1:
			RawExtract_Type1(basefilename, outfile, f, forcewidth, forceheight, nopalette, noswap);
			break;
		case 2:
			RawExtract_Type2(basefilename, outfile, f, nopalette, noswap);
			break;
		default:
			Error("Type %i not supported\n", type);
			break;
	}
	fclose(f);

	Print("done.\n");
	return 0;
}
