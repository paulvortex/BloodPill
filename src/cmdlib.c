// Useful Functions Library
// coded by  Forest [LordHavoc] Hale

#define PATHSEPERATOR   '/'
#define __USE_BSD 1

#include <sys/types.h>
#include <sys/stat.h>
#if defined(WIN32) || defined(_WIN64)
#include <limits.h>
#include <direct.h>
#include <windows.h>
#endif
#include "cmdlib.h"

#include "mem.h"
#include "bloodpill.h"

// set these before calling CheckParm
int myargc;
char **myargv;

char		com_token[MAXTOKEN];
qboolean	com_eof;

//========================================================
// strlcat and strlcpy, from OpenBSD

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*	$OpenBSD: strlcat.c,v 1.11 2003/06/17 21:56:24 millert Exp $	*/
/*	$OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $	*/


#ifndef HAVE_STRLCAT
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}
#endif  // #ifndef HAVE_STRLCAT

#ifndef HAVE_STRLCPY
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

#endif  // #ifndef HAVE_STRLCPY

/*
=================
Lists
=================
*/

list_t *NewList()
{
	list_t *list;

	list = qmalloc(sizeof(list_t));
	memset(list, 0, sizeof(list_t));
	return list;
}

void FreeList(list_t *list)
{
	int i;

	if (!list)
		return;
	if (list->items)
		for (i = 0; i < list->items; i++)
			qfree(list->item[i]);
	qfree(list);
}

void ListAdd(list_t *list, const char *str, unsigned char x)
{
	if (!list)
		return;
	if (list->items >= MAX_LIST_ITEMS)
		return;
	list->item[list->items] = qmalloc(strlen(str)+1);
	list->x[list->items] = x;
	strcpy(list->item[list->items], str);
	list->items++;
}

/*
=================
Error

For abnormal program terminations
=================
*/
void Error (char *error, ...)
{
	va_list argptr;

	Print ("\n************ ERROR ************\n");

	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

#if _MSC_VER
	if (waitforkey)
	{
		Print("press any key\n");
		getchar();
	}
#endif
	exit (1);
}


/*

qdir will hold the path up to the quake directory, including the slash

  f:\quake\
  /raid/quake/

gamedir will hold qdir + the game directory (id1, id2, etc)

  */

char *copystring(char *s)
{
  char	*b;
  b = qmalloc(strlen(s)+1);
  strcpy (b, s);
  return b;
}



/*
================
I_DoubleTime
================
*/
double I_DoubleTime (void)
{
#if defined(WIN32) || defined(_WIN64)
	static DWORD starttime;
	static qboolean first = true;
	DWORD now;

	if (first)
		timeBeginPeriod (1);

	now = timeGetTime ();

	if (first)
	{
		first = false;
		starttime = now;
		return 0.0;
	}

	if (now < starttime)				// wrapped?
		return (now / 1000.0) + (LONG_MAX - starttime / 1000.0);

	if (now - starttime == 0)
		return 0.0;

	return (now - starttime) / 1000.0;
#else
	time_t	t;

	time (&t);

	return t;
#endif
}

// single mkdir
void Q_mkdir (char *path)
{
#if defined(WIN32) || defined(_WIN64)
  if (mkdir (path) != -1)
#else
  if (mkdir (path, 0777) != -1)
#endif
    return;
  if (errno != EEXIST)
    Error ("mkdir %s: %s",path, strerror(errno));
}

/*
============
CreatePath
============
*/
void CreatePath (char *path)
{
	char *ofs, save;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{
			// create the directory
			save = *ofs;
			*ofs = 0;
			if (path[0])
			{
				printf("mkdir: '%s'\n", path);
				Q_mkdir (path);
			}
			*ofs = save;
		}
	}
}

/*
============
FileTime

returns -1 if not present
============
*/
int	FileTime (char *path)
{
  struct	stat	buf;

  if (stat (path,&buf) == -1)
    return -1;

  return (int)buf.st_mtime;
}



/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
  int		c;
  int		len;

  len = 0;
  com_token[0] = 0;

  if (!data)
    return NULL;

  // skip whitespace
skipwhite:
  while ( (c = *data) <= ' ')
    {
      if (c == 0)
	{
	  com_eof = true;
	  return NULL;			// end of file;
	}
      data++;
    }

  // skip // comments
  if (c=='/' && data[1] == '/')
    {
      while (*data && *data != '\n')
	data++;
      goto skipwhite;
    }

// skip /* */ comments
	if (c == '/' && data[1] == '*')
	{
		data += 2;
		while (1)
		{
			if (!*data)
				break;
			if (*data != '*' || *(data+1) != '/')
				data++;
			else
			{
				data += 2;
				break;
			}
		}
		goto skipwhite;
	}

  // handle quoted strings specially
  if (c == '\"')
    {
      data++;
      do
	{
	  c = *data++;
	  if (c=='\"')
	    {
		  if (len == MAXTOKEN)
			  len = 0;
	      com_token[len] = 0;
	      return data;
	    }
	  if (len < MAXTOKEN)
		 com_token[len++] = c;
	} while (1);
    }

  // parse single characters
  if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
    {
	  if (len < MAXTOKEN)
	      com_token[len++] = c;
	  if (len == MAXTOKEN)
		  len = 0;
      com_token[len] = 0;
      return data+1;
    }

  // parse a regular word
  do
    {
	  if (len < MAXTOKEN)
	      com_token[len++] = c;
      data++;
      c = *data;
      if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
	break;
    } while (c>32);

  if (len == MAXTOKEN)
	  len = 0;
  com_token[len] = 0;
  return data;
}


int Q_strncasecmp (char *s1, char *s2, int n)
{
	int		c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;		// strings not equal
		}
		if (!c1)
			return 0;		// strings are equal
	}

	return -1;
}

int Q_strcasecmp (char *s1, char *s2)
{
  return Q_strncasecmp (s1, s2, 99999);
}


char *Q_strupr (char *start)
{
  char	*in;
  in = start;
  while (*in)
    {
      *in = toupper(*in);
      in++;
    }
  return start;
}

char *Q_strlower (char *start)
{
  char	*in;
  in = start;
  while (*in)
    {
      *in = tolower(*in);
      in++;
    }
  return start;
}

// convert to stupid Windows slashes
byte unixtowin(byte c)
{
	if (c == '/')
		return '\\';
	return c;

}
char *ConvSlash (char *start)
{
  char	*in;
  in = start;
  while (*in)
    {
      *in = unixtowin(*in);
      in++;
    }
  return start;
}


/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/


/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int CheckParm (char *check)
{
  int             i;

  for (i = 1;i<myargc;i++)
    {
      if ( !Q_strcasecmp(check, myargv[i]) )
	return i;
    }

  return 0;
}



/*
================
Q_filelength
================
*/
int Q_filelength (FILE *f)
{
  int		pos;
  int		end;

  pos = ftell (f);
  fseek (f, 0, SEEK_END);
  end = ftell (f);
  fseek (f, pos, SEEK_SET);

  return end;
}

FILE *SafeOpen (char *filename, char mode[])
{
  FILE	*f;

  f = fopen(filename, mode);

  if (!f)
    Error ("Error opening %s: %s",filename,strerror(errno));

  return f;
}

FILE *SafeOpenWrite (char *filename)
{
  FILE	*f;
  char path[MAX_BLOODPATH];

  // automatically make dir structure
  ExtractFilePath(filename, path);
  CreatePath(path);
  // open file
  f = fopen(filename, "wb");
  if (!f)
	Error ("Error opening %s: %s",filename,strerror(errno));
  return f;
}

void SafeRead (FILE *f, void *buffer, int count)
{
  if ( fread (buffer, 1, count, f) != (size_t)count)
    Error ("File read failure");
}


void SafeWrite (FILE *f, void *buffer, int count)
{
  if (fwrite (buffer, 1, count, f) != (size_t)count)
    Error ("File write failure");
}



/*
==============
LoadFile
==============
*/
int    LoadFile (char *filename, void **bufferptr)
{
  FILE	*f;
  int    length;
  void    *buffer;

  f = SafeOpen(filename, "rb");
  length = Q_filelength (f);
  buffer = qmalloc (length+1);
  ((char *)buffer)[length] = 0;
  SafeRead (f, buffer, length);
  fclose (f);

  *bufferptr = buffer;
  return length;
}


/*
==============
SaveFile
==============
*/
void SaveFile (char *filename, void *buffer, int count)
{
  FILE	*f;

  f = SafeOpenWrite(filename);
  SafeWrite (f, buffer, count);
  fclose (f);
}


void DefaultPath (char *path, char *basepath)
{
  char    temp[128];

  if (path[0] == PATHSEPERATOR)
    return;                   // absolute path location
  strcpy (temp,path);
  strcpy (path,basepath);
  strcat (path,temp);
}

void ReplaceExtension (char *path, char *oldextension, char *replacementextension, char *missingextension)
{
	char    *src;
	// if path has a .EXT, replace it with extension
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)

	for (src = path + strlen(path) - 1;src >= path && *src != '/' && *src != '\\' && *src != ':';src--)
	{
		if (*src == '.')
		{
			if (!oldextension || !Q_strncasecmp(src, oldextension, strlen(oldextension)))
			{
				*src = 0;
				strcat (path, replacementextension);
			}
			return;
		}
	}

	strcat (path, missingextension);
}

void DefaultExtension (char *path, const char *extension, size_t size_path)
{
	const char *src;

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strlcat (path, extension, size_path);
}

/*
====================
Extract file parts
====================
*/
void ExtractFilePath (char *path, char *dest)
{
  char    *src;

  src = path + strlen(path) - 1;

  //
  // back up until a \ or the start
  //
  while (src != path && *(src-1) != PATHSEPERATOR)
    src--;

  memcpy (dest, path, src-path);
  dest[src-path] = 0;
}

void ExtractFileBase (char *path, char *dest)
{
  char    *src;

  src = path + strlen(path) - 1;

  //
  // back up until a \ or the start
  //
  while (src != path && *(src-1) != PATHSEPERATOR)
    src--;

  while (*src && *src != '.')
    {
      *dest++ = *src++;
    }
  *dest = 0;
}

void StripFileExtension (char *path, char *dest)
{
  while (*path && *path != '.')
      *dest++ = *path++;
  *dest = 0;
}

void ExtractFileExtension (char *path, char *dest)
{
  char    *src;

  src = path + strlen(path) - 1;

  //
  // back up until a . or the start
  //
  while (src != path && *(src-1) != '.')
    src--;
  if (src == path)
    {
      *dest = 0;	// no extension
      return;
    }

  strcpy (dest,src);
}

void AddSuffix(char *outpath, char *inpath, char *suffix)
{
	char basename[MAX_BLOODPATH], ext[128];

	StripFileExtension(inpath, basename);
	ExtractFileExtension(inpath, ext);
	if (ext[0])
		sprintf(outpath, "%s%s.%s", basename, suffix, ext);
	else
		sprintf(outpath, "%s%s", basename, suffix);
}

qboolean FileExists(char *filename) 
{
	struct stat fileinfo;
	int statsucc;

	statsucc = stat(filename, &fileinfo);
	if (statsucc == 0)
		return true;
	return false;
}

/*
==============
ParseNum / ParseHex
==============
*/
int ParseHex (char *hex)
{
  char    *str;
  int    num;

  num = 0;
  str = hex;

  while (*str)
    {
      num <<= 4;
      if (*str >= '0' && *str <= '9')
	num += *str-'0';
      else if (*str >= 'a' && *str <= 'f')
	num += 10 + *str-'a';
      else if (*str >= 'A' && *str <= 'F')
	num += 10 + *str-'A';
      else
	Error ("Bad hex number: %s",hex);
      str++;
    }

  return num;
}


int ParseNum (char *str)
{
  if (str[0] == '$')
    return ParseHex (str+1);
  if (str[0] == '0' && str[1] == 'x')
    return ParseHex (str+2);
  return atol (str);
}



/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif

#ifdef __BIG_ENDIAN__

short   LittleShort (short l)
{
  byte    b1,b2;

  b1 = l&255;
  b2 = (l>>8)&255;

  return (b1<<8) + b2;
}

short   BigShort (short l)
{
  return l;
}


int    LittleLong (int l)
{
  byte    b1,b2,b3,b4;

  b1 = l&255;
  b2 = (l>>8)&255;
  b3 = (l>>16)&255;
  b4 = (l>>24)&255;

  return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    BigLong (int l)
{
  return l;
}


float	LittleFloat (float l)
{
  union {byte b[4]; float f;} in, out;

  in.f = l;
  out.b[0] = in.b[3];
  out.b[1] = in.b[2];
  out.b[2] = in.b[1];
  out.b[3] = in.b[0];

  return out.f;
}

float	BigFloat (float l)
{
  return l;
}


#else


short   BigShort (short l)
{
  byte    b1,b2;

  b1 = l&255;
  b2 = (l>>8)&255;

  return (b1<<8) + b2;
}

short   LittleShort (short l)
{
  return l;
}


int    BigLong (int l)
{
  byte    b1,b2,b3,b4;

  b1 = l&255;
  b2 = (l>>8)&255;
  b3 = (l>>16)&255;
  b4 = (l>>24)&255;

  return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    LittleLong (int l)
{
  return l;
}

float	BigFloat (float l)
{
  union {byte b[4]; float f;} in, out;

  in.f = l;
  out.b[0] = in.b[3];
  out.b[1] = in.b[2];
  out.b[2] = in.b[1];
  out.b[3] = in.b[0];

  return out.f;
}

float	LittleFloat (float l)
{
  return l;
}


#endif


//=======================================================


// FIXME: byte swap?

// this is a 16 bit, non-reflected CRC using the polynomial 0x1021
// and the initial and final xor values shown below...  in other words, the
// CCITT standard CRC used by XMODEM

#define CRC_INIT_VALUE	0xffff
#define CRC_XOR_VALUE	0x0000

static unsigned short crctable[256] =
{
  0x0000,	0x1021,	0x2042,	0x3063,	0x4084,	0x50a5,	0x60c6,	0x70e7,
  0x8108,	0x9129,	0xa14a,	0xb16b,	0xc18c,	0xd1ad,	0xe1ce,	0xf1ef,
  0x1231,	0x0210,	0x3273,	0x2252,	0x52b5,	0x4294,	0x72f7,	0x62d6,
  0x9339,	0x8318,	0xb37b,	0xa35a,	0xd3bd,	0xc39c,	0xf3ff,	0xe3de,
  0x2462,	0x3443,	0x0420,	0x1401,	0x64e6,	0x74c7,	0x44a4,	0x5485,
  0xa56a,	0xb54b,	0x8528,	0x9509,	0xe5ee,	0xf5cf,	0xc5ac,	0xd58d,
  0x3653,	0x2672,	0x1611,	0x0630,	0x76d7,	0x66f6,	0x5695,	0x46b4,
  0xb75b,	0xa77a,	0x9719,	0x8738,	0xf7df,	0xe7fe,	0xd79d,	0xc7bc,
  0x48c4,	0x58e5,	0x6886,	0x78a7,	0x0840,	0x1861,	0x2802,	0x3823,
  0xc9cc,	0xd9ed,	0xe98e,	0xf9af,	0x8948,	0x9969,	0xa90a,	0xb92b,
  0x5af5,	0x4ad4,	0x7ab7,	0x6a96,	0x1a71,	0x0a50,	0x3a33,	0x2a12,
  0xdbfd,	0xcbdc,	0xfbbf,	0xeb9e,	0x9b79,	0x8b58,	0xbb3b,	0xab1a,
  0x6ca6,	0x7c87,	0x4ce4,	0x5cc5,	0x2c22,	0x3c03,	0x0c60,	0x1c41,
  0xedae,	0xfd8f,	0xcdec,	0xddcd,	0xad2a,	0xbd0b,	0x8d68,	0x9d49,
  0x7e97,	0x6eb6,	0x5ed5,	0x4ef4,	0x3e13,	0x2e32,	0x1e51,	0x0e70,
  0xff9f,	0xefbe,	0xdfdd,	0xcffc,	0xbf1b,	0xaf3a,	0x9f59,	0x8f78,
  0x9188,	0x81a9,	0xb1ca,	0xa1eb,	0xd10c,	0xc12d,	0xf14e,	0xe16f,
  0x1080,	0x00a1,	0x30c2,	0x20e3,	0x5004,	0x4025,	0x7046,	0x6067,
  0x83b9,	0x9398,	0xa3fb,	0xb3da,	0xc33d,	0xd31c,	0xe37f,	0xf35e,
  0x02b1,	0x1290,	0x22f3,	0x32d2,	0x4235,	0x5214,	0x6277,	0x7256,
  0xb5ea,	0xa5cb,	0x95a8,	0x8589,	0xf56e,	0xe54f,	0xd52c,	0xc50d,
  0x34e2,	0x24c3,	0x14a0,	0x0481,	0x7466,	0x6447,	0x5424,	0x4405,
  0xa7db,	0xb7fa,	0x8799,	0x97b8,	0xe75f,	0xf77e,	0xc71d,	0xd73c,
  0x26d3,	0x36f2,	0x0691,	0x16b0,	0x6657,	0x7676,	0x4615,	0x5634,
  0xd94c,	0xc96d,	0xf90e,	0xe92f,	0x99c8,	0x89e9,	0xb98a,	0xa9ab,
  0x5844,	0x4865,	0x7806,	0x6827,	0x18c0,	0x08e1,	0x3882,	0x28a3,
  0xcb7d,	0xdb5c,	0xeb3f,	0xfb1e,	0x8bf9,	0x9bd8,	0xabbb,	0xbb9a,
  0x4a75,	0x5a54,	0x6a37,	0x7a16,	0x0af1,	0x1ad0,	0x2ab3,	0x3a92,
  0xfd2e,	0xed0f,	0xdd6c,	0xcd4d,	0xbdaa,	0xad8b,	0x9de8,	0x8dc9,
  0x7c26,	0x6c07,	0x5c64,	0x4c45,	0x3ca2,	0x2c83,	0x1ce0,	0x0cc1,
  0xef1f,	0xff3e,	0xcf5d,	0xdf7c,	0xaf9b,	0xbfba,	0x8fd9,	0x9ff8,
  0x6e17,	0x7e36,	0x4e55,	0x5e74,	0x2e93,	0x3eb2,	0x0ed1,	0x1ef0
};

void CRC_Init(unsigned short *crcvalue)
{
  *crcvalue = CRC_INIT_VALUE;
}

void CRC_ProcessByte(unsigned short *crcvalue, byte data)
{
  *crcvalue = (*crcvalue << 8) ^ crctable[(*crcvalue >> 8) ^ data];
}

unsigned short CRC_Value(unsigned short crcvalue)
{
  return crcvalue ^ CRC_XOR_VALUE;
}
