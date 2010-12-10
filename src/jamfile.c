////////////////////////////////////////////////////////////////
//
// Blood Omen JAM files loader/exporter
// originally coded by MisterGrim and placed to public domain
// refactored by VorteX
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
#include "mem.h"

void Jam_DecodeFrame(byte *inbuf, byte *outbuf, byte *prevbuf, int outsize, int frametype)
{
	unsigned char *srcptr, *destptr, *prevptr;
	int bytesleft;
	unsigned int mark;
	int i;

	srcptr = inbuf;
	destptr = outbuf;
	prevptr = prevbuf;
	bytesleft = outsize;

	// non-packed frame
	if (frametype == 2)
	{
		memcpy(outbuf, inbuf, outsize);
		return;
	}
	// packed frame
	while(bytesleft > 0)
	{
		memcpy(&mark, srcptr, 4);
		srcptr += 4;
		for(i=0; i<32 && bytesleft > 0; i++,mark=mark>>1)
		{
			if(mark & 1)
			{
				*destptr = *srcptr;
				destptr ++;
				prevptr ++;
				srcptr ++;
				bytesleft --;
			}
			else
			{
				unsigned short int var1;
				int rep;
				int backoffs;
				unsigned char *back;
				var1 = srcptr[0] + 256*srcptr[1];
				rep = (var1 >> 11) + 3;
				if(frametype == 1)
				{
					backoffs = 0x821 - (var1 & 0x7ff);
					back = destptr - backoffs;
				}
				else
				{
					backoffs = 0x400 - (var1 & 0x7ff);
					back = prevptr - backoffs;
				}
				srcptr += 2;
				memcpy(destptr, back, rep);
				destptr += rep;
				prevptr += rep;
				bytesleft -= rep;
			}
		}
	}
}

void Jam_DecodeToFiles(char *infile, char *outpath)
{
	byte jamHead[16], jamColormap[768], frameHead[16], *compressed, *framedata, *prevframedata, *b;
	int i, width, height, numframes, framesize, outsize, compsize;
	char file[MAX_BLOODPATH];
	FILE *f;

	f = SafeOpen(infile, "rb");

	// first 4 chars whould be JAM0
	if (fread(&jamHead, sizeof(byte), 16, f) < 1)
		Error("File too short");
	if (jamHead[0] != 'J' || jamHead[1] != 'A' || jamHead[2] != 'M' || jamHead[3] != 0)
		Error("Bad tag, should be JAM0");

	// width/height/frames
	width = ReadUInt(jamHead + 4);
	height = ReadUInt(jamHead + 8);
	numframes = ReadUInt(jamHead + 12);
	framesize = width * height;
	Verbose("%ix%i %i frames\n", width, height, numframes);

	// parse file
	memset(jamColormap, 0, 768);
	memset(jamHead, 0, 16);
	compressed = qmalloc(framesize);
	framedata = qmalloc(framesize * 2);
	prevframedata = qmalloc(framesize * 2);
	for(i = 0; i < numframes; )
	{
		// frame header
		if (fread(&frameHead, 16, 1, f) < 1)
			Error("unexpected EOF while reading frame header");
		compsize = ReadUInt(frameHead + 8) - 16;
		if (compsize < 0 || compsize > framesize)
			Error("funky compressed block");
		outsize = ReadUInt(frameHead + 12);
		if (outsize < 0 || outsize > framesize)
			Error("funky outputsize");
		// read compressed data
		if (fread(compressed, compsize, 1, f) < 1)
			Error("unexpected EOF while reading frame");
		// palette goes onterleaved with special flag
		if (frameHead[0] == 2)
		{
			if (compsize != 768)
				Error("error reading palette");
			memcpy(&jamColormap, compressed, 768);
			continue;
		}
		// shift buffers to provide current and previous one, decode
		b = prevframedata;
		prevframedata = framedata;
		framedata = b;
		Jam_DecodeFrame(compressed, framedata, prevframedata, outsize, frameHead[4]);
		// write frame
		Pacifier("writing frame %i of %i...", i + 1, numframes);
		sprintf(file, "%s/frame%04i.tga", outpath, i);
		RawTGA(file, width, height, 0, 0, 0, 0, jamColormap, framedata, 8, NULL);
		i++;
	}

	PacifierEnd();
	qfree(compressed);
	qfree(framedata);
	qfree(prevframedata);
	fclose(f);
}

int Jam_Main(int argc, char **argv)
{
	char infile[MAX_BLOODPATH], outpath[MAX_BLOODPATH];

	Verbose("=== Jam Video ===\n");

	// in file
	if (argc < 3)
		Error("not enough parms");
	strcpy(infile, argv[1]);
	strcpy(outpath, argv[2]);
	Verbose("decoding %s into %s...\n", infile, outpath);
	Jam_DecodeToFiles(infile, outpath);
	
	return 0;
}