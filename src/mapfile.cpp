////////////////////////////////////////////////////////////////
//
// Blood Omen MAP files loader/exporter
// LZ77 decompression by Ben Lincoln
// coded by VorteX
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

// test colors for tilemaps
unsigned char tiletestcolors[20*3] =
{ 
	128, 000, 000, // red
	000, 128, 000, // green
	000, 000, 128, // blue
	000, 128, 128, // cyan
	128, 128, 000, // yellow
	128, 000, 128, // purple
	255, 128, 000,
	128, 255, 000,
	000, 255, 128,
	255, 255, 000,
	255, 000, 000,
	000, 000, 000,
	000, 000, 000,
	000, 000, 000,
	000, 000, 000,
	000, 000, 000,
	000, 000, 000,
	000, 000, 000,
	000, 000, 000,
	000, 000, 000
};

// names for environments
char environmentsnames[2][32] = 
{
	"outdoor",
	"indoor"
};

// track names for songs
char songnames[11][32] = 
{
	"oracle caves",
	"mausoleum",
	"awakening",
	"road to vengeance",
	"nupraptor",
	"maleks bastion",
	"voradors mansion",
	"dark eden",
	"solemn dirge",
	"avernus",
	"elzevir"
};

// light types
typedef struct bo_render_light_s
{
	byte x;
	byte y;
	byte sizex;
	byte sizey;
	float r;
	float g;
	float b;
	int flags;
	bool dynamic;
	bool flickering;
	bo_render_light_s *next;
} bo_render_light_t;

/*
==========================================================================================

 LZ77 stream decoding

==========================================================================================
*/

// read decompressed LZ77 stream
// thanks to Ben Lincoln for that function
#define LZ_MAX_ITERATIONS 4096000
#define LZ_MAX_OUTPUT 1048576
byte lzbuf[LZ_MAX_OUTPUT];
void *LzDec(int *outbufsize, byte *inbuf, int startpos, int buflen, bool leading_filesize)
{
	int filesize;
	bool done = false;
	int numIterations, bytesWritten;
	unsigned short tempIndex, command;
	short offset;
	byte length, currentByte, currentByte1, currentByte2;
	byte tempBuffer[4114];
	int i;

	// compare file size
	if (buflen < 8)
		return NULL;
	if (leading_filesize)
	{
		filesize = ReadUInt(inbuf) + 4;
		startpos = 4;
		if (filesize != buflen)
			return NULL;
	}

	// initialize decompressor
	tempIndex = 4078;
	numIterations = 0;
	bytesWritten = 0;
    command = 0;
    for (i = 0; i < 4078; i++)
		tempBuffer[i] = 0x20;

	// decompress
	#define readByte() inbuf[startpos]; startpos++;	
	#define tempInc(b) tempBuffer[tempIndex] = b; tempIndex++; tempIndex = (unsigned short)(tempIndex % 4096);
	#define outByte(b) lzbuf[bytesWritten] = b; bytesWritten++;
	while(!done)
    {
		// handle errors
		if (numIterations > LZ_MAX_ITERATIONS)
			return NULL;
		if (bytesWritten > LZ_MAX_OUTPUT)
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
		// command ad 0x01 - read whole next byte
		if ((command & 0x01) == 0x01)
		{
			currentByte = readByte()
			outByte(currentByte)
			tempInc(currentByte)
			if (startpos > buflen)
				break;
		}
		// otherwise unpack the sequence
		else
		{
			currentByte1 = readByte()
			currentByte2 = readByte()
			offset = (unsigned short)(currentByte1 | (currentByte2 & 0xF0) << 4);
			length = (byte)((currentByte2 & 0x0F) + 3);
			for (i = 0; i < length; i++)
			{
				outByte(tempBuffer[(offset + i) % 4096])
				if (startpos > buflen)
				{
					done = true;
					break;
				}
				tempInc(tempBuffer[(offset + i) % 4096])
			}
		}
		numIterations++;
	}
	#undef tempInc
	#undef readByte
	#undef outByte

	// playstation files apparently need to be multiples of 1024 bytes in size
	if ((bytesWritten % 1024) > 0)
		bytesWritten = bytesWritten + 1024 - (bytesWritten % 1024);

	*outbufsize = bytesWritten;
	return lzbuf;
}

/*
==========================================================================================

 Picture handling tools

==========================================================================================
*/

// a subpics for picture (up to 64 subpics per picture)
typedef struct
{
	short    x;
	short    y;
	short    w;
	short    h;
	short    ofsx;
	short    ofsy;
} subpic_t;
typedef struct
{
	int      picwidth;
	int      picheight;
	byte     numtex;
	subpic_t tex[64];
} subpics_t;

// a test subpic
subpics_t subpics_grpobject =
{
	256, 256, 1,
	{
	{ 0, 0, 0, 0, 0, 0 }
	}
};

// a generic tilemap subpics
subpics_t subpics_grp =
{
	256, 256, 64,
	{
	{ 0,   0, 32, 32, 0, 0 },	{  32,   0, 32, 32, 0, 0 },  {  64,   0, 32, 32, 0, 0 }, {  96,   0, 32, 32, 0, 0 },  { 128,   0, 32, 32, 0, 0 }, { 160,   0, 32, 32, 0, 0 }, { 192,   0, 32, 32, 0, 0 }, { 224,   0, 32, 32, 0, 0 },
	{ 0,  32, 32, 32, 0, 0 },	{  32,  32, 32, 32, 0, 0 },  {  64,  32, 32, 32, 0, 0 }, {  96,  32, 32, 32, 0, 0 },  { 128,  32, 32, 32, 0, 0 }, { 160,  32, 32, 32, 0, 0 }, { 192,  32, 32, 32, 0, 0 }, { 224,  32, 32, 32, 0, 0 },
	{ 0,  64, 32, 32, 0, 0 },	{  32,  64, 32, 32, 0, 0 },  {  64,  64, 32, 32, 0, 0 }, {  96,  64, 32, 32, 0, 0 },  { 128,  64, 32, 32, 0, 0 }, { 160,  64, 32, 32, 0, 0 }, { 192,  64, 32, 32, 0, 0 }, { 224,  64, 32, 32, 0, 0 },
	{ 0,  96, 32, 32, 0, 0 },	{  32,  96, 32, 32, 0, 0 },  {  64,  96, 32, 32, 0, 0 }, {  96,  96, 32, 32, 0, 0 },  { 128,  96, 32, 32, 0, 0 }, { 160,  96, 32, 32, 0, 0 }, { 192,  96, 32, 32, 0, 0 }, { 224,  96, 32, 32, 0, 0 },
	{ 0, 128, 32, 32, 0, 0 },	{  32, 128, 32, 32, 0, 0 },  {  64, 128, 32, 32, 0, 0 }, {  96, 128, 32, 32, 0, 0 },  { 128, 128, 32, 32, 0, 0 }, { 160, 128, 32, 32, 0, 0 }, { 192, 128, 32, 32, 0, 0 }, { 224, 128, 32, 32, 0, 0 },
	{ 0, 160, 32, 32, 0, 0 },	{  32, 160, 32, 32, 0, 0 },  {  64, 160, 32, 32, 0, 0 }, {  96, 160, 32, 32, 0, 0 },  { 128, 160, 32, 32, 0, 0 }, { 160, 160, 32, 32, 0, 0 }, { 192, 160, 32, 32, 0, 0 }, { 224, 160, 32, 32, 0, 0 },
	{ 0, 192, 32, 32, 0, 0 },	{  32, 192, 32, 32, 0, 0 },  {  64, 192, 32, 32, 0, 0 }, {  96, 192, 32, 32, 0, 0 },  { 128, 192, 32, 32, 0, 0 }, { 160, 192, 32, 32, 0, 0 }, { 192, 192, 32, 32, 0, 0 }, { 224, 192, 32, 32, 0, 0 },
	{ 0, 224, 32, 32, 0, 0 },	{  32, 224, 32, 32, 0, 0 },  {  64, 224, 32, 32, 0, 0 }, {  96, 224, 32, 32, 0, 0 },  { 128, 224, 32, 32, 0, 0 }, { 160, 224, 32, 32, 0, 0 }, { 192, 224, 32, 32, 0, 0 }, { 224, 224, 32, 32, 0, 0 }
	}
};

// subpics for powerup.tim
subpics_t subpics_powerup =
{
	128, 200, 8,
	{
	{   0,  88, 19, 42, 0, 0 },
	{  87,  88, 22, 42, 0, 0 },
	{ 109,  88, 19, 42, 0, 0 },
	{   0, 130, 20, 45, 0, 0 },
	{  21, 130, 19, 42, 0, 0 },
	{  42, 130, 22, 42, 0, 0 },
	{  67, 130, 24, 42, 0, 0 },
	{  92, 130, 28, 42, 0, 0 }
	}
};

// subpics for smallpw.tim
subpics_t subpics_powerup_small =
{
	96, 150, 1,
	{
	{  65,  66, 17, 32, 0, 0 },
	}
};

// subpics for gam.tim
subpics_t subpics_gam =
{
	128, 240, 1,
	{
	{  44, 122, 28, 21, 0, 0 }
	}
};

// font subpics
subpics_t subpics_font =
{
	630, 10, 62,
	{
	    {   1,  0, 6, 10, 0, 0 }, // 0
		{  11,  0, 5, 10, 0, 0 }, // 1
		{  19,  0, 6, 10, 0, 0 }, // 2
		{  28,  0, 6, 10, 0, 0 }, // 3
		{  37,  0, 6, 10, 0, 0 }, // 4
		{  46,  0, 6, 10, 0, 0 }, // 5
		{  55,  0, 6, 10, 0, 0 }, // 6
		{  64,  0, 6, 10, 0, 0 }, // 7
		{  73,  0, 6, 10, 0, 0 }, // 8
		{  81,  0, 6, 10, 0, 0 }, // 9
		{  90,  0,10, 10, 0, 0 }, // A
		{ 102,  0, 8, 10, 0, 0 }, // B
		{ 113,  0, 8, 10, 0, 0 }, // C
		{ 124,  0, 9, 10, 0, 0 }, // D
		{ 136,  0, 8, 10, 0, 0 }, // E
		{ 147,  0, 7, 10, 0, 0 }, // F
		{ 157,  0,10, 10, 0, 0 }, // G
		{ 169,  0,10, 10, 0, 0 }, // H
		{ 181,  0, 5, 10, 0, 0 }, // I
		{ 189,  0, 7, 10, 0, 0 }, // J
		{ 198,  0,10, 10, 0, 0 }, // K
		{ 210,  0, 9, 10, 0, 0 }, // L
		{ 221,  0,11, 10, 0, 0 }, // M
		{ 235,  0, 9, 10, 0, 0 }, // N
		{ 247,  0, 9, 10, 0, 0 }, // O
		{ 259,  0, 8, 10, 0, 0 }, // P
		{ 269,  0, 9, 10, 0, 0 }, // Q
		{ 281,  0, 9, 10, 0, 0 }, // R
		{ 293,  0, 7, 10, 0, 0 }, // S
		{ 302,  0, 8, 10, 0, 0 }, // T
		{ 313,  0, 9, 10, 0, 0 }, // U
		{ 324,  0, 9, 10, 0, 0 }, // V
		{ 335,  0,13, 10, 0, 0 }, // W
		{ 350,  0,10, 10, 0, 0 }, // X
		{ 361,  0,10, 10, 0, 0 }, // Y
		{ 373,  0, 8, 10, 0, 0 }, // Z
		{ 383,  0, 7, 10, 0, 0 }, // a
		{ 393,  0, 6, 10, 0, 0 }, // b
		{ 402,  0, 6, 10, 0, 0 }, // c
		{ 410,  0, 7, 10, 0, 0 }, // d
		{ 419,  0, 6, 10, 0, 0 }, // e
		{ 428,  0, 6, 10, 0, 0 }, // f
		{ 436,  0, 7, 10, 0, 0 }, // g
		{ 446,  0, 7, 10, 0, 0 }, // h
		{ 455,  0, 4, 10, 0, 0 }, // i
		{ 462,  0, 4, 10, 0, 0 }, // j
		{ 469,  0, 7, 10, 0, 0 }, // k
		{ 478,  0, 6, 10, 0, 0 }, // l
		{ 487,  0, 8, 10, 0, 0 }, // m
		{ 498,  0, 6, 10, 0, 0 }, // n
		{ 507,  0, 7, 10, 0, 0 }, // o
		{ 516,  0, 6, 10, 0, 0 }, // p
		{ 525,  0, 7, 10, 0, 0 }, // q
		{ 534,  0, 7, 10, 0, 0 }, // r
		{ 543,  0, 5, 10, 0, 0 }, // s
		{ 551,  0, 6, 10, 0, 0 }, // t
		{ 560,  0, 6, 10, 0, 0 }, // u
		{ 568,  0, 7, 10, 0, 0 }, // v
		{ 578,  0, 9, 10, 0, 0 }, // w
		{ 589,  0, 7, 10, 0, 0 }, // x
		{ 598,  0, 7, 10, 0, 0 }, // y
		{ 607,  0, 6, 10, 0, 0 }  // z
	}
};

// builtin font.tim
#define FONT_TIM_SIZE 6844
unsigned char FONT_TIM[FONT_TIM_SIZE] = {
    0x10, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x7F, 0x42, 0x08, 0x84, 0x10, 0xC6, 0x18, 0x08, 0x21, 0x4A, 0x29, 0x8C, 0x31, 0xCE, 0x39, 0x31, 0x46, 0x73, 0x4E, 0xB5, 0x56, 0xF7, 0x5E, 0x39, 0x67,
    0x7B, 0x6F, 0xBD, 0x77, 0x00, 0x00, 0x00, 0x7C, 0xE0, 0x03, 0xE0, 0x7F, 0x1F, 0x00, 0x1F, 0x7C, 0xFF, 0x03, 0xFF, 0x7F, 0x9C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x01, 0x0A, 0x00,
    0xF8, 0xF8, 0xEB, 0xF5, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xF7, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xF6, 0xFF,
    0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xFF, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF6, 0xF7, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF7, 0xFF, 0xFF, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xF7, 0xFF, 0xF5, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xF7, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF6,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF6, 0xFF, 0xF6, 0xEF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xFF, 0xFF,
    0xFF, 0xF6, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE,
    0xF5, 0xFF, 0xFF, 0xF2, 0xEC, 0xEE, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xF1, 0xF3, 0xFF, 0xFF, 0xFF, 0xEE, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7,
    0xFF, 0xFF, 0xFF, 0xEC, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xF3, 0xF8, 0xF7, 0xFF, 0xFF, 0xF0, 0xF8, 0xF8, 0xEE, 0xFF, 0xFF, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xFF,
    0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xFF, 0xFF, 0xEC, 0xF8, 0xF8, 0xF3, 0xFF, 0xFF, 0xF2, 0xF8, 0xEA, 0xFF, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF7, 0xFF, 0xFF, 0xF3, 0xEA, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF7, 0xFF, 0xFF, 0xF3, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF4,
    0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xEF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xF1, 0xEA, 0xFF,
    0xFF, 0xF5, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xF8, 0xF3, 0xFF, 0xFF, 0xEA, 0xF8, 0xEC, 0xFF, 0xFF, 0xFF, 0xEE, 0xF7, 0xFF, 0xFF, 0xF7, 0xF8, 0xF0, 0xFF, 0xF3, 0xF8, 0xF8, 0xEE, 0xFF,
    0xFF, 0xFF, 0xF3, 0xEC, 0xFF, 0xFF, 0xF7, 0xF8, 0xF8, 0xEE, 0xFF, 0xFF, 0xFF, 0xF5, 0xF8, 0xEE, 0xFF, 0xFF, 0xF0, 0xF8, 0xF8, 0xEA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xF2, 0xEC, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xFF, 0xFF, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xEA, 0xEF, 0xF8, 0xF4, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF,
    0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xF7, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xFF, 0xFF, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xEB,
    0xFF, 0xF3, 0xF8, 0xF6, 0xF2, 0xF8, 0xF8, 0xF8, 0xEA, 0xF7, 0xF0, 0xF0, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF5, 0xF8,
    0xF0, 0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xF1, 0xF8, 0xF8, 0xF1, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF4, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3,
    0xFF, 0xEC, 0xF8, 0xF8, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF2, 0xEA, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF2, 0xF8, 0xF8, 0xF0, 0xF7, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF1,
    0xF8, 0xF8, 0xEF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF0, 0xF7, 0xFF, 0xEF, 0xF8, 0xF8, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xF0, 0xF8, 0xEB, 0xF6, 0xF6, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF1, 0xEB, 0xF7, 0xF7,
    0xEA, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xEF, 0xF8, 0xEB, 0xF5, 0xF6, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xEE, 0xF8, 0xEF, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xF7, 0xEB, 0xF8, 0xF1,
    0xF3, 0xF8, 0xF8, 0xF8, 0xEF, 0xEC, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF3, 0xF8, 0xF8,
    0xF8, 0xF2, 0xEC, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xF5, 0xF8, 0xF8, 0xF5, 0xFF, 0xEE, 0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xF7, 0xF7, 0xEC, 0xF8, 0xEF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF0, 0xFF, 0xFF, 0xEA, 0xF8, 0xF8, 0xEF, 0xF1, 0xF8, 0xF8, 0xF8, 0xEC, 0xF4, 0xEA, 0xF8, 0xF5, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xF0, 0xF8, 0xF7, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE,
    0xEC, 0xF8, 0xF4, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF5, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xED, 0xF4, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xEE, 0xF8, 0xF8, 0xF4, 0xEF, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF3, 0xF8, 0xF3, 0xF2, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF,
    0xF0, 0xEB, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xF4, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF5, 0xF8, 0xF8, 0xF7, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF3,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xEA, 0xF7, 0xFF, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF7, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8,
    0xF8, 0xF8, 0xEC, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF1, 0xF8, 0xF0, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC,
    0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xFF, 0xEC, 0xF8, 0xF8, 0xF1, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF0, 0xFF, 0xF7, 0xEB, 0xF8, 0xF2, 0xF8,
    0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF1, 0xF8, 0xF7, 0xFF, 0xEE, 0xF8, 0xF8, 0xEE, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xEC,
    0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xEE, 0xF8, 0xF8, 0xF7, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF7, 0xEB, 0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF7, 0xEA, 0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xFF, 0xEC,
    0xF8, 0xF0, 0xFF, 0xF2, 0xF8, 0xEB, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF5, 0xEC, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xF0, 0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8,
    0xF8, 0xEC, 0xED, 0xF8, 0xEE, 0xFF, 0xF6, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF0, 0xF8, 0xF7, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA,
    0xFF, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xF4, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF7, 0xFF, 0xF6, 0xEB, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xF3, 0xF5, 0xEB, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xEA, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1,
    0xEC, 0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF5, 0xF8, 0xF0, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1,
    0xFF, 0xEE, 0xF8, 0xF8, 0xF4, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xEE, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xEC, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF,
    0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF1, 0xF2, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF2, 0xF5, 0xFF, 0xF1, 0xF8, 0xF8, 0xF1, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xEA, 0xF2, 0xFF, 0xF5, 0xF8, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xED,
    0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF1, 0xEB, 0xF7, 0xF7, 0xEA, 0xF8, 0xF8, 0xF2, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xEE, 0xF8,
    0xEE, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xFF, 0xF2, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8,
    0xF8, 0xF8, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEF, 0xF8, 0xEF, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xF0, 0xF8, 0xF0, 0xF7, 0xF6, 0xF8, 0xEF, 0xEC, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF6, 0xF8, 0xEE, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xFF, 0xFF, 0xF5, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xF7, 0xF0, 0xED, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xFF,
    0xF6, 0xED, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xEC, 0xF5, 0xFF, 0xF6, 0xF1, 0xEB, 0xF8, 0xF8, 0xF8,
    0xF2, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xFF, 0xFF, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xFF, 0xED, 0xF6, 0xFF, 0xF0, 0xF8, 0xF8,
    0xED, 0xFF, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xF0, 0xF8, 0xF8, 0xEF, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xF3, 0xF8, 0xFF, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF,
    0xFF, 0xF6, 0xED, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xFF, 0xFF, 0xF5, 0xEA, 0xF8, 0xF8, 0xF8, 0xEA, 0xF4, 0xFF, 0xFF, 0xF5, 0xEC, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xED, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF4, 0xFF, 0xF1, 0xF0, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF8, 0xF8, 0xEB, 0xFF, 0xFF, 0xF2, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xEF, 0xFF, 0xFF, 0xF0, 0xEF, 0xFF,
    0xF2, 0xF8, 0xF8, 0xF8, 0xFF, 0xFF, 0xF4, 0xFF, 0xFF, 0xF2, 0xF2, 0xFF, 0xEB, 0xF8, 0xF8, 0xF4, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xFF, 0xEF, 0xF2, 0xFF, 0xEF, 0xF8,
    0xF8, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF0, 0xF8, 0xF7, 0xF5, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xF4, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF8,
    0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF1, 0xEC, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xF4, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xF6, 0xEB, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF4, 0xEA, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF0, 0xF8, 0xF6, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7,
    0xFF, 0xFF, 0xFF, 0xF4, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF3, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF3, 0xFF, 0xFF, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0,
    0xFF, 0xF7, 0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF0, 0xFF, 0xF6, 0xF8, 0xEF, 0xED, 0xF7, 0xF7, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xEA, 0xEA, 0xF6, 0xFF, 0xF0, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xFF, 0xFF,
    0xF6, 0xEE, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xF7, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF,
    0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF,
    0xF4, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF5, 0xF8, 0xF0, 0xF4, 0xFF, 0xED, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xF1, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xF6, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8,
    0xF8, 0xEB, 0xFF, 0xEB, 0xEB, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF2, 0xF7, 0xEC, 0xF8, 0xF2, 0xED, 0xF8, 0xF8, 0xF8, 0xF0, 0xF6, 0xF8, 0xF5, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xED, 0xF1,
    0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF4, 0xEE, 0xF1, 0xEF, 0xF8, 0xF8, 0xF8, 0xF6, 0xF4, 0xF8, 0xEA, 0xF5, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xFF, 0xEF, 0xF8, 0xF2, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF0, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xF0, 0xEC, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xFF, 0xF6, 0xF8, 0xF8, 0xF4, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xFF, 0xF0, 0xED, 0xED, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF1, 0xF8, 0xF3, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8, 0xEF, 0xF6, 0xEA,
    0xF7, 0xF0, 0xF8, 0xF8, 0xF8, 0xF4, 0xF5, 0xEA, 0xF8, 0xF4, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xF2, 0xF8, 0xF5, 0xF1, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xEA, 0xED, 0xF0, 0xF8, 0xF8, 0xF8, 0xF1,
    0xEA, 0xF4, 0xF2, 0xEC, 0xF0, 0xF8, 0xF8, 0xF8, 0xF0, 0xF6, 0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xEB, 0xF8, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xEE, 0xEE, 0xFF, 0xEE,
    0xED, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF6, 0xF6, 0xEA, 0xEB, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xEB, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF0, 0xF8, 0xF7, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xEA, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF7, 0xF3, 0xF8, 0xF8, 0xF8, 0xEC, 0xF0, 0xF8, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0,
    0xF7, 0xF5, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF0, 0xF8, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xEE, 0xF0, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF1, 0xFF, 0xF6, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF5, 0xF8, 0xEE, 0xF7, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8, 0xF4,
    0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF4, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xEE, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xEB, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF1, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE,
    0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF1, 0xF4, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xEC, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF8, 0xF7, 0xFF, 0xEE, 0xF2, 0xF8, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xEA, 0xF8, 0xED, 0xFF, 0xFF,
    0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xEE, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xEE, 0xF5, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3,
    0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF7, 0xEC, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF7, 0xFF, 0xEF, 0xED, 0xF0, 0xFF, 0xF0, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xF6, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xFF, 0xEC, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xEE, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xFF, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8,
    0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF6, 0xF8, 0xEF, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xFF, 0xED, 0xF8,
    0xF8, 0xF8, 0xED, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xED,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xF4, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xED, 0xEB, 0xF4, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF0, 0xF3, 0xFF, 0xF0, 0xED, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xEB, 0xF8, 0xEF, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xEF, 0xF6, 0xEA, 0xF7, 0xF0, 0xF8, 0xF8, 0xEA, 0xFF, 0xEF, 0xF8, 0xF8, 0xED,
    0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF6, 0xF5, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF6,
    0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xF0, 0xEE, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF1, 0xEE, 0xFF, 0xF1, 0xF1, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF7, 0xF6, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF0, 0xF2, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xF2, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xF0, 0xF8, 0xF7, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xF0, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xF4, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF1, 0xF8, 0xF7,
    0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xEA, 0xF8, 0xF7, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF2, 0xF8, 0xF8, 0xEB, 0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF5, 0xF8, 0xF8, 0xF3, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF7, 0xFF, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xEC, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF1, 0xF6, 0xF8, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF1, 0xEA, 0xF6, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xEE, 0xED, 0xF8,
    0xF8, 0xF8, 0xF2, 0xF8, 0xF2, 0xFF, 0xF4, 0xF0, 0xF8, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xEA, 0xF8, 0xF8, 0xF0, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF6, 0xF8, 0xF8,
    0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF,
    0xEE, 0xEB, 0xF7, 0xF7, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xEA, 0xF8, 0xF8, 0xEE, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF,
    0xF0, 0xF8, 0xF8, 0xEB, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xF4, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xF5, 0xF8, 0xED, 0xFF, 0xF7, 0xEF, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xF1, 0xEA, 0xF7, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8,
    0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xEB, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xEA, 0xFF, 0xF3, 0xF8, 0xF8, 0xEB, 0xEA, 0xF8, 0xF8, 0xF8, 0xF0,
    0xF6, 0xF8, 0xEF, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xED, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF4, 0xEE, 0xED, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xED, 0xF8, 0xF6, 0xFF, 0xF4, 0xF8,
    0xF8, 0xF8, 0xF8, 0xFF, 0xEF, 0xF8, 0xF2, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xF5, 0xFF, 0xF0, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF8, 0xF8, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF7, 0xF1, 0xF0, 0xF0, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xEA, 0xF6, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF0,
    0xFF, 0xEB, 0xF8, 0xEF, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xFF, 0xF5, 0xEA, 0xF8, 0xF8, 0xEB, 0xFF, 0xEF, 0xF8, 0xF8, 0xED, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF4, 0xF2, 0xF6, 0xF4, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF6, 0xF8, 0xF8, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xF4,
    0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xF6, 0xF0, 0xF3, 0xF5, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xED, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xF2, 0xEB, 0xFF,
    0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF0, 0xF6, 0xEC, 0xEA, 0xF6, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF1, 0xF8, 0xF1, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xF6, 0xEB, 0xF7, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xED, 0xF4, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xEE, 0xEB, 0xF7, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xF4, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF7, 0xF5, 0xF8, 0xEC, 0xF7, 0xFF, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xEF, 0xF8, 0xF8, 0xF3, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEF, 0xF8, 0xF2, 0xFF, 0xF2, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF0, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF3, 0xF8, 0xF8, 0xF0, 0xFF, 0xEE,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF0, 0xF8, 0xF8, 0xF1, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF4, 0xF8, 0xF6, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF0, 0xFF, 0xF1, 0xF8, 0xEE, 0xF7, 0xF7, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF4, 0xF8, 0xF8, 0xEB, 0xF6, 0xEB, 0xF8, 0xF8, 0xF8, 0xF2, 0xF8, 0xED, 0xFF, 0xFF, 0xEB, 0xF8, 0xF7,
    0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xEA, 0xF8, 0xF8, 0xF8, 0xF4, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xF0, 0xF8, 0xEC, 0xF6, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF,
    0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xEE, 0xF8, 0xEB, 0xF5, 0xF7, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xEE, 0xF8, 0xEF, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0,
    0xF4, 0xEB, 0xF8, 0xF0, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF7, 0xF5, 0xEA, 0xF8, 0xF4, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xEF, 0xFF, 0xF5, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xF3, 0xF8, 0xF8, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xF2, 0xF8, 0xF8, 0xEF, 0xFF, 0xF5,
    0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF6, 0xFF, 0xEE, 0xF8, 0xF8, 0xF1, 0xF3, 0xF8, 0xF8, 0xF8, 0xEE, 0xEF, 0xF8, 0xF2, 0xF7, 0xEA,
    0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xEB, 0xEC, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF4, 0xF7, 0xEC, 0xEA, 0xF5, 0xEA, 0xF8, 0xF8, 0xF8, 0xF0, 0xF6, 0xF8, 0xF5, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF,
    0xF8, 0xF8, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEA, 0xF7, 0xF4, 0xF8, 0xED, 0xFF, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xFF, 0xEF, 0xF8, 0xF2, 0xF2, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF0, 0xFF, 0xF8, 0xF8, 0xF8, 0xEA, 0xFF, 0xEE, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xF0, 0xF1, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF8, 0xEB, 0xF5, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF0, 0xF2, 0xF7, 0xF1, 0xEF, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF8, 0xEC, 0xFF, 0xED, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xF1, 0xF8, 0xF5, 0xF6, 0xEA, 0xF8, 0xF8, 0xF8, 0xEF,
    0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xF5, 0xEA, 0xF8, 0xF4, 0xF7, 0xEA, 0xF8, 0xF8, 0xF8, 0xF4, 0xF2, 0xED, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xED, 0xF0, 0xF8, 0xF3, 0xF4, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF4, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xEB, 0xEB, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xF3,
    0xF0, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xEE, 0xF4, 0xF6, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xEB, 0xEA, 0xF2, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xF6, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xFF, 0xFF, 0xF0,
    0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xEE, 0xFF, 0xFF, 0xF5, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xEA, 0xF8, 0xF8, 0xF8, 0xEB, 0xF5,
    0xFF, 0xF5, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xF5, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xF4, 0xEA, 0xF8,
    0xF8, 0xF8, 0xF1, 0xFF, 0xF2, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xFF, 0xF1, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xEE, 0xF8, 0xEA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0xEE, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xF7, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xED,
    0xF8, 0xF8, 0xF8, 0xF5, 0xFF, 0xFF, 0xFF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xF6, 0xFF, 0xFF, 0xF4, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xF1, 0xF3, 0xFF,
    0xFF, 0xFF, 0xEE, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8, 0xEB, 0xF5, 0xFF, 0xF6, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xF3, 0xF1, 0xFF, 0xFF, 0xFF, 0xF5,
    0xF8, 0xF8, 0xEE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xF8, 0xF8, 0xF8, 0xF7, 0xFF, 0xFF, 0xEC, 0xF6, 0xF4, 0xEC, 0xFF, 0xFF, 0xFF, 0xFF, 0xEC, 0xF8, 0xF8, 0xF3, 0xFF, 0xFF, 0xEC, 0xF8,
    0xF8, 0xEA, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xF7, 0xFF, 0xFF, 0xF3, 0xEA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3,
    0xFF, 0xFF, 0xFF, 0xF5, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xF0, 0xF8, 0xF4, 0xFF, 0xFF, 0xEA, 0xF8, 0xF8, 0xEF, 0xEC, 0xF4, 0xFF, 0xF5, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF7, 0xFF, 0xFF, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEE, 0xF7, 0xFF, 0xF7, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xEA, 0xF7, 0xEF, 0xF8, 0xF8, 0xF3, 0xF4, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xF3, 0xF8, 0xF5, 0xFF, 0xFF, 0xFF, 0xEC, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xFF, 0xFF, 0xFF,
    0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF1, 0xF8, 0xF8, 0xEB, 0xFF, 0xF6, 0xF8, 0xFF, 0xFF, 0xF4, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xFF, 0xFF, 0xF5, 0xEA, 0xF8,
    0xF8, 0xF8, 0xEB, 0xF5, 0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xFF, 0xF6, 0xED, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xFF, 0xF0,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xF5, 0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF3, 0xFF,
    0xF5, 0xEA, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xFF, 0xF2, 0xFF, 0xFF, 0xF2, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0xFF, 0xF4, 0xF8,
    0xF8, 0xF8, 0xF4, 0xFF, 0xED, 0xF8, 0xF0, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xEF, 0xFF, 0xFF, 0xF6, 0xEC, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xF5, 0xFF,
    0xFF, 0xF7, 0xED, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xEF, 0xF2, 0xFF, 0xF0, 0xF8, 0xF8, 0xED, 0xF1, 0xF7, 0xF7, 0xED, 0xF8, 0xF8, 0xF8, 0xF8, 0xF2, 0xFF, 0xFF, 0xEF, 0xF8, 0xF8, 0xF8, 0xF8,
    0xEA, 0xF5, 0xFF, 0xF7, 0xEC, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xEC, 0xFF, 0xF0, 0xEC, 0xFF, 0xEE, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xFF, 0xF0,
    0xF4, 0xFF, 0xFF, 0xEB, 0xF8, 0xF8, 0xF8, 0xED, 0xFF, 0xFF, 0xF6, 0xF8, 0xF8, 0xF8, 0xF8, 0xEB, 0xFF, 0xFF, 0xFF, 0xFF, 0xF2, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xFF, 0xFF, 0xEB, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xFF, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8 
};

// cache pic
#define MAX_CACHEDPICS 128
typedef struct
{
	char  name[24];
	byte *pixels;
	int   x;
	int   y;
	int   width;
	int   height;
	// subpics
	subpics_t *subpics;
	// averaged color
	byte r;
	byte g;
	byte b;
	// use handle (for purging)
	long  used;
}cachepic_t;

cachepic_t         cachedpics[MAX_CACHEDPICS];
char              *cachepic_tilespath;
bigfileheader_t   *cachepic_bigfileheader;
FILE              *cachepic_bigfile;
long               cached_usecounter;

#define CachePic_Globals(tp, bfh, bf) cachepic_tilespath = tp; cachepic_bigfileheader = bfh; cachepic_bigfile = bf;
static cachepic_t *CachePic(char *entryname)
{
	char filename[MAX_OSPATH], *load_entryname;
	byte *filedata, *pixels, *dec;
	int i, j, e, filesize, decSize, posx, posy, width, height;
	unsigned int hash;
	cachepic_t *pic;
	bigfileentry_t *entry;
	tim_image_t *tim;
	rawblock_t *rawblock;
	rawinfo_t rawinfo;
	subpics_t *subpics;
	float stepx, stepy;
	long avg[3];
	byte pixel;

	// check if object was already loaded
	e = -1;
	for (i = 0; i < MAX_CACHEDPICS; i++)
	{
		if (cachedpics[i].pixels == NULL)
		{
			if (e < 0)
				e = i;
			continue;
		}
		if (!strcmp(cachedpics[i].name, entryname))
		{
			cachedpics[i].used = cached_usecounter;
			cached_usecounter++;
			return &cachedpics[i];
		}
	}

	// no match found, we need to load it
	// if no free slots available, we free last used one and replace it
	if (e < 0)
	{
		e = 0; // default to first item
		for (i = 1; i < MAX_CACHEDPICS; i++)
			if (cachedpics[i].used < cachedpics[e].used)
				e = i;
		if (cachedpics[e].pixels != NULL)
			mem_free(cachedpics[e].pixels);
		cachedpics[e].pixels = NULL;
		memset(&cachedpics[e], 0, sizeof(cachepic_t));
	}

	// determine the subpics
	load_entryname = entryname;
	subpics = NULL;
	     if (!strncmp(entryname, "powerup.tim", 11)) { subpics = &subpics_powerup; }
	else if (!strncmp(entryname, "smallpw.tim", 11)) { subpics = &subpics_powerup_small; load_entryname = "powerup.tim";  }
	else if (!strncmp(entryname, "gam.tim", 7))      { subpics = &subpics_gam; }
	else if (!strncmp(entryname, "font.tim", 8))     { subpics = &subpics_font; }
	
	// get file contents
	Print("  %s - ", entryname);
	entry = NULL;
	if (cachepic_bigfileheader != NULL && cachepic_bigfile)
	{
		// from a bigfile
		hash = BigfileEntryHashFromString(load_entryname, false);
		if (hash)
		{
			entry = BigfileGetEntry(cachepic_bigfileheader, hash);
			if (entry)
			{
				filesize = entry->size;
				if (!filesize)
				{
					Print("failed (null entry)\n");
					entry = NULL;
				}
				else
				{
					filedata = (byte *)mem_alloc(entry->size);
					if (fseek(cachepic_bigfile, (long int)entry->offset, SEEK_SET))
					{
						mem_free(filedata);
						Print("failed (cannot seek entry)\n");
						entry = NULL;
					}
					else
					{
						if (fread(filedata, filesize, 1, cachepic_bigfile) < 1)
						{
							mem_free(filedata);
							Print("failed (cannot read entry)\n");
							entry = NULL;
						}
					}
				}
			}
		}
	}
	
	// load from external file
	if (!entry)
	{
		sprintf(filename, "%s%s", cachepic_tilespath, load_entryname);
		filesize = LoadFileUnsafe(filename, &filedata);
		if (filesize < 0)
		{
			filesize = LoadFileUnsafe(load_entryname, &filedata);
			if (filesize < 0)
			{
				// check if we can load builtin file
				if (!strcmp(entryname, "font.tim"))
				{
					filedata = FONT_TIM;
					filesize = FONT_TIM_SIZE;
				}
				else
				{
					Print("failed (cannot open file)\n");
					return NULL;
				}
			}
		}
	}
	
	// load entry
	pixels = NULL;
	width = 0;
	height = 0;
	FlushRawInfo(&rawinfo);
	rawblock = RawExtract(filedata, filesize, &rawinfo, false, false, RAW_TYPE_UNKNOWN);
	memset(avg, 0, sizeof(avg));
	if (rawblock->errorcode >= 0)
	{
		// load raw
		width = rawblock->chunk[0].width;
		height = rawblock->chunk[0].height;
		posx = rawblock->posx - rawblock->chunk[0].x;
		posy = rawblock->posy - rawblock->chunk[0].y;
		pixels = (byte *)mem_alloc(width * height * 3);
		for (i = 0; i < width*height; i++)
		{
			pixel = rawblock->chunk[0].pixels[i];
			pixels[i*3] = rawblock->colormap[pixel*3];
			pixels[i*3 + 1] = rawblock->colormap[pixel*3 + 1];
			pixels[i*3 + 2] = rawblock->colormap[pixel*3 + 2];
			// add to avg
			avg[0] += rawblock->colormap[pixel*3];
			avg[1] += rawblock->colormap[pixel*3 + 1];
			avg[2] += rawblock->colormap[pixel*3 + 2];
		}
		Print("loaded\n");
	}
	else
	{
		posx = 0;
		posy = 0;

		// try load as tile/TIM
		dec = (byte *)LzDec(&decSize, filedata, 0, filesize, true);
		if (dec == NULL)
		{
			dec = filedata;
			decSize = filesize;
		}

		// load TIM
		tim = TIM_LoadFromBuffer(dec, decSize);
		if (tim != NULL)
		{
			if (subpics != NULL)
			{
				stepx = (float)((float)tim->dim.xsize / (float)subpics->picwidth);
				stepy = (float)((float)tim->dim.ysize / (float)subpics->picheight);
				width = subpics->picwidth;
				height = subpics->picheight;
			}
			else
			{
				stepx = 1.0f;
				stepy = 1.0f;
				width = tim->dim.xsize;
				height = tim->dim.ysize;
			}

			if (tim->type != TIM_8Bit && tim->type != TIM_16Bit)
				Print("failed (not a 8 or 24-bit TIM)\n");
			else
			{
				pixels = (byte *)mem_alloc(width * height * 3);
				if (tim->type == TIM_8Bit)
				{
					for (i = 0; i < height; i++)
					{
						for (j = 0; j < width; j++)
						{
							pixel = tim->pixels[tim->dim.xsize*(int)(i*stepy) + (int)(j*stepx)];
							pixels[width*3*i + j*3] = (tim->CLUT->data[pixel*2] & 0x1F) * 8;
							pixels[width*3*i + j*3 + 1] = (((tim->CLUT->data[pixel*2] & 0xE0) >> 5) + ((tim->CLUT->data[pixel*2 + 1] & 0x3) << 3)) * 8;
							pixels[width*3*i + j*3 + 2] = ((tim->CLUT->data[pixel*2 + 1] & 0x7C) >> 2) * 8;
							// add to avg
							if (pixels[width*3*i + j*3] != 0 || pixels[width*3*i + j*3 + 1] || pixels[width*3*i + j*3 + 2])
							{
								avg[0] += pixels[width*3*i + j*3];
								avg[1] += pixels[width*3*i + j*3 + 1];
								avg[2] += pixels[width*3*i + j*3 + 2];
							}
						}
					}
					Print("loaded\n");
				}
				else if (tim->type == TIM_24Bit)
				{
					for (i = 0; i < height; i++)
					{
						for (j = 0; j < width; j++)
						{
							pixels[width*3*i + j*3]     = tim->pixels[tim->dim.xsize*3*(int)(i*stepy) + 3*(int)(j*stepx)];
							pixels[width*3*i + j*3 + 1] = tim->pixels[tim->dim.xsize*3*(int)(i*stepy) + 3*(int)(j*stepx) + 1];
							pixels[width*3*i + j*3 + 2] = tim->pixels[tim->dim.xsize*3*(int)(i*stepy) + 3*(int)(j*stepx) + 2];
							// add to avg
							if (pixels[width*3*i + j*3] != 0 || pixels[width*3*i + j*3 + 1] || pixels[width*3*i + j*3 + 2])
							{
								avg[0] += pixels[width*3*i + j*3];
								avg[1] += pixels[width*3*i + j*3 + 1];
								avg[2] += pixels[width*3*i + j*3 + 2];
							}
						}
					}
					Print("loaded\n");
				}
				else
				{
					Print("failed (not a 8-bit or 24-bit TIM)\n");
				}
			}
			FreeTIM(tim);
		}
		else
			Print("failed (not a TIM)\n");
	}
	FreeRawBlock(rawblock);
	mem_free(filedata);

	// write into array
	pic = &cachedpics[e];
	strncpy(pic->name, entryname, 23);
	pic->x = posx;
	pic->y = posy;
	pic->width = width;
	pic->height = height;
	pic->pixels = pixels;
	pic->subpics = subpics;
	pic->used = cached_usecounter;
	cached_usecounter++;

	// set average color, normalize
	pic->r = 128;
	pic->g = 128;
	pic->b = 128;
	if (width && height)
	{
		avg[0] = (byte)(avg[0] / (width * height));
		avg[1] = (byte)(avg[1] / (width * height));
		avg[2] = (byte)(avg[2] / (width * height));
		stepx = 0.0f + max(max(avg[0], avg[1]), avg[2]);
		if (stepx)
		{
			stepx = 64.0f / stepx;
			pic->r = min(255, max(0, (byte)(avg[0] * stepx)));
			pic->g = min(255, max(0, (byte)(avg[1] * stepx)));
			pic->b = min(255, max(0, (byte)(avg[2] * stepx)));
		}
	}
	return pic;
}

void FreePic(cachepic_t *cachepic)
{
	if (cachepic->subpics)
		mem_free(cachepic->subpics);
	if (cachepic->pixels)
		mem_free(cachepic->pixels);
	memset(cachepic, 0, sizeof(cachepic_t));
}


/*
==========================================================================================

 SCAN FOR A TILE OR MAP

==========================================================================================
*/

// returns 1 if map, 2 if tile, 0 otherwise
int MapScan(byte *buffer, int filelen)
{
	byte *dec;
	int decSize;

	// map or tile are compressed, decompress it
	dec = (byte *)LzDec(&decSize, buffer, 0, filelen, true);
	if (dec == NULL)
		return 0;
	// maps are dumped structs and have fixed file length
	if (decSize == sizeof(bo_map_t))
		return 1;
	// tiles are just compressed reqular TIM's
	if (dec[0] == 16 && dec[1] == 0 && dec[2] == 0 && dec[2] == 0)
		return 2;
	// unknown
	return 0;
}

void DeveloperData(char *caption, byte *data, int per_line, int num_lines, int line_skipstart, int line_skipend, bool developer)
{
	int i, j;

	// if (!developer)
	//	return;
	per_line = per_line - line_skipend;
	if (caption && caption[0])
	{
		printf("%s\n", caption);
		//for (i = 0; i < per_line; i++)
		//	printf("%3i ", i);
		//printf("\n");
	}
	for (i = 0; i < num_lines; i++)
	{
		printf("%3i: ", i);
		data += line_skipstart;
		for (j = line_skipstart; j < per_line; j++)
		{
			printf("%3i ", *data);
			data++;
		}
		data += line_skipend;
		printf("\n");
	}
}

/*
==========================================================================================

 DRAW PRIMITIVES

==========================================================================================
*/

typedef struct
{
	byte *data;
	int   datasize;
	int   bpp;
} map_renderbuffer_t;

map_renderbuffer_t *AllocateMapDrawImage(int bpp)
{
	map_renderbuffer_t *map_image;

	if (bpp != 3 && bpp != 4)
		Error("AllocateMapDrawImage: bad BPP %i\n", bpp);

	map_image = (map_renderbuffer_t *)mem_alloc(sizeof(map_renderbuffer_t));
	map_image->datasize = 80 * 32 * bpp * 32 * 80;
	map_image->data = (byte *)mem_alloc(map_image->datasize);
	map_image->bpp = bpp;
	return map_image;
}

void FreeMapDrawImage(map_renderbuffer_t *map_image)
{
	if (!map_image)
		return;
	mem_free(map_image->data);
	mem_free(map_image);
}


// draw lights list with interpolation
static void MapDrawLighting(map_renderbuffer_t *map_image, float ambient_r, float ambient_g, float ambient_b, bo_render_light_t *lights, int num_lights, bool outdoor, bool night, bool thunder)
{
	float buffer[82][82][3], flickbuf[82][82], *p1, *p2, *p3, *p4, d1[3], d2[3], d3[3], d4[4], a, b, c[3], mod;
	int i, j, col, row, posx, posy, sizex, sizey, rnd;
	bo_render_light_t *light;
	byte *out;

	#define VectorSet(v, a, b, c) (v[0] = a,v[1] = b,v[2] = c)
	#define VectorAdd(v, a, b, c) (v[0] += a,v[1] += b,v[2] += c)
	#define VectorSub(a, b, c) (c[0] = a[0] -  b[0],c[1] = a[1] -  b[1],c[2] = a[2] -  b[2])
	#define VectorMS(a, b, s, c) (c[0] = (a[0] -  b[0])*s,c[1] = (a[1] -  b[1])*s,c[2] = (a[2] -  b[2])*s)

	// initialize buffers
	//memset(map_image->data, 128, map_image->datasize); // uncomment to get all map gray (with lighting shown as bright zones)
	srand(5);
	if (outdoor == true && night == true)
	{
		ambient_r *= 0.7f;
		ambient_g *= 0.7f;
		ambient_b *= 1.0f;
	}
	for (row = 0; row <= 80; row++)
	{
		for (col = 0; col <= 80; col++)
		{
			// ambient level
			VectorSet(buffer[row][col], ambient_r, ambient_g, ambient_b);
			// fill flicker buffer
			rnd = rand() % 255;
			flickbuf[row][col] = rnd < 128 ? 1.0f : 2.0f;
		}
	}

	// render lights into buffer
	for (col = 0; col < num_lights; col++)
	{
		light = &lights[col];
		posx = light->x;
		posy = light->y;

		// thunder light?
		if (light->flags & LIGHTFLAG_THUNDER)
			if (!thunder)
				continue;
		// directional light (forward)
		if (light->flags & LIGHTFLAG_FORWARD)
		{
			sizex = posx + light->sizex; // initial width
			for (i = 0; i < light->sizey; i++) // steps
			{
				posy = max(0, light->y - i); // y position for this step
				for (j = posx; j <= sizex; j++)
				{
					mod = 1.0;
					if (light->flickering)
						mod *= flickbuf[j][posy];
					if (light->dynamic)
						VectorAdd(buffer[j][posy], light->r * mod, light->g * mod, light->b * mod);
					else
						VectorSet(buffer[j][posy], light->r * mod, light->g * mod, light->b * mod);
				}
				// adjust step
				posx = max(0, posx - 1); 
				sizex = min(80, sizex + 1);
			}
			continue;
		}
		// directional light (back)
		if (light->flags & LIGHTFLAG_BACK)
		{
			sizex = posx + light->sizex; // initial width
			for (i = 0; i < light->sizey; i++) // steps
			{
				posy = min(80, light->y + i); // y position for this step
				for (j = posx; j <= sizex; j++)
				{
					mod = 1.0;
					if (light->flickering)
						mod *= flickbuf[j][posy];
					if (light->dynamic)
						VectorAdd(buffer[j][posy], light->r * mod, light->g * mod, light->b * mod);
					else
						VectorSet(buffer[j][posy], light->r * mod, light->g * mod, light->b * mod);
				}
				// adjust step
				posx = max(0, posx - 1); 
				sizex = min(80, sizex + 1);
			}
			continue;
		}
		// directional light (right)
		if (light->flags & LIGHTFLAG_RIGHT)
		{
			sizey = posy + light->sizex; // initial height
			for (i = 0; i < light->sizey; i++) // steps
			{
				posx = min(80, light->x + i); // y position for this step
				for (j = posy; j <= sizey; j++)
				{
					mod = 1.0;
					if (light->flickering)
						mod *= flickbuf[posx][j];
					if (light->dynamic)
						VectorAdd(buffer[posx][j], light->r * mod, light->g * mod, light->b * mod);
					else
						VectorSet(buffer[posx][j], light->r * mod, light->g * mod, light->b * mod);
				}
				// adjust step
				posy = max(0, posy - 1); 
				sizey = min(80, sizey + 1);
			}
			continue;
		}
		// directional light (left)
		if (light->flags & LIGHTFLAG_LEFT)
		{
			sizey = posy + light->sizex; // initial height
			for (i = 0; i < light->sizey; i++) // steps
			{
				posx = max(0, light->x - i); // y position for this step
				for (j = posy; j <= sizey; j++)
				{
					mod = 1.0;
					if (light->flickering)
						mod *= flickbuf[posx][j];
					if (light->dynamic)
						VectorAdd(buffer[posx][j], light->r * mod, light->g * mod, light->b * mod);
					else
						VectorSet(buffer[posx][j], light->r * mod, light->g * mod, light->b * mod);
				}
				// adjust step
				posy = max(0, posy - 1); 
				sizey = min(80, sizey + 1);
			}
			continue;
		}
		// standart omnidirectional light x and y are dimensions
		sizex = min(posx + light->sizex - 1, 81);
		sizey = min(posy + light->sizey - 1, 81);
		for (i = posx; i <= sizex; i++)
		{
			for (j = posy; j <= sizey; j++)
			{
				mod = 1.0;
				if (light->flickering)
					mod *= flickbuf[i][j];
				if (light->dynamic)
					VectorAdd(buffer[i][j], light->r * mod, light->g * mod, light->b * mod);
				else
					VectorSet(buffer[i][j], light->r * mod, light->g * mod, light->b * mod);
			}
		}
	}

	// render tilemap with interpolation
	for (row = 0; row < 80; row++)
	{
		for (col = 0; col < 80; col++)
		{
			p1 = &buffer[row][col][0];
			p2 = &buffer[row + 1][col][0];
			p3 = &buffer[row][col + 1][0];
			p4 = &buffer[row + 1][col + 1][0];
			VectorSub(p2, p1, d1); // top-left -> top-right
			VectorSub(p3, p1, d2); // top->left -> bottom-left
			VectorSub(p3, p4, d3); // bottom->right -> top-right
			VectorSub(p2, p4, d4); // bottom->right -> bottom-left
			// render tile
			for (i = 0; i < 32; i++)
			{
				out = (byte *)map_image->data + 80*32*map_image->bpp * (32*col+i) + 32*map_image->bpp*row;
				for (j = 0; j < 32; j++, out += map_image->bpp)
				{
					a = (float)i/32.0f;
					b = (float)j/32.0f;
					if ((j + i) > 32) 
					{
						// second triangle
						c[0] = p4[0] + d3[0]*(1 - b) + d4[0]*(1 - a);
						c[1] = p4[1] + d3[1]*(1 - b) + d4[1]*(1 - a);
						c[2] = p4[2] + d3[2]*(1 - b) + d4[2]*(1 - a);

						out[0] = (byte)min(255, max(0, out[0] * c[0]));
						out[1] = (byte)min(255, max(0, out[1] * c[1]));
						out[2] = (byte)min(255, max(0, out[2] * c[2]));
					}
					else
					{
						// first triangle
						c[0] = p1[0] + d1[0]*b + d2[0]*a;
						c[1] = p1[1] + d1[1]*b + d2[1]*a;
						c[2] = p1[2] + d1[2]*b + d2[2]*a;

						out[0] = (byte)min(255, max(0, out[0] * c[0]));
						out[1] = (byte)min(255, max(0, out[1] * c[1]));
						out[2] = (byte)min(255, max(0, out[2] * c[2]));
					}
				}
			}
		}
	}
}

// draw lights list
/*
static void MapDrawLightingNearest(map_renderbuffer_t *map_image, float ambient_r, float ambient_g, float ambient_b, bo_render_light_t *lights, int num_lights)
{
	byte *out;
	int row, col, i, j;
	bo_render_light_t *light, *src;
	float r, g, b;

	for (row = 0; row < 80; row++)
	{
		for (col = 0; col < 80; col++)
		{
			// create chain of lights
			light = NULL;
			for (i = 0; i < num_lights; i++)
			{
				if (lights[i].x > row || lights[i].y > col || (lights[i].x + lights[i].sizex - 1)  < row || (lights[i].y + lights[i].sizey - 1) < col)
					continue;
				if (light != NULL)
					light->next = &lights[i];
				light = &lights[i];
				light->next = NULL;
			}
			// draw a single tile
			for (i = 0; i < 32; i++)
			{
				out = (byte *)map_image->data + 80*32*map_image->bpp * (32*col+i) + 32*map_image->bpp*row;
				for (j = 0; j < 32; j++, out += map_image->bpp)
				{
					// base lighting
					r = (float)out[0] * ambient_r;
					g = (float)out[1] * ambient_g;
					b = (float)out[2] * ambient_b;
					// apply lights
					for( src = light; src != NULL; src = src->next)
					{
						r += (float)out[0] * light->r;
						g += (float)out[1] * light->g;
						b += (float)out[2] * light->b;
					}
					// result color
					out[0] = (byte)min(r, 255);
					out[1] = (byte)min(g, 255);
					out[2] = (byte)min(b, 255);
				}
			}
		}
	}
}

// draw ambient lighting
static void MapDrawLightingAmbient(map_renderbuffer_t *map_image, float r, float g, float b)
{
	byte *out, *end;

	for(out = (byte *)map_image->data, end = out + map_image->datasize; out < end; out += map_image->bpp)
	{
		out[0] = (byte)min((float)out[0] * r, 255);
		out[1] = (byte)min((float)out[1] * g, 255);
		out[2] = (byte)min((float)out[2] * b, 255);
	}
}
*/

// fill tile with color
static void MapDrawColor(map_renderbuffer_t *map_image, int row, int col, byte r, byte g, byte b)
{
	byte i, j;

	for (i = 0; i < 32; i++)
	{
		byte *out = (byte *)map_image->data + 80*32*map_image->bpp * (32*col+i) + 32*map_image->bpp*row;
		for (j = 0; j < 32; j++, out += map_image->bpp)
		{
			out[0] = (byte)min(out[0] + r, 255);
			out[1] = (byte)min(out[1] + g, 255);
			out[2] = (byte)min(out[2] + b, 255);
			if (map_image->bpp == 4)
				out[3] = min(255, (int)(out[3] + 255));
		}
	}					
}

// draw a line from tile to tile
static void MapDrawLine(map_renderbuffer_t *map_image, float row1, float col1, float row2, float col2, byte r, byte g, byte b)
{
	int x1, x2, y1, y2, error, error2, deltaX, deltaY, signX, signY;

	row1 = min(80, max(0, row1));
	row2 = min(80, max(0, row2));
	col1 = min(80, max(0, col1));
	col2 = min(80, max(0, col2));
	x1 = (int)row1 * 32 + (int)((row1 - (int)row1)*32);
	x2 = (int)row2 * 32 + (int)((row2 - (int)row2)*32);
	y1 = (int)col1 * 32 + (int)((col1 - (int)col1)*32);
	y2 = (int)col2 * 32 + (int)((col2 - (int)col2)*32);

	deltaX = abs(x2 - x1);
    deltaY = abs(y2 - y1);
    signX = x1 < x2 ? 1 : -1;
    signY = y1 < y2 ? 1 : -1;
    error = deltaX - deltaY;
    for (;;)
    {
		map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1] = min(map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1] + r, 255);
		map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 1] = min(map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 1] + g, 255);
		map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 2] = min(map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 2] + b, 255);
		if (map_image->bpp == 4)
			map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 3] = min(map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 3] + 255, 255);
        if (x1 == x2 && y1 == y2)
            break;
        error2 = error * 2;
        if (error2 > -deltaY)
        {
            error -= deltaY;
            x1 += signX;
        }
        if(error2 < deltaX)
        {
            error += deltaX;
            y1 += signY;
        }
    }
}

// draw a dashed line from tile to tile
static void MapDrawDashedLine(map_renderbuffer_t *map_image, float row1, float col1, float row2, float col2, byte r, byte g, byte b)
{
	int x1, x2, y1, y2, error, error2, deltaX, deltaY, signX, signY, skip;

	row1 = min(80, max(0, row1));
	row2 = min(80, max(0, row2));
	col1 = min(80, max(0, col1));
	col2 = min(80, max(0, col2));
	x1 = (int)row1 * 32 + (int)((row1 - (int)row1)*32);
	x2 = (int)row2 * 32 + (int)((row2 - (int)row2)*32);
	y1 = (int)col1 * 32 + (int)((col1 - (int)col1)*32);
	y2 = (int)col2 * 32 + (int)((col2 - (int)col2)*32);

	deltaX = abs(x2 - x1);
    deltaY = abs(y2 - y1);
    signX = x1 < x2 ? 1 : -1;
    signY = y1 < y2 ? 1 : -1;
    error = deltaX - deltaY;
	skip = 0;
	for (;;)
    {
		if (skip >= 0)
		{
			map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1] = min(map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1] + r, 255);
			map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 1] = min(map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 1] + g, 255);
			map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 2] = min(map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 2] + b, 255);
			if (map_image->bpp == 4)
				map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 3] = min(map_image->data[80*32*map_image->bpp*y1 + map_image->bpp*x1 + 3] + 255, 255);
		}
		skip++;
		if (skip > 3)
			skip = -2;

        if (x1 == x2 && y1 == y2)
            break;
        error2 = error * 2;
        if (error2 > -deltaY)
        {
            error -= deltaY;
            x1 += signX;
        }
        if(error2 < deltaX)
        {
            error += deltaX;
            y1 += signY;
        }
    }
}

// fill the inner border
static void MapDrawBorder(map_renderbuffer_t *map_image, int row, int col, byte r, byte g, byte b, int margin, int width)
{
	int i, j, length;

	length = 32 - margin*2;
	// up
	for (i = 0; i < width; i++)
	{
		byte *out = (byte *)map_image->data + 80*32*map_image->bpp * (32*col+margin+i) + 32*map_image->bpp*row + map_image->bpp*margin;
		for (j = 0; j < length; j++, out += map_image->bpp)
		{
			out[0] = (byte)min(out[0] + r, 255);
			out[1] = (byte)min(out[1] + g, 255);
			out[2] = (byte)min(out[2] + b, 255);
			if (map_image->bpp == 4)
				out[3] = (byte)min((int)(out[3] + 255), 255);
		}
	}
	// down
	for (i = 0; i < width; i++)
	{
		byte *out = (byte *)map_image->data + 80*32*map_image->bpp * (32*col+32-1-margin-i) + 32*map_image->bpp*row + map_image->bpp*margin;
		for (j = 0; j < length; j++, out += map_image->bpp)
		{
			out[0] = (byte)min(out[0] + r, 255);
			out[1] = (byte)min(out[1] + g, 255);
			out[2] = (byte)min(out[2] + b, 255);
			if (map_image->bpp == 4)
				out[3] = (byte)min((int)(out[3] + 255), 255);
		}
	}
	// left
	length = length - width*2;
	for (i = 0; i < length; i++)
	{
		byte *out = (byte *)map_image->data + 80*32*map_image->bpp * (32*col+margin+width+i) + 32*map_image->bpp*row + map_image->bpp*margin;
		for (j = 0; j < width; j++, out += map_image->bpp)
		{
			out[0] = (byte)min(out[0] + r, 255);
			out[1] = (byte)min(out[1] + g, 255);
			out[2] = (byte)min(out[2] + b, 255);
			if (map_image->bpp == 4)
				out[3] = (byte)min((int)(out[3] + 255), 255);
		}
	}
	// right
	for (i = 0; i < length; i++)
	{
		byte *out = (byte *)map_image->data + 80*32*map_image->bpp * (32*col+margin+width+i) + 32*map_image->bpp*row + map_image->bpp*(32-margin-width);
		for (j = 0; j < width; j++, out += map_image->bpp)
		{
			out[0] = (byte)min(out[0] + r, 255);
			out[1] = (byte)min(out[1] + g, 255);
			out[2] = (byte)min(out[2] + b, 255);
			if (map_image->bpp == 4)
				out[3] = (byte)min((int)(out[3] + 255), 255);
		}
	}

}

// determin a shadow pixel in tilemap
// yes, this is bogus as shadows always uses index 0
#define IsShadowPixel(in) ((in[0] == 8 && in[1] == 16 && (in[2] == 32 || in[2] == 33)) || (in[0] == 9 && in[1] == 17 && in[2] == 36))

// draw a picture (sprite or tim)
static void MapDrawPic(map_renderbuffer_t *map_image, float row, float col, cachepic_t *pic, byte subpic, bool solid, bool align_center, float r, float g, float b)
{
	int i, j, x, y, startx, starty, width, height;
	byte *out, *in;

	if (!pic || !pic->pixels || !pic->width || !pic->height)
		return;

	// get whole pic or subpic
	if (pic->subpics != NULL)
	{
		if (subpic >= pic->subpics->numtex)
		{
			Print("MapDrawPic(%s): subpic %i out of range 0-%i!\n", pic->name, subpic, pic->subpics->numtex);
			startx = 0;
			starty = 0;
			width = pic->width;
			height = pic->height;
		}
		else
		{
			startx = pic->subpics->tex[subpic].x;
			starty = pic->subpics->tex[subpic].y;
			width = pic->subpics->tex[subpic].w;
			height = pic->subpics->tex[subpic].h;
		}
	}
	else
	{
		startx = 0;
		starty = 0;
		width = pic->width;
		height = pic->height;
	}
	
	// apply align, draw
	if (align_center)
	{
		x = (int)(32*(int)col + (col - (int)col)*32 + (32 - height)/2);
		y = (int)(32*(int)row + (row - (int)row)*32 + (32 - width)/2);
	}
	else
	{
		x = (int)(32*(int)col + (col - (int)col)*32 - pic->y);
		y = (int)(32*(int)row + (row - (int)row)*32 - pic->x);
		if (pic->subpics != NULL && subpic < pic->subpics->numtex)
		{
			x = x - pic->subpics->tex[subpic].ofsy; // this is correct
			y = y - pic->subpics->tex[subpic].ofsx;
		}
	}
	x = min(32*80, max(0, x));
	y = min(32*80, max(0, y));

	// solid draw
	if (solid)
	{
		for (i = 0; i < height; i++)
		{
			in = (byte *)pic->pixels + pic->width*3*(i + starty) + startx * 3;
			out = (byte *)map_image->data + 80*32*map_image->bpp*(x + i) + map_image->bpp*(y);
			for (j = 0; j < width; j++)
			{
				out[0] = (byte)min(255, max(0, in[0] * r));
				out[1] = (byte)min(255, max(0, in[1] * g));
				out[2] = (byte)min(255, max(0, in[2] * b));
				if (map_image->bpp == 4)
					out[3] = (byte)min(255, max(0, (int)(in[3] + 255)));	
				out += map_image->bpp;
				in += 3;
			}
		}
		return;
	}

	// bound draw pos to map size
	width = min(width, 2560 - y);
	height = min(height, 2560 - x);

	// draw with alpha and shadow
	for (i = 0; i < height; i++)
	{
		in = (byte *)pic->pixels + 3*(pic->width*(starty + i) + startx);
		out = (byte *)map_image->data + map_image->bpp*(80*32*(x + i) + y);
		for (j = 0; j < width; j++)
		{
			if (in[0] || in[1] || in[2])
			{
				if (IsShadowPixel(in))
				{
					out[0] = (byte)min(255, max(0, (float)(out[0] + in[0]) * 0.5 * r));
					out[1] = (byte)min(255, max(0, (float)(out[1] + in[1]) * 0.5 * g));
					out[2] = (byte)min(255, max(0, (float)(out[2] + in[2]) * 0.5 * b));
					if (map_image->bpp == 4)
						out[3] = (byte)min(255, max(0, (float)(out[3] + 128)));
				}	
				else 
				{
					out[0] = (byte)min(255, max(0, (float)in[0] * r));
					out[1] = (byte)min(255, max(0, (float)in[1] * g));
					out[2] = (byte)min(255, max(0, (float)in[2] * b));
					if (map_image->bpp == 4)
						out[3] = (byte)min(255, max(0, (float)(out[3] + 255)));
				}
			}
			out += map_image->bpp;
			in += 3;
		}
	}	
}

// draws a string
static void MapDrawString(map_renderbuffer_t *map_image, float row, float col, char *str, bool center_align, float r, float g, float b)
{
	float stringwidth, stringheight;
	cachepic_t *fontmap;
	int l, i;
	byte c;

	fontmap = CachePic("font.tim");
	if (!fontmap)
		Error("font not loaded!");
	l = strlen(str);

	// if align - get string width and compensate
	if (center_align)
	{
		stringwidth = 0;
		stringheight = 0;
		for (i = 0; i < l; i++)
		{
			if (str[i] >= 48 && str[i] <= 57)
				c = (byte)(str[i] - 48);
			else if (str[i] >= 65 && str[i] <= 90)
				c = (byte)(10 + str[i] - 65);
			else if (str[i] >= 97 && str[i] <= 122)
				c = (byte)(36 + str[i] - 97);
			else
			{
				if (str[i] == ' ')
				{
					stringwidth += 6.0f / 32.0f;
					stringheight = max(stringheight, 9.0f / 32.0f);
				}
				else if (str[i] == '	')
				{
					stringwidth += 18.0f / 32.0f;
					stringheight = max(stringheight, 9.0f / 32.0f);
				}
				continue;
			}
			stringwidth += (float)(fontmap->subpics->tex[c].w / 32.0f) + 1.0f/32.0f;
			stringheight = max(stringheight, (float)(fontmap->subpics->tex[c].h / 32.0f));
		}
		row = max(0, row - stringwidth * 0.5f);
		col = max(0, col - stringheight * 0.5f);
	}

	// draw string
	for (i = 0; i < l; i++)
	{
		if (str[i] >= 48 && str[i] <= 57)
			c = (byte)(str[i] - 48);
		else if (str[i] >= 65 && str[i] <= 90)
			c = (byte)(10 + str[i] - 65);
		else if (str[i] >= 97 && str[i] <= 122)
			c = (byte)(36 + str[i] - 97);
		else
		{
			if (str[i] == ' ')
				row += 6.0f / 32.0f;
			else if (str[i] == '	')
				row += 18.0f / 32.0f;
			continue;
		}
		MapDrawPic(map_image, row, col, fontmap, c, false, false, r, g, b);
		row += (float)(fontmap->subpics->tex[c].w / 32.0f) + 1.0f/32.0f;
	}
}

/*
==========================================================================================

 DRAW MACROS

==========================================================================================
*/

// draw a contents color block
static void Draw_Contents(map_renderbuffer_t *map_image, int row, int col, int contents, bool with_solid, bool with_triggers)
{
	byte color[3];

	if (!contents)
		return; // empty

	// get color of trigger type
	memset(color, 0, sizeof(color));
	switch(contents)
	{
		case CONTENTS_SOLID:
			if (with_solid)
			{
				color[0] = 60;
				color[1] = 60;
				color[2] = 64;
			}
			break;
		case CONTENTS_MISTWALK:
			if (with_solid)
			{
				color[0] = 16;
				color[1] = 32;
				color[2] = 64;
			}
			break;
		case CONTENTS_JUMPFENCE:
			if (with_solid)
			{
				color[0] = 32;
				color[1] = 32;
				color[2] = 64;
			}
			break;
		case CONTENTS_SPIKES:
		case CONTENTS_FIRE:
			if (with_solid)
			{
				color[0] = 64;
				color[1] = 0;
				color[2] = 0;
			}
			break;
		case CONTENTS_TRAPTELEPORT:
			if (with_solid)
			{
				color[0] = 16;
				color[1] = 0;
				color[2] = 0;
				MapDrawBorder(map_image, row, col, 255, 64, 64, 1, 1);
				MapDrawBorder(map_image, row, col, 255, 64, 64, 4, 1);
				MapDrawBorder(map_image, row, col, 255, 64, 64, 8, 1);
				MapDrawBorder(map_image, row, col, 255, 64, 64, 13, 1);
			}
			break;
		case CONTENTS_SAVEGAME:
		case CONTENTS_BATMARK:
		case CONTENTS_SACRIFICE:
			if (with_triggers)
			{
				color[0] = 40;
				color[1] = 40;
				color[2] = 10;
			}
			break;
		case CONTENTS_LAVA:
		case CONTENTS_WATER:
		case CONTENTS_SWAMP:
		case CONTENTS_JUMPWALL:
		case CONTENTS_ICE:
			break;
		default:
			Print("  unknown contents: %i\n", contents);
			if (with_solid)
			{
				color[0] = 128;
				color[1] = 0;
				color[2] = 128;
			}
			break;
	}
	MapDrawColor(map_image, row, col, color[0], color[1], color[2]);
}

// draw a trigger line to objects that can be activated
static void Draw_TriggerLine(bo_map_t *map, map_renderbuffer_t *map_image, int row, int col, unsigned short t, byte toggled_objects)
{
	int i;
	cachepic_t *pic;
	float org[2], f;
	char filename[256];
	byte color[3];

	if (t == 0xFFFF)
		return;

	// animated tiles
	for (i = 0; i < 100; i++)
	{
		if (map->atiles[i].targetnum != t)
			continue;
		MapDrawDashedLine(map_image, row + 0.5f, col + 0.5f,  map->atiles[i].x + 0.5f, map->atiles[i].y + 0.5f, 64, 64, 32);
		MapDrawBorder(map_image, map->atiles[i].x, map->atiles[i].y, 64, 64, 32, 10, 1);
	}
	// effects
	for (i = 0; i < 64; i++)
	{
		if (map->effects[i].targetnum != t)
			continue;
		// check if toggled
		if (map->effects[i].targetnum != 0xFFFF && map->effects[i].start_on == toggled_objects)
			continue;
		// get origin we should draw line to
		if (map->effects[i].sprite != 0xFF && map->effects[i].sprite < 8 &&  map->sprites[map->effects[i].sprite])
		{
			sprintf(filename, "eff%05i.sdr", map->sprites[map->effects[i].sprite]);
			pic = CachePic(filename);
			// normalize color
			f = (float)(128 / max(max(pic->r, pic->g), pic->b));
			color[0] = (byte)(pic->r * f);
			color[1] = (byte)(pic->g * f);
			color[2] = (byte)(pic->b * f);
			// get origin
			org[0] = map->effects[i].x  + 0.5f - (pic->x / 32.0f) + (pic->width / 64.0f);
			org[1] = map->effects[i].y  + 0.5f - (pic->y /32.0f) + (pic->height / 64.0f);
		}
		else
		{
			// normalize color
			float f = (float)(128 / max(max(map->effects[i].r, map->effects[i].g), map->effects[i].b));
			color[0] = (byte)(map->effects[i].r * f);
			color[1] = (byte)(map->effects[i].g * f);
			color[2] = (byte)(map->effects[i].b * f);
			// get origin
			org[0] = map->effects[i].lightposx + 0.5f;
			org[1] = map->effects[i].lightposy;
		}
		MapDrawDashedLine(map_image, row + 0.5f, col + 0.5f, org[0], org[1], color[0], color[1], color[2]);
	}
}

// draw a trigger border for objecfts that are activated from another level
static void Draw_NoTrigger(bo_map_t *map, map_renderbuffer_t *map_image, int row, int col, unsigned short t)
{
	int i;

	// trigger lines for monsters
	for (i = 0; i < 32; i++)
	{
		if (map->monsters[i].u1[12] == 0xFF)
			continue;
		if (map->monsters[i].target == t)
			return;
	}
	// trigger lines for items
	for (i = 0; i < 50; i++)
	{
		if (map->items[i].savenum == 0xFFFF)
			continue;
		if (map->items[i].target == t)
			return;
	}
	// trigger lines for buttons
	for (i = 0; i < 255; i++)
	{
		if (map->triggers[i].parm1 == 0xFFFF || map->triggers[i].type != TRIGGER_TOUCH)
			continue;
		if (map->buttons[map->triggers[i].parm2].target == t)
			return;
	}
	// there are no trigger targeting this, draw
	MapDrawBorder(map_image, row, col, 0, 128, 64, 10, 1);
	Print("  triggertarget %i called from other location/code\n", t);
}

// draw a path line
static void Draw_Path(map_renderbuffer_t *map_image, int row1, int col1, int row2, int col2, byte r, byte g, byte b, bool drawarrow)
{
	float start[2], end[2], dir[2], nd[2], n[2], yaw;

	if (row1 == row2 && col1 == col2)
		return;

	// dots
	MapDrawBorder(map_image, row2, col2, 255, 255, 255, 14, 2);
	// a line
	start[0] = (float)(row1 + 0.5f);
	start[1] = (float)(col1 + 0.5f);
	end[0] = (float)(row2 + 0.5f);
	end[1] = (float)(col2 + 0.5f);
	MapDrawLine(map_image, start[0], start[1], end[0], end[1], r, g, b);
	// direction vector
	dir[0] = (float)(end[0] - start[0]);
	dir[1] = (float)(end[1] - start[1]);
	nd[0] = dir[0] * (1.0f / (float)sqrt(dir[0]*dir[0] + dir[1]*dir[1]));
	nd[1] = dir[1] * (1.0f / (float)sqrt(dir[0]*dir[0] + dir[1]*dir[1]));
	// normal vector
	yaw  = (float)(atan2(dir[1], dir[0])) + 3.1416f/2.0f;
	n[0] = (float)cos(yaw);
	n[1] = (float)sin(yaw);
	// draw arrows
	if (drawarrow)
	{
		start[0] = start[0] + nd[0]*0.5f;
		start[1] = start[1] + nd[1]*0.5f;
		MapDrawLine(map_image, start[0], start[1], start[0] + n[0]*0.25f - nd[0]*0.25f, start[1] + n[1]*0.25f - nd[1]*0.25f, r, g, b);
		MapDrawLine(map_image, start[0], start[1], start[0] - n[0]*0.25f - nd[0]*0.25f, start[1] - n[1]*0.25f - nd[1]*0.25f, r, g, b);
	}
}

/*
==========================================================================================

 MAP EXTRACTION

==========================================================================================
*/
				
int MapExportTGA(char *mapfile, byte *fileData, int fileDataSize, char *outfile, bigfileheader_t *bigfileheader, FILE *bigfile, char *tilespath, bool with_solid, bool with_triggers, bool with_lighting, bool show_save_id, byte toggled_objects, bool developer, int devnum, bool group_sections_by_path)
{
	int i, j, decSize;
	int map_mincol, map_minrow, map_maxcol, map_maxrow;
	unsigned short tilepix, tilegroup, map_num, map_section;
	char filename[MAX_OSPATH], path[MAX_OSPATH], mapname[32], s[256], *picname;
	byte *dec, contents, subpic;
	map_renderbuffer_t *map_image;
	cachepic_t *pic;
	float f;
	bo_map_t map;

	CachePic_Globals(tilespath, bigfileheader, bigfile)

	// random is used
	srand(0);

	// extract map (get mapnum and section)
	ExtractFileName(mapfile, mapname);
	StripFileExtension(mapname, mapname);
	map_num = 0;
	map_section = 0;
	if (mapname[0] != 'm' || strlen(mapname) != 8)
		Print("warning: %s is not valid, map num and section is not known\n", mapname);
	else
	{
		// get map and section
		map_section = atoi((char *)(mapname + 6));
		subpic = mapname[6];
		mapname[6] = 0;
		map_num = atoi((char *)(mapname + 1));
		mapname[6] = subpic;
	}
	Print("map %i\n", map_num);
	Print("section %i\n", map_section);

	// decompress
	dec = (byte *)LzDec(&decSize, fileData, 0, fileDataSize, true);
	if (dec == NULL)
		return 0;
	// should have fixed size
	if (decSize != sizeof(bo_map_t))
		return 0;

	memcpy(&map, dec, sizeof(bo_map_t));
	map_mincol = 80;
	map_minrow = 80;
	map_maxcol = 0;
	map_maxrow = 0;
	map_image = AllocateMapDrawImage(3);

	// draw world
	// we are splitting world rendering to background (before everything) and foreground (after monsters)
	// so 'alwaysontop' walls will draw over characters and items
	// however, this is not the way game does it, 
	// because it should draw tile-per-tick to achieve most accurate sorting
	Print("Assembling world back...\n");
	for (i = 0; i < 80; i++)
	{
		for (j = 0; j < 80; j++)
		{
			// tile1
			tilepix = map.backtiles[i][j];
			if (tilepix != 0xFFFF && !(tilepix & (TILEFLAG_NODRAW + TILEFLAG_ALWAYSONTOP)))
			{
				map_mincol = min(i, map_mincol);
				map_minrow = min(j, map_minrow);
				map_maxcol = max(i, map_maxcol);
				map_maxrow = max(j, map_maxrow);
				// draw tile
				tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
				sprintf(filename, "grp%05i.ctm", map.tilemaps[tilegroup]);
				pic = CachePic(filename);
				if (pic)
				{
					pic->subpics = &subpics_grp; // always use standart tilemap 
					MapDrawPic(map_image, (float)i, (float)j, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), true, false, 1.0f, 1.0f, 1.0f);
				}
			}
			// tile2
			tilepix = map.foretiles[i][j];
			if (tilepix != 0xFFFF && !(tilepix & (TILEFLAG_NODRAW + TILEFLAG_ALWAYSONTOP)))
			{
				map_mincol = min(i, map_mincol);
				map_minrow = min(j, map_minrow);
				map_maxcol = max(i, map_maxcol);
				map_maxrow = max(j, map_maxrow);
				// draw tile
				tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
				sprintf(filename, "grp%05i.ctm", map.tilemaps[tilegroup]);
				pic = CachePic(filename);
				if (pic)
				{
					pic->subpics = &subpics_grp; // always use standart tilemap 
					MapDrawPic(map_image, (float)i, (float)j, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), false, false, 1.0f, 1.0f, 1.0f);
				}
			}
		}
	}
	for (i = 0; i < 100; i++)
	{
		if (map.atiles[i].targetnum == 0xFFFF)
			continue;
		tilepix = (toggled_objects ? map.atiles[i].tile2 : map.atiles[i].tile1);
		if (tilepix != 0xFFFF && !(tilepix & (TILEFLAG_NODRAW + TILEFLAG_ALWAYSONTOP)))
		{
			// draw tile
			tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
			sprintf(filename, "grp%05i.ctm", map.tilemaps[tilegroup]);
			pic = CachePic(filename);
			if (pic)
			{
				pic->subpics = &subpics_grp; // always use standart tilemap 
				MapDrawPic(map_image, (float)map.atiles[i].x, (float)map.atiles[i].y, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), false, false, 1.0f, 1.0f, 1.0f);
			}
		}
	}
	// end background

	// buttons
	// uses additional data:
	// 1) touch triggers (sets origin of button)
	// 2) animated tiles it targets
	//DeveloperData("buttons:", (byte *)&map.buttons, 12, 20, 0, 0, developer);
	Print("Assembling buttons...\n");
	for (i = 0; i < 20; i++)
	{
		if (!map.buttons[i].savenum || map.buttons[i].savenum == 0xFFFF)
			continue;
		tilepix = (toggled_objects ? map.buttons[i].tile2 : map.buttons[i].tile1);
		if (tilepix != 0xFFFF && !(tilepix & TILEFLAG_NODRAW))
		{
			// find script for it and draw
			for (j = 0; j < 256; j++)
				if (map.triggers[j].type == TRIGGER_TOUCH && map.triggers[j].parm2 == i)
					break;
			if (j >= 256)
				Print("Failed to find a script for button %i (triggergroup %i)\n", i, map.buttons[i].target);
			else
			{
				tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
				sprintf(filename, "grp%05i.ctm", map.tilemaps[tilegroup]);
				pic = CachePic(filename);
				if (pic)
				{
					pic->subpics = &subpics_grp; // always use standart tilemap 
					MapDrawPic(map_image, (float)map.triggers[j].x, (float)map.triggers[j].y, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), false, false, 1.0f, 1.0f, 1.0f);
				}
			}
		}
	}

	// scenery
	Print("Assembling scenery...\n");
	//DeveloperData(NULL, (byte *)&map.scenery, 28, 256, 0, 0, developer);
	for (i = 0; i < 256; i++)
	{
		if (!map.scenery[i].active)
			continue;
		tilepix = (toggled_objects ? (map.scenery[i].toggled ? map.scenery[i].tile2 : (map.scenery[i].destructible ? map.scenery[i].tile3 : map.scenery[i].tile1)) : map.scenery[i].tile1);
		if (tilepix != 0xFFFF && !(tilepix & TILEFLAG_NODRAW))
		{
			tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
			sprintf(filename, "grp%05i.ctm", map.tilemaps[tilegroup]);
			pic = CachePic(filename);
			if (!pic)
				MapDrawColor(map_image, map.scenery[i].x, map.scenery[i].y, 128, 0, 128);
			else
			{
				subpic =  (tilepix & TILEFLAG_IMASK) - (tilegroup * 64);
				if (tilegroup >= 8 || subpic >= 32)
				{
					Print("  %s have bad tilegroup/subpic %i/%i!\n", pic->name, tilegroup, subpic);
					MapDrawColor(map_image, map.scenery[i].x, map.scenery[i].y, 128, 0, 128);
				}
				else
				{
					// build a single group object
					// DeveloperData(NULL, (byte *)&map.grpobjects[tilegroup][subpic], 8, 1, 0, 0, developer);
					pic->subpics = &subpics_grpobject;
					pic->subpics->tex[0].x = map.grpobjects[tilegroup][subpic].x;
					pic->subpics->tex[0].y = map.grpobjects[tilegroup][subpic].y;
					pic->subpics->tex[0].w = min(map.grpobjects[tilegroup][subpic].w, 256 - map.grpobjects[tilegroup][subpic].x);
					pic->subpics->tex[0].h = min(map.grpobjects[tilegroup][subpic].h, 256 -  map.grpobjects[tilegroup][subpic].y);
					pic->subpics->tex[0].ofsx = map.grpobjects[tilegroup][subpic].ofsx * 32;
					pic->subpics->tex[0].ofsy = map.grpobjects[tilegroup][subpic].ofsy * 32;
					MapDrawPic(map_image, (float)map.scenery[i].x, (float)map.scenery[i].y, pic, 0, false, false, 1.0f, 1.0f, 1.0f);
				}
			}
		}
	}

	// items
	Print("Assembling items...\n");
	//DeveloperData("items: ", (byte *)&map.items, 12, 50, 0, 0, developer);
	for (i = 0; i < 50; i++)
	{
		if (map.items[i].savenum == 0xFFFF)
			continue;
		switch(map.items[i].itemcode)
		{
			case MAPITEM_SMALLBLOODFLASK:
				picname = "smallpw.tim"; // VorteX: this file not exist, its forced to load powerup.tim on low resolution to simulate lesser flask
				subpic = 0;
				break;
			case MAPITEM_BLOODFLASK:
				picname = "powerup.tim";
				subpic = 1;
				break;
			case MAPITEM_RUNEPYRAMID:
				picname = "powerup.tim";
				subpic = 0;
				break;
			case MAPITEM_PURPLESPHERE:
				picname = "powerup.tim";
				subpic = 3;
				break;
			case MAPITEM_BLUESPHERE:
				picname = "powerup.tim";
				subpic = 4;
				break;
			case MAPITEM_GREENSPHERE:
				picname = "powerup.tim";
				subpic = 5;
				break;
			case MAPITEM_GOLDSPHERE:
				picname = "powerup.tim";
				subpic = 6;
				break;
			case MAPITEM_REDSPHERE:
				picname = "powerup.tim";
				subpic = 7;
				break;
			case MAPITEM_ANCIENTVIAL:
				picname = "powerup.tim";
				subpic = 2;
				break;
			case MAPITEM_UNIQUE1:
			case MAPITEM_UNIQUE2:
			case MAPITEM_UNIQUE3:
			case MAPITEM_UNIQUE4:
				subpic = map.uniqueitems[map.items[i].itemcode - MAPITEM_UNIQUE1];
				if (subpic == 0xFF)
					Print("  unique item %i is not mapped!\n", subpic);
				else
				{
					     if (subpic == ITEM_SPELL_INSPIREHATE)  picname = "scard00.sdr";
					else if (subpic == ITEM_SPELL_STUN)         picname = "scard01.sdr";
					else if (subpic == ITEM_SPELL_SPIRITWRACK)  picname = "scard02.sdr";
					else if (subpic == ITEM_SPELL_BLOODGOUT)    picname = "scard03.sdr";
					else if (subpic == ITEM_SPELL_BLOODSHOWER)  picname = "scard04.sdr";
					else if (subpic == ITEM_SPELL_SPIRITDEATH)  picname = "scard05.sdr";
					else if (subpic == ITEM_SPELL_LIGHT)        picname = "scard06.sdr";
					else if (subpic == ITEM_SPELL_LIGHTNING)    picname = "scard07.sdr";
					else if (subpic == ITEM_SPELL_REPEL)        picname = "scard08.sdr";
					else if (subpic == ITEM_SPELL_ENERGYBOLT)   picname = "scard09.sdr";
					else if (subpic == ITEM_SPELL_INCAPACITATE) picname = "scard10.sdr";
					else if (subpic == ITEM_SPELL_CONTROLMIND)  picname = "scard11.sdr";
					else if (subpic == ITEM_SPELL_SANCTUARY)    picname = "scard12.sdr";
					else if (subpic == ITEM_ARTIFACT_FLAY)      picname = "scard13.sdr";
					else if (subpic == ITEM_ARTIFACT_PENTALICH) picname = "scard14.sdr";
					else if (subpic == ITEM_ARTIFACT_IMPLODE)   picname = "scard15.sdr";
					else if (subpic == ITEM_ARTIFACT_FONT)      picname = "scard16.sdr";
					else if (subpic == ITEM_ARTIFACT_BANK)      picname = "scard17.sdr";
					else if (subpic == ITEM_ARTIFACT_HEART)     picname = "scard18.sdr";
					else if (subpic == ITEM_ARTIFACT_ANTITOXIN) picname = "scard19.sdr";
					else if (subpic == ITEM_ARTIFACT_SLOWTIME)  picname = "scard20.sdr";
					else if (subpic == ITEM_TOKEN_MOEBIUS)      picname = "scard21.sdr";
					else if (subpic == ITEM_TOKEN_DEJOULE)      picname = "scard22.sdr";
					else if (subpic == ITEM_TOKEN_DOLL)         picname = "scard23.sdr";
					else if (subpic == ITEM_TOKEN_MALEK)        picname = "scard24.sdr";
					else if (subpic == ITEM_TOKEN_AZIMUTH)      picname = "scard25.sdr";
					else if (subpic == ITEM_TOKEN_MORTANIUS)    picname = "scard26.sdr";
					else if (subpic == ITEM_TOKEN_NUPRAPTOR)    picname = "scard27.sdr";
					else if (subpic == ITEM_TOKEN_BANE)         picname = "scard28.sdr";
					else if (subpic == ITEM_TOKEN_SIGNETRING)   picname = "scard29.sdr";
					else if (subpic == ITEM_TOKEN_TIMEDEVICE)   picname = "scard30.sdr";
					else if (subpic == ITEM_TOKEN_ANACROTHE)    picname = "scard31.sdr";
					else if (subpic == ITEM_WEAPON_MACE)        picname = "scard33.sdr";
					else if (subpic == ITEM_WEAPON_AXES)        picname = "scard34.sdr";
					else if (subpic == ITEM_WEAPON_FLAMESWORD)  picname = "scard35.sdr";
					else if (subpic == ITEM_WEAPON_SOULREAVER)  picname = "scard36.sdr";
					else if (subpic == ITEM_ARMOR_BONE)         picname = "scard38.sdr";
					else if (subpic == ITEM_ARMOR_CHAOS)        picname = "scard39.sdr";
					else if (subpic == ITEM_ARMOR_FLESH)        picname = "scard40.sdr";
					else if (subpic == ITEM_ARMOR_WRAITH)       picname = "scard41.sdr";
					else if (subpic == ITEM_FORM_BAT)           picname = "scard43.sdr";
					else if (subpic == ITEM_FORM_WOLF)          picname = "scard44.sdr";
					else if (subpic == ITEM_FORM_DISGUISE)      picname = "scard45.sdr";
					else if (subpic == ITEM_FORM_MIST)          picname = "scard46.sdr";
					else if (subpic == ITEM_ENDING_1)	        picname = "scard59.sdr";
					else if (subpic == ITEM_ENDING_2)           picname = "scard60.sdr";
					else
					{
						Print("  unknown unique item %i!\n", subpic);
						picname = "pcard48.sdr";
					}
				}
				subpic = 0;
				break;
			default:
				Print("  unknown item code %i!\n", map.items[i].itemcode);
				picname = "pcard48.sdr";
				subpic = 0;
				break;
		}
		pic = CachePic(picname);
		if (pic)
			MapDrawPic(map_image, map.items[i].x, map.items[i].y, pic, subpic, false, true, 1.0f, 1.0f, 1.0f);
		else
			MapDrawColor(map_image, map.items[i].x, map.items[i].y, 0, 128, 0);
	}

	// monsters
	Print("Assembling monsters...\n");
	//DeveloperData("monsters: ", (byte *)&map.monsters, 164, 32, 0, 0, developer);
	for (i = 0; i < 32; i++)
	{
		if (map.monsters[i].u1[12] == 0xFF)
			continue;
		// monster pic
		sprintf(filename, "char%04i.sha", map.monsters[i].charnum);
		pic = CachePic(filename);
		if (!pic)
			MapDrawColor(map_image, map.monsters[i].x, map.monsters[i].y, 64, 128, 128);
		else
			MapDrawPic(map_image, map.monsters[i].x + 0.5f, map.monsters[i].y + 0.5f, pic, 0, false, false, 1.0f, 1.0f, 1.0f);
	}

	// effects, defines 2 things:
	// 1) animated sprites
	// 2) lightning (applied later after foreground)
	//DeveloperData("effects:", (byte *)&map.effects, sizeof(bo_effect_t), 64, 0, 0, developer);
	Print("Assembling effects...\n");
	for (i = 0; i < 64; i++)
	{
		if (map.effects[i].sprite == 0xFF)
			continue;
		// check if toggled
		if (map.effects[i].targetnum != 0xFFFF && map.effects[i].start_on == toggled_objects)
			continue;
		if (map.environments == ENVIRONMENTS_OUTDOOR && map.effects[i].flags & EFFECTFLAG_LIGHT && toggled_objects == 0) // outdoor locations have light drawn on night only
			continue;
		// draw sprite
		if (map.effects[i].sprite >= 8)
			Print("  sprite %i is out of bounds on %i!\n", map.effects[i].sprite, i);
		else if (map.sprites[map.effects[i].sprite])
		{
			sprintf(filename, "eff%05i.sdr", map.sprites[map.effects[i].sprite]);
			pic = CachePic(filename);
			if (pic)
				MapDrawPic(map_image, map.effects[i].x + 0.5f, map.effects[i].y + 0.5f, pic, 0, false, false, 1.0f, 1.0f, 1.0f);
		}
		// draw parms
		/*
		sprintf(s, "effect %i %i", map.sprites[map.effects[i].sprite], map.effects[i].flags);
		if (map.effects[i].flags & EFFECTFLAG_LIGHT)
			MapDrawString(map_image,  map.effects[i].x + 0.5f, map.effects[i].y + 1.5f, s, true, (float)map.effects[i].r / 15.0f, (float)map.effects[i].g / 15.0f, (float)map.effects[i].b / 15.0f);
		else
			MapDrawString(map_image,  map.effects[i].x + 0.5f, map.effects[i].y + 1.5f, s, true, 1.5f, 1.5f, 0.7f);
		*/
	}

	// foreground
	// same as background but does draw contents (optional)
	// and only draw tiles which have flag 'alwaysontop'
	Print("Assembling world fore...\n");
	for (i = 0; i < 80; i++)
	{
		for (j = 0; j < 80; j++)
			{
				// tile1
			tilepix = map.backtiles[i][j];
			if (tilepix != 0xFFFF && !(tilepix & TILEFLAG_NODRAW) && (tilepix & TILEFLAG_ALWAYSONTOP))
			{
				map_mincol = min(i, map_mincol);
				map_minrow = min(j, map_minrow);
				map_maxcol = max(i, map_maxcol);
				map_maxrow = max(j, map_maxrow);
				// draw tile
				tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
				sprintf(filename, "grp%05i.ctm", map.tilemaps[tilegroup]);
				pic = CachePic(filename);
				if (pic)
				{
					pic->subpics = &subpics_grp; // always use standart tilemap 
					MapDrawPic(map_image, (float)i, (float)j, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), true, false, 1.0f, 1.0f, 1.0f);
				}
			}
			// tile2
			tilepix = map.foretiles[i][j];
			if (tilepix != 0xFFFF && !(tilepix & TILEFLAG_NODRAW) && (tilepix & TILEFLAG_ALWAYSONTOP))
			{
				map_mincol = min(i, map_mincol);
				map_minrow = min(j, map_minrow);
				map_maxcol = max(i, map_maxcol);
				map_maxrow = max(j, map_maxrow);
				// draw tile
				tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
				sprintf(filename, "grp%05i.ctm", map.tilemaps[tilegroup]);
				pic = CachePic(filename);
				if (pic)
				{
					pic->subpics = &subpics_grp; // always use standart tilemap 
					MapDrawPic(map_image, (float)i, (float)j, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), false, false, 1.0f, 1.0f, 1.0f);
				}
			}
		}
	}

	// animated tiles
	for (i = 0; i < 100; i++)
	{
		if (map.atiles[i].targetnum == 0xFFFF)
			continue;
		tilepix = (toggled_objects ? map.atiles[i].tile2 : map.atiles[i].tile1);
		if (tilepix != 0xFFFF && !(tilepix & TILEFLAG_NODRAW) && (tilepix & TILEFLAG_ALWAYSONTOP))
		{
			// draw tile
			tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
			sprintf(filename, "grp%05i.ctm", map.tilemaps[tilegroup]);
			pic = CachePic(filename);
			if (pic)
			{
				pic->subpics = &subpics_grp; // always use standart tilemap 
				MapDrawPic(map_image, (float)map.atiles[i].x, (float)map.atiles[i].y, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), false, false, 1.0f, 1.0f, 1.0f);
			}
		}
		// draw contents
		contents = toggled_objects ? map.atiles[i].contents2 : map.atiles[i].contents1;
		if (contents != map.contents[map.atiles[i].x][map.atiles[i].y])
			Draw_Contents(map_image, map.atiles[i].x, map.atiles[i].y, contents, with_solid, with_triggers);
	}

	// lighting
	if (with_lighting)
	{
		int max_lights, num_lights;
		bo_render_light_t *lights, *light;
		Print("Apply lighting...\n");
		// allocate lights
		num_lights = 0;
		max_lights = 256;
		lights = (bo_render_light_t *)malloc(sizeof(bo_render_light_t) * max_lights);
		memset(lights, 0, sizeof(bo_render_light_t) * max_lights);
		// create lights for effects
		for (i = 0; i < 64; i++)
		{
			if (map.effects[i].sprite == 0xFF)
				continue;
			// check if toggled
			if (map.effects[i].targetnum != 0xFFFF && map.effects[i].start_on == toggled_objects)
				continue;
			// add light
			if (map.effects[i].flags & EFFECTFLAG_LIGHT)
			{
				if (map.environments == ENVIRONMENTS_OUTDOOR && toggled_objects == 0) // outdoor locations have light drawn on night only
					continue;
				if (num_lights < max_lights)
				{
					light = &lights[num_lights++];
					light->x = map.effects[i].lightposx;
					light->y = map.effects[i].lightposy;
					light->sizex = map.effects[i].lightwidth;
					light->sizey = map.effects[i].lightheight;
					light->r = (float)map.effects[i].r / 15.0f;
					light->g = (float)map.effects[i].g / 15.0f;
					light->b = (float)map.effects[i].b / 15.0f;
					light->flags = map.effects[i].lightflags;
					light->flickering = (map.effects[i].flags & EFFECTFLAG_FLICKER) ? true : false;
					light->dynamic = (map.effects[i].flags & EFFECTFLAG_DYNAMIC) ? true : false;
				}	
			}
		}
		// create lights for items
		for (i = 0; i < 50; i++)
		{
			if (map.items[i].savenum == 0xFFFF)
				continue;
			switch(map.items[i].itemcode)
			{
				case MAPITEM_SMALLBLOODFLASK:
					light = &lights[num_lights++];
					light->x = map.items[i].x;
					light->y = map.items[i].y;
					light->sizex = 2;
					light->sizey = 2;
					light->r = 0.6f;
					light->g = 0.2f;
					light->b = 0.2f;
					light->dynamic = true;
					break;
				case MAPITEM_BLOODFLASK:
					light = &lights[num_lights++];
					light->x = map.items[i].x;
					light->y = map.items[i].y;
					light->sizex = 2;
					light->sizey = 2;
					light->r = 0.6f;
					light->g = 0.2f;
					light->b = 0.2f;
					light->dynamic = true;
					break;
				case MAPITEM_RUNEPYRAMID:
					light = &lights[num_lights++];
					light->x = map.items[i].x;
					light->y = map.items[i].y;
					light->sizex = 2;
					light->sizey = 2;
					light->r = 0.4f;
					light->g = 0.4f;
					light->b = 1.0f;
					light->dynamic = true;
					break;
				case MAPITEM_PURPLESPHERE:
					light = &lights[num_lights++];
					light->x = map.items[i].x;
					light->y = map.items[i].y;
					light->sizex = 2;
					light->sizey = 2;
					light->r = 1.5f;
					light->g = 0.3f;
					light->b = 1.6f;
					light->dynamic = true;
					break;
				case MAPITEM_BLUESPHERE:
					light = &lights[num_lights++];
					light->x = map.items[i].x;
					light->y = map.items[i].y;
					light->sizex = 2;
					light->sizey = 2;
					light->r = 0.25f;
					light->g = 0.75f;
					light->b = 2.0f;
					light->dynamic = true;
					break;
				case MAPITEM_GREENSPHERE:
					light = &lights[num_lights++];
					light->x = map.items[i].x;
					light->y = map.items[i].y;
					light->sizex = 2;
					light->sizey = 2;
					light->r = 0.2f;
					light->g = 1.3f;
					light->b = 0.2f;
					light->dynamic = true;
					break;
				case MAPITEM_GOLDSPHERE:
					light = &lights[num_lights++];
					light->x = map.items[i].x;
					light->y = map.items[i].y;
					light->sizex = 2;
					light->sizey = 2;
					light->r = 2.0f;
					light->g = 1.0f;
					light->b = 0.0f;
					light->dynamic = true;
					break;
				case MAPITEM_REDSPHERE:
					light = &lights[num_lights++];
					light->x = map.items[i].x;
					light->y = map.items[i].y;
					light->sizex = 2;
					light->sizey = 2;
					light->r = 1.5f;
					light->g = 0.5f;
					light->b = 0.5f;
					light->dynamic = true;
					break;
				case MAPITEM_ANCIENTVIAL:
					light = &lights[num_lights++];
					light->x = map.items[i].x;
					light->y = map.items[i].y;
					light->sizex = 2;
					light->sizey = 2;
					light->r = 0.8f;
					light->g = 0.8f;
					light->b = 0.4f;
					light->dynamic = true;
					break;
				default:
					break;
			}
		}
		// draw lighting
		MapDrawLighting(map_image, (float)map.ambientcolor[0] / 15.0f, (float)map.ambientcolor[1] / 15.0f, (float)map.ambientcolor[2] / 15.0f, lights, num_lights, map.environments == ENVIRONMENTS_OUTDOOR ? true : false, toggled_objects != 0, toggled_objects != 0);
		// free lights
		free(lights);
	}
	// end foreground

	// content zones
	Print("Assembling contents...\n");
	for (i = 0; i < 80; i++) // rows 
		for (j = 0; j < 80; j++) // columns
			Draw_Contents(map_image, i, j, map.contents[i][j], with_solid, with_triggers);
	
	// effects
	/*
	Print("Assembling lights...\n");
	if (with_triggers)
	{
		for (i = 0; i < 64; i++)
		{
			if (map.effects[i].sprite == 0xFF)
				continue;
			// check if toggled
			if (map.effects[i].targetnum != 0xFFFF && map.effects[i].start_on == toggled_objects)
				continue;
			// draw light
			if (map.effects[i].r || map.effects[i].g || map.effects[i].b)
			{
				// normalize color
				float f = (float)(128 / max(max(map.effects[i].r, map.effects[i].g), map.effects[i].b));
				color[0] = (byte)(map.effects[i].r * f);
				color[1] = (byte)(map.effects[i].g * f);
				color[2] = (byte)(map.effects[i].b * f);
				if (!with_lighting)
				{
					// draw light center (we shift 0.5 up to not make it overrid other trigger marks)
					MapDrawLine(map_image, map.effects[i].lightposx + 0.25f, map.effects[i].lightposy - 0.25f, map.effects[i].lightposx + 0.75f, map.effects[i].lightposy + 0.25f, color[0], color[1], color[2]);
					MapDrawLine(map_image, map.effects[i].lightposx + 0.75f, map.effects[i].lightposy - 0.25f, map.effects[i].lightposx + 0.25f, map.effects[i].lightposy + 0.25f, color[0], color[1], color[2]);
					MapDrawLine(map_image, map.effects[i].lightposx + 0.50f, map.effects[i].lightposy + 0.40f, map.effects[i].lightposx + 0.50f, map.effects[i].lightposy - 0.40f, color[0], color[1], color[2]);
					MapDrawLine(map_image, map.effects[i].lightposx + 0.10f, map.effects[i].lightposy + 0.00f, map.effects[i].lightposx + 0.90f, map.effects[i].lightposy + 0.00f, color[0], color[1], color[2]);
				}
				// draw a line from effect center to light (if there is effect)
				if (map.effects[i].sprite != 0xFF && map.effects[i].sprite < 8 &&  map.sprites[map.effects[i].sprite])
				{
					sprintf(filename, "eff%05i.sdr", map.sprites[map.effects[i].sprite]);
					pic = CachePic(filename);
					// normalize color
					f = (float)(128 / max(max(pic->r, pic->g), pic->b));
					color[0] = (byte)(pic->r * f);
					color[1] = (byte)(pic->g * f);
					color[2] = (byte)(pic->b * f);
					// draw a line from sprite center to light
					MapDrawLine(map_image, map.effects[i].x  + 0.5f - (pic->x / 32.0f) + (pic->width / 64.0f), map.effects[i].y  + 0.5f - (pic->y /32.0f) + (pic->height / 64.0f), map.effects[i].lightposx + 0.5f, map.effects[i].lightposy + 0.0f, color[0], color[1], color[2]);
				}
			}
		}
	}
	*/

	// triggers
	Print("Assembling triggers...\n");
	//DeveloperData("triggers", (byte *)&map.triggers, sizeof(bo_trigger_t), 255, 0, 0, developer);
	for (i = 0; i < 255; i++)
	{
		if (!map.triggers[i].type || map.triggers[i].type == 0xFF)
			continue;
		map_mincol = min(map.triggers[i].x, map_mincol);
		map_minrow = min(map.triggers[i].y, map_minrow);
		map_maxcol = max(map.triggers[i].x, map_maxcol);
		map_maxrow = max(map.triggers[i].y, map_maxrow);
		switch(map.triggers[i].type)
		{
			case TRIGGER_TOUCH:
				if (with_triggers)
					MapDrawBorder(map_image, map.triggers[i].x, map.triggers[i].y, 128, 128, 64, 2, 2);
				break;
			case TRIGGER_EXIT:
			case TRIGGER_ENTRANCE:
				if (with_triggers)
				{
					if (map.triggers[i].type == TRIGGER_EXIT)
					{
						MapDrawBorder(map_image, map.triggers[i].x, map.triggers[i].y, 64, 0, 0, 1, 2);
						MapDrawBorder(map_image, map.triggers[i].x, map.triggers[i].y, 64, 0, 0, 8, 1);
					}
					else
					{
						MapDrawBorder(map_image, map.triggers[i].x, map.triggers[i].y, 0, 64, 0, 1, 2);
						MapDrawBorder(map_image, map.triggers[i].x, map.triggers[i].y, 0, 64, 0, 8, 1);
					}
					sprintf(filename, "%03i", map.triggers[i].parm1);
					MapDrawString(map_image, map.triggers[i].x + 0.5f, map.triggers[i].y + 0.33f, filename, true, 1.0f, 1.0f, 1.0f);
					sprintf(filename, "%02i", map.triggers[i].parm3);
					MapDrawString(map_image, map.triggers[i].x + 0.5f, map.triggers[i].y + 0.67f, filename, true, 1.0f, 1.0f, 1.0f);
				}
				break;
			case TRIGGER_SPEECHMARK:
			case TRIGGER_IMAGEMARK:
				MapDrawPic(map_image, map.triggers[i].x, map.triggers[i].y, CachePic("gam.tim"), 0, false, true, 1.0f, 1.0f, 1.0f);
				if (with_triggers)
				{
					if (map.triggers[i].parm1 != 0xFFFE)
					{
						sprintf(filename, "%03i", map.triggers[i].parm1);
						MapDrawString(map_image, map.triggers[i].x + 0.5f, map.triggers[i].y + 0.33f, filename, true, 1.0f, 1.0f, 1.0f);
					}
					if (map.triggers[i].parm2 != 0xFFFF)
					{
						sprintf(filename, "%i", map.triggers[i].parm2);
						MapDrawString(map_image, map.triggers[i].x + 0.5f, map.triggers[i].y + 0.67f, filename, true, 1.0f, 1.0f, 1.0f);
					}
				}
				break;
			case TRIGGER_TELEPORT:
				if (with_triggers)
				{
					if (map.triggers[i].parm1 == map_num && map.triggers[i].parm3 == map_section)
					{
						// draw the point to destination
						MapDrawDashedLine(map_image, map.triggers[i].x + 0.5f, map.triggers[i].y + 0.5f,  map.triggers[i].srcx + 0.5f, map.triggers[i].srcy + 0.5f, 128, 128, 128);
						MapDrawBorder(map_image, map.triggers[i].srcx, map.triggers[i].srcy, 96, 96, 96, 3, 1);
						MapDrawBorder(map_image, map.triggers[i].srcx, map.triggers[i].srcy, 96, 96, 96, 7, 1);
						MapDrawBorder(map_image, map.triggers[i].srcx, map.triggers[i].srcy, 96, 96, 96, 10, 1);
						MapDrawBorder(map_image, map.triggers[i].srcx, map.triggers[i].srcy, 96, 96, 96, 12, 1);
					}
					else
					{
						// draw the exit point (as it teleporting to another level)
						MapDrawBorder(map_image, map.triggers[i].x, map.triggers[i].y, 64, 0, 0, 1, 2);
						MapDrawBorder(map_image, map.triggers[i].x, map.triggers[i].y, 64, 0, 0, 8, 1);
						sprintf(filename, "%03i", map.triggers[i].parm1);
						MapDrawString(map_image, map.triggers[i].x + 0.5f, map.triggers[i].y + 0.33f, filename, true, 1.0f, 1.0f, 1.0f);
						sprintf(filename, "%02i", map.triggers[i].parm3);
						MapDrawString(map_image, map.triggers[i].x + 0.5f, map.triggers[i].y + 0.67f, filename, true, 1.0f, 1.0f, 1.0f);
					}
				}
				break;
			default:
				Print("  unknown trigger type: %i\n", map.triggers[i].type);
				if (with_triggers)
				{
					MapDrawBorder(map_image, map.triggers[i].x, map.triggers[i].y, 128, 0, 128, 2, 2);
					MapDrawDashedLine(map_image, map.triggers[i].x + 0.5f, map.triggers[i].y + 0.5f,  map.triggers[i].srcx + 0.5f, map.triggers[i].srcy + 0.5f, 64, 64, 32);
				}
				break;
		}
	}
	if (with_triggers)
	{
		// draw trigger zones
		for (i = 0; i < 80; i++)
		{
			for (j = 0; j < 80; j++)
			{
				if (map.triggertiles[i][j] == 0xFF)
					continue;
				MapDrawBorder(map_image, i, j, 128, 64, 64, 4, 1);
			}
		}

		// draw buttons
		for (i = 0; i < 20; i++)
		{
			if (!map.buttons[i].savenum)
				continue;
			for (j = 0; j < 256; j++)
				if (map.triggers[j].type == TRIGGER_TOUCH && map.triggers[j].parm2 == i)
					break;
			if (j >= 256)
				Print("Failed to find a script for button %i (triggergroup %i)\n", i, map.buttons[i].target);
			else
				MapDrawBorder(map_image, map.triggers[j].x, map.triggers[j].y, 128, 128, 64, 8, 2);
		}

		// draw lines to items spawned by scenery destroy
		for (i = 0; i < 256; i++)
		{
			if (!map.scenery[i].active)
				continue;
			for (j = 0; j < 3; j++)
				if (map.scenery[i].spawnitems[j] != 0xFF && map.scenery[i].spawnitems[j] < 50)
					MapDrawDashedLine(map_image, map.scenery[i].x + 0.5f, map.scenery[i].y + 0.5f,  map.items[map.scenery[i].spawnitems[j]].x + 0.5f, map.items[map.scenery[i].spawnitems[j]].y + 0.5f, 128, 128, 64);
		}

		// trigger lines for monsters
		for (i = 0; i < 32; i++)
		{
			if (map.monsters[i].u1[12] == 0xFF || map.monsters[i].target == 0xFFFF)
				continue;
			Print("  triggergroup %i (monster)\n", map.monsters[i].target);
			Draw_TriggerLine(&map, map_image, map.monsters[i].x, map.monsters[i].y, map.monsters[i].target, toggled_objects);
		}

		// trigger lines for items
		for (i = 0; i < 50; i++)
		{
			if (map.items[i].savenum == 0xFFFF || map.items[i].target == 0xFFFF)
				continue;
			Print("  triggergroup %i (item)\n", map.items[i].target);
			Draw_TriggerLine(&map, map_image, map.items[i].x, map.items[i].y, map.items[i].target, toggled_objects);
		}

		// trigger lines for buttons
		for (i = 0; i < 255; i++)
		{
			if (map.triggers[i].parm1 == 0xFFFF || map.triggers[i].type != TRIGGER_TOUCH)
				continue;
			if (map.buttons[map.triggers[i].parm2].target == 0xFFFF)
				continue;
			Print("  triggergroup %i (button)\n", map.buttons[map.triggers[i].parm2].target);
			Draw_TriggerLine(&map, map_image, map.triggers[i].x, map.triggers[i].y, map.buttons[map.triggers[i].parm2].target, toggled_objects);
		}

		// notrigger borders for animated tiles (one that activated from another level or code)
		for (i = 0; i < 100; i++)
		{
			if (map.atiles[i].targetnum == 0xFFFF)
				continue;
			Draw_NoTrigger(&map, map_image, map.atiles[i].x, map.atiles[i].y, map.atiles[i].targetnum);
		}

		// paths and speech numbers for monsters
		for (i = 0; i < 32; i++)
		{
			if (map.monsters[i].u1[12] == 0xFF)
				continue;

			// speech num
			if (map.monsters[i].speechnum != 0xFFFF)
			{
				sprintf(filename, "%03i", map.monsters[i].speechnum);
				MapDrawString(map_image, map.monsters[i].x + 0.5f, map.monsters[i].y + 0.5f, filename, true, 1.0f, 1.0f, 1.0f);
			}

			// path (buggy)
			/*
			sprintf(filename, "char%04i.sha", map.monsters[i].charnum);
			pic = CachePic(filename);
			if (map.monsters[i].lastpath)
			{
				Draw_Path(map_image, map.monsters[i].x, map.monsters[i].y, map.monsters[i].paths[0].x, map.monsters[i].paths[0].y, pic->r, pic->g, pic->b, false);
				for(j = 0; j < map.monsters[i].lastpath; j++)
					Draw_Path(map_image, map.monsters[i].paths[j].x, map.monsters[i].paths[j].y, map.monsters[i].paths[j + 1].x, map.monsters[i].paths[j + 1].y, pic->r, pic->g, pic->b, true);
				Draw_Path(map_image, map.monsters[i].paths[j].x, map.monsters[i].paths[j].y, map.monsters[i].paths[0].x, map.monsters[i].paths[0].y, pic->r, pic->g, pic->b, true);
			}
			*/
		}

		// print map info
		f = max(0.5f, map_minrow - 1.5f);
		ExtractFileBase(mapfile, filename);
		MapDrawString(map_image, max(0.5f, map_mincol - 1.5f), f, filename, false, 1.0f, 1.0f, 1.0f);
		f += 0.35f;
		sprintf(filename, "MAP %03i SECTION %02i", map_num, map_section);
		MapDrawString(map_image, max(0.5f, map_mincol - 1.5f), f, filename, false, 1.0f, 1.0f, 1.0f);
		if (toggled_objects)
		{
			f += 0.25f;
			MapDrawString(map_image, max(0.5f, map_mincol - 1.5f), f, "toggleable objects using alternative state", false, 1.0f, 1.0f, 1.0f);
		}
		if (with_triggers)
		{
			f += 0.25f;
			MapDrawString(map_image, max(0.5f, map_mincol - 1.5f), f, "triggers and misc info are shown", false, 1.0f, 1.0f, 1.0f);
		}
		if (with_lighting)
		{
			f += 0.25f;
			MapDrawString(map_image, max(0.5f, map_mincol - 1.5f), f, "lighting are shown", false, 1.0f, 1.0f, 1.0f);
		}
		if (show_save_id)
		{
			f += 0.25f;
			MapDrawString(map_image, max(0.5f, map_mincol - 1.5f), f, "savegame identifiers are shown", false, 1.0f, 1.0f, 1.0f);
		}
		if (with_solid)
		{
			f += 0.25f;
			MapDrawString(map_image, max(0.5f, map_mincol - 1.5f), f, "content zones are shown", false, 1.0f, 1.0f, 1.0f);
		}
		if (with_triggers)
		{
			f += 0.45f;
			MapDrawString(map_image, max(0.5f, map_mincol - 1.5f), f, "MAP INFO", false, 1.0f, 1.0f, 0.66f);
			f += 0.25f;
			sprintf(s, "%s, %s", (map.songnum >= 0 && map.songnum <= 10) ? songnames[map.songnum] : "?", (map.environments >= 0 && map.environments <= 1) ? environmentsnames[map.environments] : "?");
			MapDrawString(map_image, max(0.5f, map_mincol - 1.5f), f, s, false, 1.0f, 1.0f, 0.66f);
		}
	}

	// save identifiers
	Print("Assembling save identifiers...\n");
	if (show_save_id)
	{
		// for monsters
		for (i = 0; i < 32; i++)
		{
			if (map.monsters[i].u1[12] == 0xFF) // monster is empty?
				continue;
			strcpy(filename, "0");
			if (map.monsters[i].savenum != 0)
				sprintf(filename, "%03i", map.monsters[i].savenum);
			MapDrawString(map_image, map.monsters[i].x + 0.5f, map.monsters[i].y + 0.0f, filename, true, 0.7f, 0.9f, 1.5f);
		}
		// for items
		for (i = 0; i < 50; i++)
		{
			if (map.items[i].savenum == 0xFFFF) // item is empty?
				continue;
			strcpy(filename, "0");
			if (map.items[i].savenum != 0)
				sprintf(filename, "%03i", map.items[i].savenum);
			MapDrawString(map_image, map.items[i].x + 0.5f, map.items[i].y + 0.0f, filename, true, 0.7f, 0.9f, 1.5f);
		}
		// for buttons
		for (i = 0; i < 20; i++)
		{
			if (!map.buttons[i].savenum)
				continue;
			for (j = 0; j < 256; j++)
				if (map.triggers[j].type == TRIGGER_TOUCH && map.triggers[j].parm2 == i)
					break;
			if (j >= 256)
				continue;
			sprintf(filename, "%03i", map.buttons[i].savenum);
			MapDrawString(map_image, map.triggers[i].x + 0.5f, map.triggers[i].y + 0.0f, filename, true, 0.7f, 0.9f, 1.5f);
		}
		// for scenery
		for (i = 0; i < 256; i++)
		{
			if (!map.scenery[i].active)
				continue;
			tilepix = (toggled_objects ? (map.scenery[i].toggled ? map.scenery[i].tile2 : (map.scenery[i].destructible ? map.scenery[i].tile3 : map.scenery[i].tile1)) : map.scenery[i].tile1);
			if (tilepix != 0xFFFF && !(tilepix & TILEFLAG_NODRAW))
			{
				strcpy(filename, "0");
				if (map.scenery[i].savenum != 0)
					sprintf(filename, "%03i", map.scenery[i].savenum);
				MapDrawString(map_image, map.scenery[i].x + 0.5f, map.scenery[i].y + 0.0f, filename, true, 0.7f, 0.9f, 1.5f);
			}
		}
	}

	// test code
	//DeveloperData("monsters: ", (byte *)&map.monsters, 164, 32, 0, 0, developer);
	/*
	for (i = 0; i < 32; i++)
	{
		if (map.monsters[i].u1[12] == 0xFF)
			continue;
		// speech num
		printf("speech av%i\n", map.monsters[i].speechnum);
		// test
		picname = (byte *)(&map.triggers[i]);
		sprintf(filename, "%i", 0);
		MapDrawString(map_image, map.monsters[i].x + 0.5f, map.monsters[i].y + 0.5f, filename, true, 1.0f, 1.0f, 1.0f);
	}
	*/

	// apply crop bounds
	// write big map
	map_mincol = 0 - max(0, map_mincol - 2) * 32;
	map_minrow = 0 - max(0, map_minrow - 2) * 32;
	map_maxcol = (min(80, map_maxcol + 2) - 80) * 32;
	map_maxrow = (min(80, map_maxrow + 2) - 80) * 32;
	if (group_sections_by_path)
	{
		ExtractFilePath(outfile, path);
		sprintf(filename, "%smap%04i/sect%02i.tga", path, map_num, map_section);
	}
	else
	{
		strncpy(filename, outfile, MAX_OSPATH);
		DefaultExtension(filename, ".tga", MAX_OSPATH);
	}
	Print("Writing map %s...\n", filename);
	RawTGA(filename, 80 * 32, 80 * 32, map_mincol, map_minrow, map_maxcol, map_maxrow, NULL, map_image->data, 24, NULL);
	FreeMapDrawImage(map_image);

	// write layers
	/*
	Print("Writing decompressed level...\n", filename);
	sprintf(filename, "%s.map", outfile);
	SaveFile(filename, &map, sizeof(bo_map_t));
	DeveloperData("tilemaps", (byte *)&map.tilemaps, 2, 40, 0, 0, developer);
	DeveloperData("u1", (byte *)&map.u1, 12, 1, 0, 0, developer);
	DeveloperData("objects", (byte *)&map.objects, sizeof(bo_object_t), 10, 0, 0, developer);
	DeveloperData("monsters", (byte *)&map.monsters, sizeof(bo_monster_t), 32, 0, 0, developer);
	DeveloperData("grpobjects", (byte *)&map.grpobjects, sizeof(bo_grpobject_t), 8 * 32, 0, 0, developer);
	DeveloperData("atiles", (byte *)&map.atiles, sizeof(bo_atile_t), 100, 0, 0, developer);
	DeveloperData("buttons", (byte *)&map.buttons, sizeof(bo_button_t), 20, 0, 0, developer);
	DeveloperData("u2", (byte *)&map.u2, 8, 1, 0, 0, developer);
	DeveloperData("scenery", (byte *)&map.scenery, sizeof(bo_scenery_t), 256, 0, 0, developer);
	DeveloperData("backtiles", (byte *)&map.backtiles, sizeof(unsigned short) * 80, 80, 0, 0, developer);
	DeveloperData("foretiles", (byte *)&map.foretiles, sizeof(unsigned short) * 80, 80, 0, 0, developer);
	DeveloperData("contents", (byte *)&map.contents, sizeof(byte) * 80, 80, 0, 0, developer);
	DeveloperData("triggers", (byte *)&map.triggers, sizeof(bo_trigger_t), 255, 0, 0, developer);
	DeveloperData("triggertiles", (byte *)&map.triggertiles, sizeof(byte) * 80, 80, 0, 0, developer);
	DeveloperData("uniqueitems", (byte *)&map.uniqueitems, 4, 1, 0, 0, developer);
	DeveloperData("items", (byte *)&map.items, sizeof(bo_item_t), 50, 0, 0, developer);
	DeveloperData("u4", (byte *)&map.u4, 8, 1, 0, 0, developer);
	DeveloperData("u5", (byte *)&map.u5, 8, 40, 0, 0, developer);
	DeveloperData("u6", (byte *)&map.u6, 24, 1, 0, 0, developer);
	DeveloperData("sprites", (byte *)&map.sprites, sizeof(unsigned short), 8, 0, 0, developer);
	DeveloperData("effects", (byte *)&map.effects, sizeof(bo_effect_t), 64, 0, 0, developer);
	DeveloperData("u7", (byte *)&map.u7, 8, 117, 0, 0, developer);
	*/

	return 1;
}

/*
==========================================================================================

 MAP EXPORT FOR BLOOD OMNICIDE

==========================================================================================
*/

// regions buider structure
// VorteX: regions are groups (like 4 tiles being solid), used as optimization for physics engine
typedef struct
{
	int x;
	int y;
	int sizex;
	int sizey;
	int maxx;
	int maxy;
	int s;
} region_t;

region_t testregion;
byte     testregioncases[80][80];
int      numtestregions;
byte     testcontents[80][80];

void TestRegions_Recursive(int sizex, int sizey, int contents)
{
	int i, k;

	if (testcontents[testregion.x][testregion.y] != contents)
		return;
	if (testregioncases[sizex][sizey])
		return;
	testregioncases[sizex][sizey] = 1;

	// test region with row increased
	k = testregion.x + sizex;
	if (k < testregion.maxx)
	{
		for (i = 0; i < sizey; i++)
			if (testcontents[k][testregion.y + i] != contents)
				break;
		if (i >= sizey)
		{
			// can build region with such parms
			if ((sizex + 1)*sizey >= testregion.s)
			{
				testregion.sizex = sizex + 1;
				testregion.sizey = sizey;
				testregion.s = testregion.sizex * testregion.sizey;
			}
			TestRegions_Recursive(sizex + 1, sizey, contents);
		}
	}

	// test region with col increased
	k = testregion.y + sizey;
	if (k < testregion.maxy)
	{
		for (i = 0; i < sizex; i++)
			if (testcontents[testregion.x + i][k] != contents)
				break;
		if (i >= sizex)
		{
			// can build region with such parms
			if (sizex*(sizey + 1) >= testregion.s)
			{
				testregion.sizex = sizex;
				testregion.sizey = sizey + 1;
				testregion.s = testregion.sizex * testregion.sizey;
			}
			TestRegions_Recursive(sizex, sizey + 1, contents);
		}
	}
}

int TestRegions(int startx, int starty, int maxx, int maxy, int contents)
{
	numtestregions = 0;
	memset(testregioncases, 0, sizeof(testregioncases));
	testregion.x = startx;
	testregion.y = starty;
	testregion.sizex = 1;
	testregion.sizey = 1;
	testregion.maxx = maxx;
	testregion.maxy = maxy;
	testregion.s = 1;
	TestRegions_Recursive(1, 1, contents);
	return testregion.s;
}

static int BuildRegions(bo_map_t *map, int map_mini, int map_minj, int map_maxi, int map_maxj, FILE *out, region_t *outregions, int maxregions, int contents, int debugwrite)
{
	int i, j;
	region_t maxreg;
	int haveregions[80][80];
	int numcollisionregions;

	memcpy(testcontents, map->contents, sizeof(testcontents));

	// eliminate tiles that surrounded by same contents from 4 sides
	/*
	for (i = 1; i < 79; i++)
		for (j = 1; j < 79; j++)
			if (testcontents[i][j] == contents)
				if (testcontents[i-1][j] == contents || testcontents[i-1][j] == 255)
					if (testcontents[i+1][j] == contents || testcontents[i+1][j] == 255)
						if (testcontents[i][j-1] == contents || testcontents[i][j-1] == 255)
							if (testcontents[i][j+1] == contents || testcontents[i][j+1] == 255)
								testcontents[i][j] = 255;
	*/

	// find out regions
	numcollisionregions = 0;
	do
	{
		maxreg.x = 0;
		maxreg.y = 0;
		maxreg.sizex = 0;
		maxreg.sizey = 0;
		maxreg.s = 0;
		for (i = map_mini; i < map_maxi; i++)
		{
			for (j = map_minj; j < map_maxj; j++)
			{
				haveregions[i][j] = TestRegions(i, j, map_maxi, map_maxj, contents);
				if (haveregions[i][j] > maxreg.s)
					memcpy(&maxreg, &testregion, sizeof(maxreg));
			}
		}
		if (maxreg.s < 2) // finished
			break;

		// add collision region
		memcpy(&outregions[numcollisionregions], &maxreg, sizeof(maxreg));
		for (i = 0; i < maxreg.sizex; i++)
			for (j = 0; j < maxreg.sizey; j++)
				testcontents[maxreg.x + i][maxreg.y + j] = 255;
		numcollisionregions++;
	}
	while(numcollisionregions < maxregions);

	// write a one-point regions
	for (i = map_mini; i < map_maxi; i++)
	{
		for (j = map_minj; j < map_maxj; j++)
		{
			if (testcontents[i][j] == contents)
			{
				if (numcollisionregions >= maxregions)
					break;
				outregions[numcollisionregions].x = i;
				outregions[numcollisionregions].y = j;
				outregions[numcollisionregions].sizex = 1;
				outregions[numcollisionregions].sizey = 1;
				outregions[numcollisionregions].s = 1;
				numcollisionregions++;
			}
		}
	}

	// debug write
	if (debugwrite)
	{
		byte color[80][80][3], c[3];
		char filename[128];
		int k;

		for (k = 0; k < numcollisionregions; k++)
		{
			c[0] = rand() % 255 + 64;
			c[1] = rand() % 255 + 64;
			c[2] = rand() % 255 + 64;

			for (i = 0; i < outregions[k].sizex; i++)
			{
				for (j = 0; j < outregions[k].sizey; j++)
				{
					color[outregions[k].x + i][outregions[k].y + j][0] = c[0];
					color[outregions[k].x + i][outregions[k].y + j][1] = c[1];
					color[outregions[k].x + i][outregions[k].y + j][2] = c[2];
				}
			}
		}
		sprintf(filename, "regions_%i_%i.tga", contents, debugwrite);
		RawTGA(filename, 80, 80, 0, 0, 0, 0, NULL, (byte *)color, 24, NULL);
	}

	return numcollisionregions;
}

static void BuildMapRegion(bo_map_t *map, int map_mini, int map_minj, int map_maxi, int map_maxj, FILE *out, region_t *collisionregions, int contents, char *contentsName, int debug)
{
	int i, numregions;

	numregions = BuildRegions(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, 6400, contents, debug);
	if (numregions)
	{
		Print("%i regions for %s\n", numregions, contentsName);
		fprintf(out, "%s [\n", contentsName);
		for (i = 0; i < numregions; i++)
			fprintf(out, " ( %i %i %i %i )\n", collisionregions[i].x, collisionregions[i].y, collisionregions[i].sizex, collisionregions[i].sizey);
		fprintf(out, "]\n");
	}
}

static void BuildMapRegions(bo_map_t *map, int map_mini, int map_minj, int map_maxi, int map_maxj, FILE *out)
{
	region_t collisionregions[6400];
	
	fprintf(out, "regions {\n");

	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, 0, "empty", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_SOLID, "solid", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_WATER, "water", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_LAVA, "lava", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_SWAMP, "swamp", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_ICE, "ice", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_FIRE, "fire", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_SPIKES, "spikes", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_TRAPTELEPORT, "teleport", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_JUMPWALL, "jumpwall", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_JUMPFENCE, "jumpfence", 0);
	BuildMapRegion(map, map_mini, map_minj, map_maxi, map_maxj, out, collisionregions, CONTENTS_MISTWALK, "mistwalk", 0);

	fprintf(out, "}\n");
}

// Atlas texture (not used)
// VorteX: atlas texture are rendered full map texture (only static objects) used to speed up drawing
// unfortunately, atlases for Blood Omen maps are too large to be used
void BuildAtlasTexture(char *outfile, bo_map_t *map, bigfileheader_t *bigfileheader, FILE *bigfile, char *tilespath, int map_mini, int map_minj, int map_maxi, int map_maxj, bool top)
{
	int i, j;
	unsigned short tilepix, tilegroup;
	char filename[MAX_OSPATH];
	map_renderbuffer_t *map_image;
	cachepic_t *pic;

	CachePic_Globals(tilespath, bigfileheader, bigfile)
	map_image = AllocateMapDrawImage(3);

	for (i = map_mini; i < map_maxi; i++)
	{
		for (j = map_minj; j < map_maxj; j++)
		{
			// foreground
			if (!top)
			{
				// tile1
				tilepix = map->backtiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & (TILEFLAG_NODRAW + TILEFLAG_ALWAYSONTOP)))
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					sprintf(filename, "grp%05i.ctm", map->tilemaps[tilegroup]);
					pic = CachePic(filename);
					if (pic)
					{
						pic->subpics = &subpics_grp; // always use standart tilemap 
						MapDrawPic(map_image, (float)i, (float)j, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), true, false, 1.0f, 1.0f, 1.0f);
					}
				}
				// tile2
				tilepix = map->foretiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & (TILEFLAG_NODRAW + TILEFLAG_ALWAYSONTOP)))
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					sprintf(filename, "grp%05i.ctm", map->tilemaps[tilegroup]);
					pic = CachePic(filename);
					if (pic)
					{
						pic->subpics = &subpics_grp; // always use standart tilemap 
						MapDrawPic(map_image, (float)i, (float)j, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), false, false, 1.0f, 1.0f, 1.0f);
					}
				}
			}
			if (top)
			{
				// tile1
				tilepix = map->backtiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & TILEFLAG_NODRAW) && (tilepix & TILEFLAG_ALWAYSONTOP))
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					sprintf(filename, "grp%05i.ctm", map->tilemaps[tilegroup]);
					pic = CachePic(filename);
					if (pic)
					{
						pic->subpics = &subpics_grp; // always use standart tilemap 
						MapDrawPic(map_image, (float)i, (float)j, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), true, false, 1.0f, 1.0f, 1.0f);
					}
				}
				// tile2
				tilepix = map->foretiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & TILEFLAG_NODRAW) && (tilepix & TILEFLAG_ALWAYSONTOP))
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					sprintf(filename, "grp%05i.ctm", map->tilemaps[tilegroup]);
					pic = CachePic(filename);
					if (pic)
					{
						pic->subpics = &subpics_grp; // always use standart tilemap 
						MapDrawPic(map_image, (float)i, (float)j, pic, (tilepix & TILEFLAG_IMASK) - (tilegroup * 64), false, false, 1.0f, 1.0f, 1.0f);
					}
				}
			}
		}
	}
	// write
	map_mini = 0 - max(0, map_mini) * 32;
	map_minj = 0 - max(0, map_minj) * 32;
	map_maxi = (min(80, map_maxi) - 80) * 32;
	map_maxj = (min(80, map_maxj) - 80) * 32;
	RawTGA(outfile, 80 * 32, 80 * 32, map_mini, map_minj, map_maxi, map_maxj, NULL, map_image->data, 24, NULL);
	FreeMapDrawImage(map_image);
}

// Draw Meshes
// like atlas texture, but for draw geometry. This contains all static tiles grouped into single model.
// VorteX: to get rid of transparent sorting math, all static tiles are drawn with alphaTest
//         so static tiles with shadows should be moved to dynamic render path as well
bool TileMapIsUsed(bo_map_t *map, int i, int map_mini, int map_minj, int map_maxi, int map_maxj, bool noanimated)
{
	int j, k;

	for (j = map_mini; j < map_maxi; j++)
		for (k = map_minj; k < map_maxj; k++)
			if ((map->backtiles[j][k] != 0xFFFF && ((map->backtiles[j][k] & TILEFLAG_IMASK) / 64) == i) || (map->foretiles[j][k] != 0xFFFF && ((map->foretiles[j][k] & TILEFLAG_IMASK) / 64) == i))
				return true;
	if (noanimated)
		return false;
	for (j = 0; j < 100; j++)
		if (map->atiles[j].targetnum != 0xFFFF && ((map->atiles[j].tile1 != 0xFFFF && ((map->atiles[j].tile1 & TILEFLAG_IMASK) / 64) == i) || (map->atiles[j].tile2 != 0xFFFF && ((map->atiles[j].tile2 & TILEFLAG_IMASK) / 64) == i)))
			return true;
	for (j = 0; j < 20; j++)
		if (map->buttons[j].savenum && map->buttons[j].savenum != 0xFFFF && ((map->buttons[j].tile1 != 0xFFFF && ((map->buttons[j].tile1 & TILEFLAG_IMASK) / 64) == i) || (map->buttons[j].tile2 != 0xFFFF && ((map->buttons[j].tile2 & TILEFLAG_IMASK) / 64) == i)))
			return true;
	for (j = 0; j < 256; j++)
		if (map->scenery[j].active && ((map->scenery[j].tile1 != 0xFFFF && ((map->scenery[j].tile1 & TILEFLAG_IMASK) / 64) == i) || (map->scenery[j].tile2 != 0xFFFF && ((map->scenery[j].tile2 & TILEFLAG_IMASK) / 64) == i) || (map->scenery[j].tile3 != 0xFFFF && ((map->scenery[j].tile3 & TILEFLAG_IMASK) / 64) == i)))
			return true;
	return false;
}

const int z1_1 = 0, z1_2 = 1, z2_1 = 2, z2_2 = 3;
const int zh[4] = {0, 1, 8, 9};

void PrintVert(FILE *obj, int x, int y, int z, int obj_orientation)
{
	if (obj_orientation)
		fprintf(obj, "v %i %i %i\n", x, zh[z], y);
	else
		fprintf(obj, "v %i %i %i\n", x, y, zh[z]);
}

void BuildStaticGeometry(char *outfile, bo_map_t *map, char *tilematerialspath, int map_mini, int map_minj, int map_maxi, int map_maxj, int map_maxtile, float tilemapsize, bool *tilemaps_exclude)
{
	int i, j, k, tilepix, tilenum, tilegroup, idx, tileflags, z, obj_orientation;
	unsigned int num_verts = 0, num_tc = 0, num_triangles = 0;
	float tc_epsilon1, tc_epsilon2;
	int vx_indexes[81][81][4][4], tc_indexes[64];
	char filename[MAX_OSPATH];
	float tc[2];
	FILE *obj;

	memset(vx_indexes, 0, sizeof(vx_indexes));
	memset(tc_indexes, 0, sizeof(tc_indexes));

	// export materials
	sprintf(filename, "%s.mtl", outfile);
	obj = SafeOpenWrite(filename);
	fprintf(obj, "# Wavefront material file\n");
	fprintf(obj, "# Generated by Blood Pill\n");
	for (k = 0; k < map_maxtile; k++)
	{
		if (!TileMapIsUsed(map, k, map_mini, map_minj, map_maxi, map_maxj, true))
			continue;
		fprintf(obj, "newmtl %sgrp%05i\n", tilematerialspath, map->tilemaps[k]);
		fprintf(obj, "map_Kd %sgrp%05i.tga\n", tilematerialspath, map->tilemaps[k]);
		fprintf(obj, "\n");
	}
	WriteClose(obj);

	// export geometry, sort by tilemap
	sprintf(filename, "%s.obj", outfile);
	obj = SafeOpenWrite(filename);
	fprintf(obj, "# Wavefront object file\n");
	fprintf(obj, "# Generated by Blood Pill\n");
	ExtractFileBase(outfile, filename);
	fprintf(obj, "mtllib %s.mtl\n", filename);
	fprintf(obj, "o map\n");
	tileflags = TILEFLAG_UNKNOWN1+TILEFLAG_NODRAW+TILEFLAG_ONTOPINFRONT+TILEFLAG_UNKNOWN6;
	obj_orientation = 1;
	idx = 1;
	for (k = 0; k < map_maxtile; k++)
	{
		if (!TileMapIsUsed(map, k, map_mini, map_minj, map_maxi, map_maxj, true))
			continue;
		// export vertexes
		for (i = 0; i < 80; i++)
		{
			for (j = 0; j < 80; j++)
			{
				tilepix = map->backtiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & tileflags) && !tilemaps_exclude[tilepix & TILEFLAG_IMASK])
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					if (tilegroup == k)
					{
						z = ((tilepix & TILEFLAG_ALWAYSONTOP) != 0) ? z2_1 : z1_1;
						if (!vx_indexes[i][j][z][0])
						{
							if (i > 0 && vx_indexes[i-1][j][z][1])
								vx_indexes[i][j][z][0] = vx_indexes[i-1][j][z][1];
							else if (j > 0 && vx_indexes[i][j-1][z][3])
								vx_indexes[i][j][z][0] = vx_indexes[i][j-1][z][3];
							else if (i > 0 && j > 0 && vx_indexes[i-1][j-1][z][2])
								vx_indexes[i][j][z][0] = vx_indexes[i-1][j-1][z][2];
							else
							{
								PrintVert(obj, i, -j, z, obj_orientation);
								vx_indexes[i][j][z][0] = idx;
								idx += 1;
							}
						}
						if (!vx_indexes[i][j][z][1])
						{
							if (i < 79 && vx_indexes[i+1][j][z][0])
								vx_indexes[i][j][z][1] = vx_indexes[i+1][j][z][0];
							else if (j > 0 && vx_indexes[i][j-1][z][2])
								vx_indexes[i][j][z][1] = vx_indexes[i][j-1][z][2];
							else if (i < 79 && j > 0 && vx_indexes[i+1][j-1][z][3])
								vx_indexes[i][j][z][1] = vx_indexes[i+1][j-1][z][3];
							else
							{
								PrintVert(obj, i + 1, -j, z, obj_orientation);
								vx_indexes[i][j][z][1] = idx;
								idx += 1;
							}
						}
						if (!vx_indexes[i][j][z][2])
						{
							if (i < 79 && vx_indexes[i+1][j][z][3])
								vx_indexes[i][j][z][2] = vx_indexes[i+1][j][z][3];
							else if (j < 79 && vx_indexes[i][j+1][z][1])
								vx_indexes[i][j][z][2] = vx_indexes[i][j+1][z][1];
							else if (i < 79 && j < 79 && vx_indexes[i+1][j+1][z][0])
								vx_indexes[i][j][z][2] = vx_indexes[i+1][j+1][z][0];
							else
							{
								PrintVert(obj, i + 1, -j-1, z, obj_orientation);
								vx_indexes[i][j][z][2] = idx;
								idx += 1;
							}
						}
						if (!vx_indexes[i][j][z][3])
						{
							if (i > 0 && vx_indexes[i-1][j][z][2])
								vx_indexes[i][j][z][3] = vx_indexes[i-1][j][z][2];
							else if (j < 79 && vx_indexes[i][j+1][z][0])
								vx_indexes[i][j][z][3] = vx_indexes[i][j+1][z][0];
							else if (i > 0 && j < 79 && vx_indexes[i-1][j+1][z][1])
								vx_indexes[i][j][z][3] = vx_indexes[i-1][j+1][z][1];
							else
							{
								PrintVert(obj, i, -j-1, z, obj_orientation);
								vx_indexes[i][j][z][3] = idx;
								idx += 1;
							}
						}
					}
				}
				tilepix = map->foretiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & tileflags) && !tilemaps_exclude[tilepix & TILEFLAG_IMASK])
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					if (tilegroup == k)
					{
						z = ((tilepix & TILEFLAG_ALWAYSBELOW) != 0) ? z1_2 : z2_2;
						if (!vx_indexes[i][j][z][0])
						{
							if (i > 0 && vx_indexes[i-1][j][z][1])
								vx_indexes[i][j][z][0] = vx_indexes[i-1][j][z][1];
							else if (j > 0 && vx_indexes[i][j-1][z][3])
								vx_indexes[i][j][z][0] = vx_indexes[i][j-1][z][3];
							else if (i > 0 && j > 0 && vx_indexes[i-1][j-1][z][2])
								vx_indexes[i][j][z][0] = vx_indexes[i-1][j-1][z][2];
							else
							{
								PrintVert(obj, i, -j, z, obj_orientation);
								vx_indexes[i][j][z][0] = idx;
								idx += 1;
							}
						}
						if (!vx_indexes[i][j][z][1])
						{
							if (i < 79 && vx_indexes[i+1][j][z][0])
								vx_indexes[i][j][z][1] = vx_indexes[i+1][j][z][0];
							else if (j > 0 && vx_indexes[i][j-1][z][2])
								vx_indexes[i][j][z][1] = vx_indexes[i][j-1][z][2];
							else if (i < 79 && j > 0 && vx_indexes[i+1][j-1][z][3])
								vx_indexes[i][j][z][1] = vx_indexes[i+1][j-1][z][3];
							else
							{
								PrintVert(obj, i + 1, -j, z, obj_orientation);
								vx_indexes[i][j][z][1] = idx;
								idx += 1;
							}
						}
						if (!vx_indexes[i][j][z][2])
						{
							if (i < 79 && vx_indexes[i+1][j][z][3])
								vx_indexes[i][j][z][2] = vx_indexes[i+1][j][z][3];
							else if (j < 79 && vx_indexes[i][j+1][z][1])
								vx_indexes[i][j][z][2] = vx_indexes[i][j+1][z][1];
							else if (i < 79 && j < 79 && vx_indexes[i+1][j+1][z][0])
								vx_indexes[i][j][z][2] = vx_indexes[i+1][j+1][z][0];
							else
							{
								PrintVert(obj, i + 1, -j-1, z, obj_orientation);
								vx_indexes[i][j][z][2] = idx;
								idx += 1;
							}
						}
						if (!vx_indexes[i][j][z][3])
						{
							if (i > 0 && vx_indexes[i-1][j][z][2])
								vx_indexes[i][j][z][3] = vx_indexes[i-1][j][z][2];
							else if (j < 79 && vx_indexes[i][j+1][z][0])
								vx_indexes[i][j][z][3] = vx_indexes[i][j+1][z][0];
							else if (i > 0 && j < 79 && vx_indexes[i-1][j+1][z][1])
								vx_indexes[i][j][z][3] = vx_indexes[i-1][j+1][z][1];
							else
							{
								PrintVert(obj, i, -j-1, z, obj_orientation);
								vx_indexes[i][j][z][3] = idx;
								idx += 1;
							}
						}
					}
				}
			}
		}
	}
	num_verts = idx - 1;
	tc_epsilon1 = (1.0f/tilemapsize);
	tc_epsilon2 = (1.0f/8.0f - 2.0f/tilemapsize);
	idx = 1;
	for (k = 0; k < map_maxtile; k++)
	{
		if (!TileMapIsUsed(map, k, map_mini, map_minj, map_maxi, map_maxj, true))
			continue;
		// export texture coordinates
		for (i = 0; i < 80; i++)
		{
			for (j = 0; j < 80; j++)
			{
				tilepix = map->backtiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & tileflags) && !tilemaps_exclude[tilepix & TILEFLAG_IMASK])
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					if (tilegroup == k)
					{
						tilenum = (tilepix & TILEFLAG_IMASK) - tilegroup * 64;
						if (!tc_indexes[tilenum])
						{
							tc[0] = (float)(tilenum - (tilenum / 8) * 8) / 8.0f + tc_epsilon1;
							tc[1] = 0-(float)((int)(tilenum / 8))/8.0f - tc_epsilon1;
							fprintf(obj, "vt %f %f\n", tc[0], tc[1]);
							tc[0] += tc_epsilon2;
							fprintf(obj, "vt %f %f\n", tc[0], tc[1]);
							tc[1] -= tc_epsilon2;
							fprintf(obj, "vt %f %f\n", tc[0], tc[1]);
							tc[0] -= tc_epsilon2;
							fprintf(obj, "vt %f %f\n", tc[0], tc[1]);
							tc_indexes[tilenum] = idx;
							idx += 4;
						}
					}
				}
				tilepix = map->foretiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & tileflags) && !tilemaps_exclude[tilepix & TILEFLAG_IMASK])
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					if (tilegroup == k)
					{
						tilenum = (tilepix & TILEFLAG_IMASK) - tilegroup * 64;
						if (!tc_indexes[tilenum])
						{
							tc[0] = (float)(tilenum - (tilenum / 8) * 8) / 8.0f + tc_epsilon1;
							tc[1] = 0-(float)((int)(tilenum / 8))/8.0f - tc_epsilon1;
							fprintf(obj, "vt %f %f\n", tc[0], tc[1]);
							tc[0] += tc_epsilon2;
							fprintf(obj, "vt %f %f\n", tc[0], tc[1]);
							tc[1] -= tc_epsilon2;
							fprintf(obj, "vt %f %f\n", tc[0], tc[1]);
							tc[0] -= tc_epsilon2;
							fprintf(obj, "vt %f %f\n", tc[0], tc[1]);
							tc_indexes[tilenum] = idx;
							idx += 4;
						}
					}
				}
			}
		}
	}
	num_tc = idx - 1;
	num_triangles = 0;
	for (k = 0; k < map_maxtile; k++)
	{
		if (!TileMapIsUsed(map, k, map_mini, map_minj, map_maxi, map_maxj, true))
			continue;
		// export triangles
		fprintf(obj, "usemtl %sgrp%05i\n", tilematerialspath, map->tilemaps[k], tilemapsize);	
		for (i = 0; i < 80; i++)
		{
			for (j = 0; j < 80; j++)
			{
				tilepix = map->backtiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & tileflags) && !tilemaps_exclude[tilepix & TILEFLAG_IMASK])
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					if (tilegroup == k)
					{
						z = ((tilepix & TILEFLAG_ALWAYSONTOP) != 0) ? z2_1 : z1_1;
						tilenum = (tilepix & TILEFLAG_IMASK) - tilegroup * 64;
						idx = tc_indexes[tilenum];
						fprintf(obj, "f %i/%i %i/%i %i/%i\n", vx_indexes[i][j][z][0], idx,     vx_indexes[i][j][z][1], idx + 1, vx_indexes[i][j][z][2], idx + 2);  
						fprintf(obj, "f %i/%i %i/%i %i/%i\n", vx_indexes[i][j][z][2], idx + 2, vx_indexes[i][j][z][3], idx + 3, vx_indexes[i][j][z][0], idx);
						num_triangles += 2;
					}
				}
				tilepix = map->foretiles[i][j];
				if (tilepix != 0xFFFF && !(tilepix & tileflags) && !tilemaps_exclude[tilepix & TILEFLAG_IMASK])
				{
					tilegroup = (tilepix & TILEFLAG_IMASK) / 64;
					if (tilegroup == k)
					{
						z = ((tilepix & TILEFLAG_ALWAYSBELOW) != 0) ? z1_2 : z2_2;
						tilenum = (tilepix & TILEFLAG_IMASK) - tilegroup * 64;
						idx = tc_indexes[tilenum];
						fprintf(obj, "f %i/%i %i/%i %i/%i\n", vx_indexes[i][j][z][0], idx,     vx_indexes[i][j][z][1], idx + 1, vx_indexes[i][j][z][2], idx + 2);  
						fprintf(obj, "f %i/%i %i/%i %i/%i\n", vx_indexes[i][j][z][2], idx + 2, vx_indexes[i][j][z][3], idx + 3, vx_indexes[i][j][z][0], idx);
						num_triangles += 2;
					}
				}
			}
		}
	}
	fprintf(obj, "# model stats:\n");
	fprintf(obj, "# %i vertexes\n", num_verts);
	fprintf(obj, "# %i texture coordinates\n", num_tc);
	fprintf(obj, "# %i triangles\n", num_triangles);
	WriteClose(obj);
}

int MapExportTXT(char *mapfile, byte *fileData, int fileDataSize, char *outfile, bigfileheader_t *bigfileheader, FILE *bigfile, char *tilespath)
{
	char mapname[32], filename[MAX_OSPATH];
	unsigned short map_num, map_section, map_numtilemaps, map_maxtile;
	int i, j, k, row, col, decSize, map_mini, map_minj, map_maxi, map_maxj, tilepix;
	FILE *mapsv, *mapcl;
	cachepic_t *pic;
	bool tilemaps_withshadow[40*64];
	byte *dec, *end;
	bo_map_t map;

	CachePic_Globals(tilespath, bigfileheader, bigfile)
	memset(tilemaps_withshadow, 0, sizeof(tilemaps_withshadow));
	
	// extract map (get mapnum and section)
	ExtractFileName(mapfile, mapname);
	StripFileExtension(mapname, mapname);
	map_num = 0;
	map_section = 0;
	if (mapname[0] != 'm' || strlen(mapname) != 8)
		Print("warning: %s is not valid, map num and section is not known\n", mapname);
	else
	{
		// get map and section
		map_section = atoi((char *)(mapname + 6));
		mapname[6] = 0;
		map_num = atoi((char *)(mapname + 1));
	}

	// decompress
	dec = (byte *)LzDec(&decSize, fileData, 0, fileDataSize, true);
	if (dec == NULL)
		return 0;
	if (decSize != sizeof(bo_map_t))
		return 0;
	memcpy(&map, dec, sizeof(bo_map_t));

	// set export path
	sprintf(filename, "%s_sv.txt", outfile);
	Print("exporting server-side map to %s\n", filename);
	mapsv = SafeOpenWrite(filename);
	sprintf(filename, "%s_cl.txt", outfile);
	Print("exporting client-side map to %s\n", filename);
	mapcl = SafeOpenWrite(filename);

	// get map bounds
	map_mini = 80;
	map_minj = 80;
	map_maxi = 0;
	map_maxj = 0;
	for (i = 0; i < 80; i++)
	{
		for (j = 0; j < 80; j++)
		{
			if (map.backtiles[i][j] != 0xFFFF)
			{
				map_mini = min(i, map_mini);
				map_minj = min(j, map_minj);
				map_maxi = max(i, map_maxi);
				map_maxj = max(j, map_maxj);
			}
			if (map.foretiles[i][j] != 0xFFFF)
			{
				map_mini = min(i, map_mini);
				map_minj = min(j, map_minj);
				map_maxi = max(i, map_maxi);
				map_maxj = max(j, map_maxj);
			}
		}
	}
	map_mini = max(0, map_mini - 1);
	map_minj = max(0, map_minj - 1);
	map_maxi = min(80, map_maxi + 1);
	map_maxj = min(80, map_maxj + 1);
	Print("map bounds (%i %i) (%i %i)\n", map_mini, map_minj, map_maxi, map_maxj);

	// preload tilemaps
	map_maxtile = 0;
	map_numtilemaps = 0;
	for (i = 0; i < 40; i++)
	{
		if (!TileMapIsUsed(&map, i, map_mini, map_minj, map_maxi, map_maxj, false))
			continue;
		map_numtilemaps++;
		map_maxtile = max(map_maxtile, i+1);	
		// check if tile contains shadow pixels, set a flag for this case
		sprintf(filename, "grp%05i.ctm", map.tilemaps[i]);
		pic = CachePic(filename);
		if (!pic)
			continue;
		for (j = 0; j < 64; j++)
		{
			row = j/8;
			col = j - row*8;
			for (k = 0; k < 32; k++)
			{
				dec = pic->pixels + (row*32 + k)*256*3 + col*32*3;
				end = dec + 32*3;
				while(dec < end)
				{
					if (IsShadowPixel(dec))
						goto foundshadow;
					dec += 3;
				}		
			}
			continue;
foundshadow:
			tilemaps_withshadow[i*64+j] = true;
		}
		FreePic(pic);
	}
	Print("map using %i tilemaps (max index %i)\n", map_numtilemaps, map_maxtile);

	// generate static geometry to help renderer
	//sprintf(filename, "%s_256", outfile);
	//BuildStaticGeometry(filename, &map, "legacy/tiles256/", map_mini, map_minj, map_maxi, map_maxj, map_maxtile, 256, tilemaps_withshadow);
	//sprintf(filename, "%s_512", outfile);
	//BuildStaticGeometry(filename, &map, "legacy/tiles512/", map_mini, map_minj, map_maxi, map_maxj, map_maxtile, 512, tilemaps_withshadow);
	//sprintf(filename, "%s_1024", outfile);
	//BuildStaticGeometry(filename, &map, "legacy/tiles1024/", map_mini, map_minj, map_maxi, map_maxj, map_maxtile, 1024, tilemaps_withshadow);

	// headers
	fprintf(mapsv, "BloodOmenMap\n");
	fprintf(mapcl, "BloodOmenMap\n");
	fprintf(mapsv, "{\n");
	fprintf(mapcl, "{\n");
	fprintf(mapsv, "header {\n");
	fprintf(mapcl, "header {\n");
	fprintf(mapsv, " map %i\n", map_num);
	fprintf(mapcl, " map %i\n", map_num);
	fprintf(mapsv, " section %i\n", map_section);
	fprintf(mapcl, " section %i\n", map_section);
	fprintf(mapsv, " left %i\n", map_mini);
	fprintf(mapcl, " left %i\n", map_mini);
	fprintf(mapsv, " top %i\n", map_minj);
	fprintf(mapcl, " top %i\n", map_minj);
	fprintf(mapsv, " width %i\n", map_maxi);
	fprintf(mapcl, " width %i\n", map_maxi);
	fprintf(mapsv, " height %i\n", map_maxj);
	fprintf(mapcl, " height %i\n", map_maxj);
	fprintf(mapsv, "}\n");
	fprintf(mapcl, "}\n");

	// tilemaps
	// 2 passes to eliminate extra lines
	fprintf(mapcl, "tileSet [\n");
	for (i = 0; i < map_maxtile; i++)
	{
		// check if tilemap is actually used
		for (j = map_mini; j < map_maxi; j++)
			for (k = map_minj; k < map_maxj; k++)
				if ((map.backtiles[j][k] != 0xFFFF && ((map.backtiles[j][k] & TILEFLAG_IMASK) / 64) == i) || (map.foretiles[j][k] != 0xFFFF && ((map.foretiles[j][k] & TILEFLAG_IMASK) / 64) == i))
					goto tilefound2;
		for (j = 0; j < 100; j++)
			if (map.atiles[j].targetnum != 0xFFFF && ((map.atiles[j].tile1 != 0xFFFF && ((map.atiles[j].tile1 & TILEFLAG_IMASK) / 64) == i) || (map.atiles[j].tile2 != 0xFFFF && ((map.atiles[j].tile2 & TILEFLAG_IMASK) / 64) == i)))
				goto tilefound2;
		for (j = 0; j < 20; j++)
			if (map.buttons[j].savenum && map.buttons[j].savenum != 0xFFFF && ((map.buttons[j].tile1 != 0xFFFF && ((map.buttons[j].tile1 & TILEFLAG_IMASK) / 64) == i) || (map.buttons[j].tile2 != 0xFFFF && ((map.buttons[j].tile2 & TILEFLAG_IMASK) / 64) == i)))
				goto tilefound2;
		for (j = 0; j < 256; j++)
			if (map.scenery[j].active && ((map.scenery[j].tile1 != 0xFFFF && ((map.scenery[j].tile1 & TILEFLAG_IMASK) / 64) == i) || (map.scenery[j].tile2 != 0xFFFF && ((map.scenery[j].tile2 & TILEFLAG_IMASK) / 64) == i) || (map.scenery[j].tile3 != 0xFFFF && ((map.scenery[j].tile3 & TILEFLAG_IMASK) / 64) == i)))
				goto tilefound2;
		fprintf(mapcl, " -\n", map.tilemaps[i]);
		continue;
tilefound2:
		fprintf(mapcl, " \"grp%05i\"\n", map.tilemaps[i]);	
	}
	fprintf(mapcl, "]\n");

	// world
	fprintf(mapcl, "map [\n");
	for (i = map_mini; i < map_maxi; i++)
	{
		fprintf(mapcl, " // %i\n", i);
		for (j = map_minj; j < map_maxj; j++)
		{ 
			if (map.backtiles[i][j] == 0xFFFF)
				fprintf(mapcl, "- ");
			else
			{
				tilepix = map.backtiles[i][j];
				if (tilemaps_withshadow[tilepix & TILEFLAG_IMASK])
					fprintf(mapcl, "%i ", tilepix | 65536); // extra flag for shadow areas
				else
					fprintf(mapcl, "%i ", tilepix);
			}
			if (map.foretiles[i][j] == 0xFFFF)
				fprintf(mapcl, "- ");
			else
			{
				tilepix = map.foretiles[i][j];
				if (tilemaps_withshadow[tilepix & TILEFLAG_IMASK])
					fprintf(mapcl, "%i ", tilepix | 65536); // extra flag for shadow areas
				else
					fprintf(mapcl, "%i ", tilepix);
			}
			fprintf(mapcl, "\n");
		}
	}
	fprintf(mapcl, "]\n");

	// world collision
	BuildMapRegions(&map, map_mini, map_minj, map_maxi, map_maxj, mapsv);

	// entities
	int numEnts = 0;
	fprintf(mapsv, "entities [\n");
	// triggers
	for (i = 0; i < 255; i++)
	{
		if (!map.triggers[i].type || map.triggers[i].type == 0xFF)
			continue;
		if (map.triggers[i].type == TRIGGER_EXIT)
		{
			fprintf(mapsv, "exit {\n");
			fprintf(mapsv, " num %i\n", i);
			fprintf(mapsv, " x %i\n", map.triggers[i].x);
			fprintf(mapsv, " y %i\n", map.triggers[i].y);
			fprintf(mapsv, " map %i\n", map.triggers[i].parm1);
			fprintf(mapsv, " section %i\n", map.triggers[i].parm3);
			fprintf(mapsv, " destx %i\n", map.triggers[i].srcx);
			fprintf(mapsv, " desty %i\n", map.triggers[i].srcy);
			fprintf(mapsv, "}\n");
			numEnts++;
		}
		else if (map.triggers[i].type == TRIGGER_ENTRANCE)
		{
			fprintf(mapsv, "entrance {\n");
			fprintf(mapsv, " num %i\n", i);
			fprintf(mapsv, " x %i\n", map.triggers[i].x);
			fprintf(mapsv, " y %i\n", map.triggers[i].y);
			fprintf(mapsv, " map %i\n", map.triggers[i].parm1);
			fprintf(mapsv, " section %i\n", map.triggers[i].parm3);
			fprintf(mapsv, "}\n");
			numEnts++;
		}
		else if (map.triggers[i].type == TRIGGER_SPEECHMARK)
		{
			fprintf(mapsv, "speech {\n");
			fprintf(mapsv, " num %i\n", i);
			fprintf(mapsv, " x %i\n", map.triggers[i].x);
			fprintf(mapsv, " y %i\n", map.triggers[i].y);
			fprintf(mapsv, " speech %i\n", map.triggers[i].parm1);
			if (map.triggers[i].parm2 < 6500)
				fprintf(mapsv, " image %i\n", map.triggers[i].parm2);
			fprintf(mapsv, "}\n");
			numEnts++;
		}
		else if (map.triggers[i].type == TRIGGER_IMAGEMARK)
		{
			fprintf(mapsv, "image {\n");
			fprintf(mapsv, " num %i\n", i);
			fprintf(mapsv, " x %i\n", map.triggers[i].x);
			fprintf(mapsv, " y %i\n", map.triggers[i].y);
			fprintf(mapsv, " image %i\n", map.triggers[i].parm2);
			if (map.triggers[i].parm1 < 6500)
				fprintf(mapsv, " speech %i\n", map.triggers[i].parm1);
			fprintf(mapsv, "}\n");
			numEnts++;
		}
		else if (map.triggers[i].type == TRIGGER_TOUCH)
		{
			fprintf(mapsv, "touch {\n");
			fprintf(mapsv, " num %i\n", i);
			fprintf(mapsv, " x %i\n", map.triggers[i].x);
			fprintf(mapsv, " y %i\n", map.triggers[i].y);
			fprintf(mapsv, "}\n");
			numEnts++;
		}
		else if (map.triggers[i].type == TRIGGER_TELEPORT)
		{
			fprintf(mapsv, "teleport {\n");
			fprintf(mapsv, " num %i\n", i);
			fprintf(mapsv, " x %i\n", map.triggers[i].x);
			fprintf(mapsv, " y %i\n", map.triggers[i].y);
			fprintf(mapsv, " map %i\n", map.triggers[i].parm1);
			fprintf(mapsv, " section %i\n", map.triggers[i].parm3);
			fprintf(mapsv, " destx %i\n", map.triggers[i].srcx);
			fprintf(mapsv, " desty %i\n", map.triggers[i].srcy);
			fprintf(mapsv, "}\n");
			numEnts++;
		}
	}
	// touch zones
	for (i = 0; i < 80; i++)
	{
		for (j = 0; j < 80; j++)
		{
			if (map.contents[i][j] == CONTENTS_SACRIFICE)
			{
				fprintf(mapsv, "sacrifice {\n");
				fprintf(mapsv, " x %i\n", i);
				fprintf(mapsv, " y %i\n", j);
				fprintf(mapsv, "}\n");
				numEnts++;
			}
			else if (map.contents[i][j] == CONTENTS_SAVEGAME)
			{
				fprintf(mapsv, "saveGame {\n");
				fprintf(mapsv, " x %i\n", i);
				fprintf(mapsv, " y %i\n", j);
				fprintf(mapsv, "}\n");
				numEnts++;
			}
			else if (map.contents[i][j] == CONTENTS_BATMARK)
			{
				fprintf(mapsv, "batMark {\n");
				fprintf(mapsv, " x %i\n", i);
				fprintf(mapsv, " y %i\n", j);
				fprintf(mapsv, "}\n");
				numEnts++;
			}
		}
	}
	// enemies
	for (i = 0; i < 32; i++)
	{
		if (map.monsters[i].u1[12] == 0xFF)
			continue;
		fprintf(mapsv, "enemy {\n");
		fprintf(mapsv, " num %i\n", i);
		if (map.monsters[i].savenum != 0)
			fprintf(mapsv, " savenum %i\n", map.monsters[i].savenum);
		fprintf(mapsv, " x %i\n", map.monsters[i].x);
		fprintf(mapsv, " y %i\n", map.monsters[i].y);
		fprintf(mapsv, " type %i\n", map.monsters[i].charnum);
		if (map.monsters[i].target != 0xFFFF)
			fprintf(mapsv, " target %i\n", map.monsters[i].target);
		if (map.monsters[i].speechnum != 0xFFFF)
			fprintf(mapsv, " speech %i\n", map.monsters[i].speechnum);
		fprintf(mapsv, "}\n");
		numEnts++;
	}
	// items
	for (i = 0; i < 50; i++)
	{
		if (map.items[i].savenum == 0xFFFF)
			continue;
		fprintf(mapsv, "item {\n");
		fprintf(mapsv, " num %i\n", i);
		fprintf(mapsv, " savenum %i\n", map.items[i].savenum);
		fprintf(mapsv, " x %i\n", map.items[i].x);
		fprintf(mapsv, " y %i\n", map.items[i].y);
		fprintf(mapsv, " type %i\n", map.items[i].itemcode);
		if (map.items[i].itemcode >= MAPITEM_UNIQUE1 && map.items[i].itemcode <= MAPITEM_UNIQUE4)
			fprintf(mapsv, " unique %i\n", map.uniqueitems[map.items[i].itemcode - MAPITEM_UNIQUE1]);
		if (map.items[i].target != 0xFFFF)
			fprintf(mapsv, " target %i\n", map.items[i].target);
		if (map.items[i].hidden)
			fprintf(mapsv, " hidden\n");
		fprintf(mapsv, "}\n");
	}
	Print("%i entities\n", numEnts);
	fprintf(mapsv, "]\n");
	fprintf(mapsv, "}\n");
	fprintf(mapcl, "}\n");

	WriteClose(mapsv);
	WriteClose(mapcl);
	return 1;
}

/*
==========================================================================================

 MAP ANALYZER

==========================================================================================
*/

int MapAnalyze(char *mapfile, byte *fileData, int fileDataSize, char *outfile, bigfileheader_t *bigfileheader, FILE *bigfile, char *tilespath)
{
	char mapname[32], filename[1024];
	unsigned short map_num, map_section;
	FILE *f;
	int i, j, decSize;
	byte *dec;
	bo_map_t map;
	
	// extract map (get mapnum and section)
	ExtractFileName(mapfile, mapname);
	StripFileExtension(mapname, mapname);
	map_num = 0;
	map_section = 0;
	if (mapname[0] != 'm' || strlen(mapname) != 8)
		Print("warning: %s is not valid, map num and section is not known\n", mapname);
	else
	{
		// get map and section
		map_section = atoi((char *)(mapname + 6));
		mapname[6] = 0;
		map_num = atoi((char *)(mapname + 1));
	}

	// decompress
	dec = (byte *)LzDec(&decSize, fileData, 0, fileDataSize, true);
	if (dec == NULL)
		return 0;
	if (decSize != sizeof(bo_map_t))
		return 0;
	memcpy(&map, dec, sizeof(bo_map_t));

	// u1
	sprintf(filename, "%s_u1.csv", outfile);
	f = SafeOpen(filename, "a+");
	fprintf(f, "m%04i;s%02i;", map_num, map_section); 
	for (i = 0; i < 12; i+=1) fprintf(f, "%03i;", *(unsigned char *)(map.u1 + i));
	fprintf(f, "\n");
	fclose(f);

	// u2
	sprintf(filename, "%s_u2.csv", outfile);
	f = SafeOpen(filename, "a+");
	fprintf(f, "%04i;%02i;", map_num, map_section); 
	for (i = 0; i < 8; i+=1) fprintf(f, "%03i;", *(unsigned char *)(map.u2 + i));
	fprintf(f, "\n");
	fclose(f);

	// u4
	sprintf(filename, "%s_u4.csv", outfile);
	f = SafeOpen(filename, "a+");
	fprintf(f, "m%04i;s%02i;", map_num, map_section); 
	for (i = 0; i < 8; i+=1) fprintf(f, "%03i;", *(unsigned char *)(map.u4 + i));
	fprintf(f, "\n");
	fclose(f);

	// u5
	sprintf(filename, "%s_u5.csv", outfile);
	f = SafeOpen(filename, "a+");
	for (i = 0; i < 8; i+=1)
	{
		fprintf(f, "m%04i;s%02i;", map_num, map_section); 
		for (j = 0; j < 40; j += 1) 
			fprintf(f, "%03i;", *(unsigned char *)(map.u5[i] + j));
		fprintf(f, "\n");
	}
	fclose(f);

	// u6
	sprintf(filename, "%s_u6.csv", outfile);
	f = SafeOpen(filename, "a+");
	fprintf(f, "m%04i;s%02i;", map_num, map_section); 
	for (i = 0; i < 24; i+=1) fprintf(f, "%03i;", *(unsigned char *)(map.u6 + i));
	fprintf(f, "\n");
	fclose(f);

	// u7
	sprintf(filename, "%s_u7.csv", outfile);
	f = SafeOpen(filename, "a+");
	fprintf(f, "m%04i;s%02i;", map_num, map_section); 
	for (i = 0; i < 930; i+=1) fprintf(f, "%03i;", *(unsigned char *)(map.u7 + i));
	fprintf(f, "\n");
	fclose(f);

	// e
	sprintf(filename, "%s_e.csv", outfile);
	f = SafeOpen(filename, "a+");
	for (i = 0; i < 64; i++)
	{
		if (map.effects[i].sprite == 0xFF)
			continue;
		fprintf(f, "m%04i;s%02i;", map_num, map_section); 
		for (j = 0; j < sizeof(bo_effect_t); j += 1) 
			fprintf(f, "%03i;", *(unsigned char *)((byte *)&(map.effects[i]) + j));
		fprintf(f, "\n");
	}

	// o
	sprintf(filename, "%s_o.csv", outfile);
	f = SafeOpen(filename, "a+");
	fprintf(f, "m%05i%02i;", map_num, map_section); 
	for (i = 0; i < 10*164; i+=1) fprintf(f, "%03i;", *(unsigned char *)((byte *)map.objects + i));
	fprintf(f, "\n");
	fclose(f);

	// map stats
	sprintf(filename, "%s_mapstats.csv", outfile);
	f = SafeOpen(filename, "a+");
	fprintf(f, "m%05i%02i;%s;%s\n", map_num, map_section, (map.environments >= 0 && map.environments <= 1) ? environmentsnames[map.environments] : "?", (map.songnum >= 0 && map.songnum <= 10) ? songnames[map.songnum] : "?"); 
	fclose(f);

	return 1;
}


/*
==========================================================================================

 MAIN

==========================================================================================
*/


int MapConvert_Main(int argc, char **argv)
{
	int i = 1, devnum;
	char filename[MAX_OSPATH], tilespath[MAX_OSPATH], ext[5], outfile[MAX_OSPATH], *c;
	bool with_solid, with_triggers, with_lighting, show_save_id, toggled_objects, developer, txt, analyze;
	byte *fileData;
	int fileSize;

	Print("=== Map Converter ===\n");
	if (i < 1)
		Error("not enough parms");

	// get inner file
	strcpy(filename, argv[i]);
	ExtractFileExtension(filename, ext);
	i++;

	// get out file (if supplied)
	strcpy(outfile, filename);
	if (i < argc)
	{
		c = argv[i];
		if (c[0] != '-')
			strcpy(outfile, c);
	}
	StripFileExtension(outfile, outfile);

	// parse cmdline
	with_solid = false;
	with_triggers = false;
	with_lighting = false;
	show_save_id = false;
	toggled_objects = false;
	developer = false;
	txt = false;
	strcpy(tilespath, "");
	devnum = 0;
	analyze = false;
	for (i = i; i < argc; i++)
	{
		if (!strcmp(argv[i], "-tilespath"))
		{
			i++; 
			if (i < argc)
				strcpy(tilespath, argv[i]);
			ConvSlashW2U(tilespath);
			// add slash 
			if (tilespath[0])
				if (tilespath[strlen(tilespath)-1] != '/')
					strcat(tilespath, "/");
			Verbose("Option: tile files path '%s'\n", tilespath);
			continue;
		}
		if (!strcmp(argv[i], "-c"))
		{
			Verbose("Option: showing contents\n");
			with_solid = true;
			continue;
		}
		if (!strcmp(argv[i], "-t"))
		{
			Verbose("Option: showing triggers\n");
			with_triggers = true;
			continue;
		}
		if (!strcmp(argv[i], "-l"))
		{
			Verbose("Option: showing lighting\n");
			with_lighting = true;
			continue;
		}
		if (!strcmp(argv[i], "-s"))
		{
			Verbose("Option: showing save identifiers\n");
			show_save_id = true;
			continue;
		}
		if (!strcmp(argv[i], "-a"))
		{
			Verbose("Option: dynamic objects in alternative state\n");
			toggled_objects = true;
			continue;
		}
		if (!strcmp(argv[i], "-d"))
		{
			Verbose("Option: developer mode\n");
			developer = true;
			continue;
		}
		if (!strcmp(argv[i], "-devnum"))
		{
			i++;
			if (i < argc)
			{
				devnum = atoi(argv[i]);
				Verbose("Option: developer number %i\n", devnum);
				developer = true;
			}
			continue;
		}
		if (!strcmp(argv[i], "-f"))
		{
			i++;
			if (i < argc)
			{
				if (!stricmp(argv[i], "txt"))
					txt = true;
				Verbose("Option: convert map to text format used by blood omnicide\n");
			}
			continue;
		}
		if (!strcmp(argv[i], "-analyze"))
		{
			analyze = true;
			Verbose("Option: analyzing map\n");
			continue;
		}
	}

	// open source file, try load it
	fileSize = LoadFile(filename, &fileData);
	if (analyze)
		MapAnalyze(filename, fileData, fileSize, outfile, NULL, NULL, tilespath);
	else if (txt)
		MapExportTXT(filename, fileData, fileSize, outfile, NULL, NULL, tilespath);
	else
		MapExportTGA(filename, fileData, fileSize, outfile, NULL, NULL, tilespath, with_solid, with_triggers, with_lighting, show_save_id, toggled_objects, developer, devnum, false);
		
	return 0;
}