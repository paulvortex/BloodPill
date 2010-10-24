////////////////////////////////////////////////////////////////
//
// Blood Pill - zlib support for PK3 writing
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

// dll pointer
static dllhandle_t zlib_dll = NULL;

// dll names to load
const char* zlib_dllnames[] =
{
#if defined(WIN32)
	"zlib1.dll",
#elif defined(MACOSX)
	"libz.dylib",
#else
	"libz.so.1",
	"libz.so",
#endif
	NULL
};

// functions to import
static dllfunction_t zlib_funcs[] =
{
	{"inflate",			(void **) &qz_inflate},
	{"inflateEnd",		(void **) &qz_inflateEnd},
	{"inflateInit2_",	(void **) &qz_inflateInit2_},
	{"inflateReset",	(void **) &qz_inflateReset},
	{"deflateInit2_",   (void **) &qz_deflateInit2_},
	{"deflateEnd",      (void **) &qz_deflateEnd},
	{"deflate",         (void **) &qz_deflate},
	{NULL, NULL}
};

/*
====================
PK3_CloseLibrary

Unload the Zlib DLL
====================
*/

void PK3_CloseLibrary(void)
{
	UnloadDll(&zlib_dll);
}

/*
====================
PK3_OpenLibrary

Try to load the Zlib DLL
====================
*/

qboolean PK3_OpenLibrary(qboolean verbose)
{
	if (zlib_dll)
		return true;
	return LoadDll(zlib_dllnames, &zlib_dll, zlib_funcs, verbose);
}

/*
====================
PK3_Enabled

See if zlib (and pk3 routines) is available
====================
*/

qboolean PK3_Enabled(void)
{
	PK3_OpenLibrary(false); // to be safe
	return (zlib_dll != 0);
}
