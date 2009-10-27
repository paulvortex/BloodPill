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

// sprite formats
typedef enum
{
	SPR_QUAKE = 1,
	SPR_DARKPLACES = 32
}sprversion_t;

// sprite types
typedef enum
{
	SPR_VP_PARALLEL_UPRIGHT = 0,
	SPR_VP_FACING_UPRIGHT = 1,
	SPR_VP_PARALLEL = 2,
	SPR_ORIENTED = 3,
	SPR_VP_PARALLEL_ORIENTED = 4
}sprtype_t;

// frames and framegroup
typedef enum
{
	SPR_FRAME_SINGLE = 0,
	SPR_FRAME_GROUP = 1
}sprframetype_t;

// sprite header
typedef struct
{ 
	char	name[4];	// "IDSP"
	long	ver1;		// Version = 1
	long	type;		// See bove
	float	radius;		// Bounding Radius
	long	maxwidth;	// Width of the largest frame
	long	maxheight;	// Height of the largest frame
	long	nframes;	// Number of frames
	float	beamlength;	// pushs the sprite away, strange legacy from DOOM?
	long	synchtype;	// 0=synchron 1=random
} spr_t;

// sprite export flags
#define SPR_FLAGS_SHADOWONLY	0x000001	// write only shadow

// functions
void SPR_WriteFromRawblock(void *rawblock, char *outfile, sprversion_t version, sprtype_t type, int cx, int cy, int shadowpixel, int flags);


