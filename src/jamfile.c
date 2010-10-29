////////////////////////////////////////////////////////////////
//
// Blood Omen JAM/VAG files loader/exporter
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
#include "mem.h"

void *DecompressLZ77Stream(int *outbufsize, byte *inbuf, int startpos, int buflen, qboolean leading_filesize);

int Jam_Main(int argc, char **argv)
{
	char infile[MAX_BLOODPATH], outfile[MAX_BLOODPATH];
	unsigned int width, height, numframes;
	byte *fbuf, *dec;
	byte tag[4];
	int decSize;
	FILE *f;

	Verbose("=== Jam Video ===\n");

	// in file
	if (argc < 2)
		Error("not enough parms");
	strcpy(infile, argv[1]);
	Verbose("File: %s\n", argv[1]);

	f = SafeOpen(infile, "rb");

	// first 4 chars whould be JAM0
	if (fread(&tag, sizeof(char), 4, f) < 1)
		Error("File too short");
	if (tag[0] != 'J' || tag[1] != 'A' || tag[2] != 'M' || tag[3] != 0)
		Error("Bad tag, should be JAM/0");

	// width/height/frames
	if (fread(&width, sizeof(width), 1, f) < 1)
		Error("File too short");
	Verbose("Width: %i\n", width);
	if (fread(&height, sizeof(height), 1, f) < 1)
		Error("File too short");
	Verbose("Height: %i\n", height);
	if (fread(&numframes, sizeof(numframes), 1, f) < 1)
		Error("File too short");
	Verbose("Frames: %i\n", numframes);

	// read file
	sprintf(outfile, "%s.tga", infile);
	fbuf = qmalloc(76800*3);
	if (fread(fbuf, 76800*3, 1, f) < 1)
		Error("File too short");
	decSize = 0;
	dec = DecompressLZ77Stream(&decSize, fbuf, 0, 76800*3, false);
	Verbose("Frame 0: %i bytes\n", decSize);
	
	RawTGA(outfile, 320, 240, 0, 0, 0, 0, NULL, fbuf, 16, NULL);
	qfree(fbuf);
	return 0;
}