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

// blood omen map structure
typedef struct 
{
	unsigned short parm1;    // first parm, if script is not defined it's 0xFFFF, 0xFFFE is null value
	                         //  - destination/source map number (TRIGGER_EXIT, TRIGGER_ENTRANCE, TRIGGER_TELEPORT)
	                         //  - speech num (TRIGGER_INFOMARK, TRIGGER_IMAGEMARK)
	unsigned short parm2;    // second parm:
	                         //  - button num (TRIGGER_BUTTON)
	                         //  - pic num (TRIGGER_IMAGEMARK)
	byte           parm3;    // destination/source map section (TRIGGER_EXIT, TRIGGER_ENTRANCE, TRIGGER_TELEPORT)
	byte           srcx;     // sposition
	byte           srcy;     // sposition
	byte           type;     // script type, one of TRIGGER_
	byte           u2[3];    // ?
	byte           x;        // position
	byte           y;        // position
	byte           u3[3];    // ?
}bo_trigger_t;

typedef struct
{
	unsigned short savenum;  // unique savegame id
	byte           itemcode; // one of MAPITEM_ code
	byte           x;        // position
	byte           y;        // position
	byte           hidden;   // set to 1 if item is spawned by trigger (like barrel destruction)
	unsigned short target;   // targeted triggergroup
	byte           u[4];     // ?
}bo_item_t;

typedef struct
{
    unsigned short targetnum;// a triggergroup
	byte x;                  // position
	byte u1;                 // ?
	byte y;                  // position
	byte u2;                 // ?
	unsigned short tile1;    // tilenum
	unsigned short tile2;    // alternate state tilenum
	byte contents1;          // contents, see CONTENTS_ defines for possible content values
	byte contents2;          // alternate state contents
	byte u3[12];             // ?
}bo_atile_t;

typedef struct
{
	unsigned short target;   // a triggergroup to call
	unsigned short tile1;    // tilenum
	unsigned short tile2;    // alternate state tilenum
	byte flags;              // see BUTTONFLAG_* defs
	byte u1;                 // ?
	unsigned short savenum;  // unique savegame id
	byte u2;                 // ?
	byte u3;                 // ?
}bo_button_t;

typedef struct
{
	byte x;                  // position
	byte y;                  // position
	byte lightposx;          // position of lightsource
	byte lightposy;          // position of lightsource
	byte sprite;             // index of sprite from sprites array
	byte lightsizex;         // size of light polygons
	byte lightsizey;         // size of light polygons
	byte u1;                 // ?
	byte lightform;          // light form flags
	byte r;                  // red color component
	byte g;                  // green color component
	byte b;                  // blue color component
	unsigned short targetnum;// triggergroup
	byte start_on;           // 1 if effect starts on, 0 if starts off
	byte u3[5];              // ?
}bo_effect_t;

typedef struct
{
	byte data[164];
}bo_object_t;

typedef struct
{
	byte x;                  // start position
	byte y;                  // end position
	byte w;                  // width
	byte h;                  // height
	byte ofsx;               // multiply by 16 to get real offset
	byte ofsy;               // multiply by 16 to get real offset
	byte u1;
	byte u2;
}bo_grpobject_t;

typedef struct
{
	byte u1;                 // ?
	byte u2;                 // ?
	byte u3;                 // ?
	byte u4;                 // ?
	unsigned short tile1;    // base tile (grpobject really, not standart 32x32 tile)
	unsigned short tile2;    // toggled tile (grpobject)
	unsigned short tile3;    // destroyed tilee (grpobject)
	byte u5;                 // ?
	byte u6;                 // ?
	byte pushable;           // a strength required to push
	byte toggled;            // trigger can toggle it's state (means tile2 is there)
	byte u7;                 // ?
	byte spawnitems[3];      // item codes to spawn when toggled/destroyed
	byte u8;                 // ?
	byte destructible;       // destructible flags
	unsigned short savenum;  // unique savegame id
	byte u10;                // ?
	byte x;                  // position
	byte y;                  // position
	byte u11;                // ? 
	byte active;             // 1 is filled, otherwise 0
	byte u13;                // ? 
}bo_scenery_t;

typedef struct
{
	byte x;                  // position
	byte y;                  // position
	byte u1;                 // ?
	byte u2;                 // ?
}bo_path_t;

typedef struct
{
	bo_path_t paths[4];      // patrole paths
	byte u1[12];             // ?
	unsigned short savenum;  // unique savegame id
	byte u2[52];             // ?
	byte charnum;            // monster class (same as char* sprite)
	byte u3[10];             // ?
	unsigned short target;   // triggergroup to call once killed
	byte u4[20];             // ?
	byte lastpath;           // ?
	byte u6[18];             // ?
	byte u7[15];             // ?
	byte x;                  // position
	byte y;                  // position
	byte u8[4];              // ?
	unsigned short speechnum;// a speech number for disguise talk
	byte u9[6];              // ?
}bo_monster_t;

typedef struct
{
	// a chain of tilemap numbers used (to make a filenames)
	// each tilemap holds 64 tiles, tiles numbering is linear
	// like, a tile #131 is tile #3 from tilemap #2
	// being 2-byte (unsigned short) tilenum contains some flags (flag are starting from byte 10)
	// see TILEFLAG_ defines for possible tileflags
	unsigned short  tilemaps[40];
	// yet unknown info
	byte            u1[12];
	bo_object_t     objects[10];
	bo_monster_t    monsters[32];
	bo_grpobject_t  grpobjects[8][32];
	// animated tiles - a tiles that can be toggled between 2 states and have different contents for each state
	// used on doors, spikes that pops up on the floor etc.
	// animated tiles have triggergroup to be executed by script
	bo_atile_t      atiles[100];
	// a special buttons array holds all buttons used on level
	// pretty similar to animated tiles
	// buttons have triggergroup to be executed by script
	bo_button_t     buttons[20];
	// yet unknown info #2
	byte            u2[8];
	// scenery pushable objects - tables, chairs, stones etc.
	bo_scenery_t    scenery[256];
	// first tilemap
	unsigned short  backtiles[80][80];
	// second tilemap
	unsigned short  foretiles[80][80];
	// contents map - a contents value for each static tile (in game they get overriden by switchable tiles if presented)
	// see CONTENTS_ defines for possible content values
	byte            contents[80][80];
	// linear triggers array - all triggers used on map
	bo_trigger_t    triggers[255];
	// triggers are non-solid interaction points, activated when player stands in them
	// this array golds pointers to triggers
	byte            triggertiles[80][80];
	// items placed on level
	// itemcodes 0-9 are powerups, codes 10, 11, 12, 13 reserved for unique item codes
	// so there will be not more than 4 unique item type per level
	// unique items are: spells, weapons, forms, artifacts, tokens
	byte            uniqueitems[4];
	bo_item_t      	items[50];
	// yet unknown data
	byte            u4[8];
	byte            u5[8][40];
	byte            u6[24];
	// effects
	unsigned short  sprites[8];
	bo_effect_t     effects[64];
	// yet unknown data
	unsigned char   u7[936];
}bo_map_t;

// things that are not figured out yet:
// - how lightning is done (variable form lights, day-night light, ambient light)
// - monster's paths's switch (some paths are messed up)
// - counters and puzzle triggers?

// tileflags
#define TILEFLAG_UNKNOWN1       1024
#define TILEFLAG_NODRAW         2048
#define TILEFLAG_UNKNOWN3       4096
#define TILEFLAG_UNKNOWN4       8192
#define TILEFLAG_ALWAYSONTOP    16384
#define TILEFLAG_UNKNOWN6       32768
#define TILEFLAG_MASK           (1024 + 2048 + 4096 + 8192 + 16384 + 32768)
#define TILEFLAG_IMASK          (1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512)

// contents
#define CONTENTS_SOLID          1
#define CONTENTS_WATER          2
#define CONTENTS_LAVA           3
#define CONTENTS_FIRE           4
#define CONTENTS_SPIKES         5
#define CONTENTS_TRAPTELEPORT   6
#define CONTENTS_JUMPWALL       8
#define CONTENTS_SWAMP          11
#define CONTENTS_JUMPFENCE      10
#define CONTENTS_MISTWALK       12
#define CONTENTS_SACRIFICE      13
#define CONTENTS_ICE            19
#define CONTENTS_SAVEGAME       44
#define CONTENTS_BATMARK        45

// triggers
#define TRIGGER_EXIT            1
#define TRIGGER_ENTRANCE        2
#define TRIGGER_SPEECHMARK      3
#define TRIGGER_TOUCH           4
#define TRIGGER_TELEPORT        6
#define TRIGGER_IMAGEMARK       7

// button flags
#define BUTTONFLAG_TOGGLE		1
#define BUTTONFLAG_SECRET       2

// item codes
#define MAPITEM_SMALLBLOODFLASK 1
#define MAPITEM_BLOODFLASK      2
#define MAPITEM_RUNEPYRAMID     3
#define MAPITEM_PURPLESPHERE    4
#define MAPITEM_BLUESPHERE      5
#define MAPITEM_GREENSPHERE     6
#define MAPITEM_GOLDSPHERE      7
#define MAPITEM_REDSPHERE       8
#define MAPITEM_ANCIENTVIAL     9
#define MAPITEM_UNIQUE1         10 // unique item codes
#define MAPITEM_UNIQUE2         11 // unique item codes
#define MAPITEM_UNIQUE3         12 // unique item codes
#define MAPITEM_UNIQUE4         13 // unique item codes

// real item codes
#define ITEM_SPELL_INSPIREHATE  10
#define ITEM_SPELL_STUN         11
#define ITEM_SPELL_SPIRITWRACK  12
#define ITEM_SPELL_BLOODGOUT    13
#define ITEM_SPELL_BLOODSHOWER  14
#define ITEM_SPELL_SPIRITDEATH  15
#define ITEM_SPELL_LIGHT        16
#define ITEM_SPELL_LIGHTNING    17
#define ITEM_SPELL_REPEL        18
#define ITEM_SPELL_ENERGYBOLT   19
#define ITEM_SPELL_INCAPACITATE 20
#define ITEM_SPELL_CONTROLMIND  21
#define ITEM_SPELL_SANCTUARY    22
#define ITEM_ARTIFACT_FLAY      23
#define ITEM_ARTIFACT_PENTALICH 24
#define ITEM_ARTIFACT_IMPLODE   25
#define ITEM_ARTIFACT_FONT      26
#define ITEM_ARTIFACT_BANK      27
#define ITEM_ARTIFACT_HEART     28
#define ITEM_ARTIFACT_ANTITOXIN 29
#define ITEM_ARTIFACT_SLOWTIME  30
#define ITEM_TOKEN_MOEBIUS      31
#define ITEM_TOKEN_DEJOULE      32
#define ITEM_TOKEN_DOLL         33
#define ITEM_TOKEN_MALEK        34
#define ITEM_TOKEN_AZIMUTH      35
#define ITEM_TOKEN_MORTANIUS    36
#define ITEM_TOKEN_NUPRAPTOR    37
#define ITEM_TOKEN_BANE         38
#define ITEM_TOKEN_SIGNETRING   39
#define ITEM_TOKEN_TIMEDEVICE   40
#define ITEM_TOKEN_ANACROTHE    41
#define ITEM_WEAPON_MACE        43
#define ITEM_WEAPON_AXES        44
#define ITEM_WEAPON_FLAMESWORD  45
#define ITEM_WEAPON_SOULREAVER  46
#define ITEM_ARMOR_BONE         48
#define ITEM_ARMOR_CHAOS        49
#define ITEM_ARMOR_FLESH        50
#define ITEM_ARMOR_WRAITH       51
#define ITEM_FORM_BAT           53
#define ITEM_FORM_WOLF          54
#define ITEM_FORM_DISGUISE      55
#define ITEM_FORM_MIST          56
#define ITEM_ENDING_1           69
#define ITEM_ENDING_2           70

// functions
int MapScan(byte *buffer, int filelen);
void *LzDec(int *outbufsize, byte *inbuf, int startpos, int buflen, bool leading_filesize);
