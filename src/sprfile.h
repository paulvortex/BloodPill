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
	SPR_VP_PARALLEL_UPRIGHT = 0, // flames and such, vertical beam sprite, faces view plane
	SPR_VP_FACING_UPRIGHT = 1, // flames and such, vertical beam sprite, faces viewer's origin (not the view plane)
	SPR_VP_PARALLEL = 2, // normal sprite, faces view plane
	SPR_ORIENTED = 3, // bullet marks on walls, ignores viewer entirely
	SPR_OVERHEAD = 7, // VP_PARALLEL with couple of hacks
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
	char	name[4];	// 4 (+0) "IDSP"
	long	ver1;		// 4 (+4) Version = 1
	long	type;		// 4 (+8) See bove
	float	radius;		// 4 (+12) Bounding Radius
	long	maxwidth;	// 4 (+16) Width of the largest frame
	long	maxheight;	// 4 Height of the largest frame
	long	nframes;	// 4 Number of frames
	float	beamlength;	// 4 pushs the sprite away, strange legacy from DOOM?
	long	synchtype;	// 4 0=synchron 1=random
} spr_t;

// functions
void SPR_WriteFromRawblock(void *rawblock, char *outfile, sprversion_t version, sprtype_t type, int cx, int cy, float alpha, int flags, qboolean mergeintoexistingfile);
void SPR_Merge(list_t *mergelist, char *outfile, qboolean delmerged);

