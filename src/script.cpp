////////////////////////////////////////////////////////////////
//
// Blood Pill script launcher
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
#include "bigfile.h"
#include "zlib.h"
#include "mem.h"
#include "soxsupp.h"

// colormaps for Blood Omnicide
#define MAX_COLORMAPS		256
#define CUSTOM_COLORMAPS	32
#define MAX_COLORMAP_NAME	64
#define MAX_COLORMAP_STRING	256
typedef struct
{
	char name[MAX_COLORMAP_NAME];
	char map[MAX_COLORMAP_STRING]; // colormap string 'color''color2'...
}colormap_t;
typedef struct
{
	colormap_t maps[MAX_COLORMAPS];
	int num;
}colormaps_t;
colormaps_t *legacycolormaps;

// legacy macromodels for Blood Omnicide
#define MAX_LEGACYMODELCONFIG_NAME	64
#define MAX_LEGACYMODELS			512
#define MAX_LEGACYMODELSUBS			2048
typedef struct
{
	char name[MAX_COLORMAP_NAME];
	int colormapid; // index of colormap in colormaps array
	int speechoffset;
	char feedoffsets[128];
	char bloodoffsets[128];
	char spelloffsets[128];
}legacymodel_t;
typedef struct
{
	legacymodel_t models[MAX_LEGACYMODELS];
	int num;
}legacymodels_t;
legacymodels_t *legacymodels;

// legacy model sub parts for Blood Omnicide
typedef struct
{
	char name[MAX_COLORMAP_NAME];
	int basemodel;
	char orient[8];
	float scale;
}legacymodelsub_t;
typedef struct
{
	legacymodelsub_t subs[MAX_LEGACYMODELSUBS];
	int num;
}legacymodelsubs_t;
legacymodelsubs_t *legacymodelsubs;

double scriptstarted;
char path[MAX_OSPATH] = { 0 };
char bigfilepath[MAX_OSPATH] = { 0 };
char spr_parms[MAX_OSPATH] = { 0 };
char extract_parms[MAX_OSPATH] = { 0 };
bigfileentry_t *entry = NULL;
bigfileheader_t *bigfile = NULL;
FILE *bigfilehandle;

void SpriteLitFileName(char *in)
{
	char name[MAX_OSPATH], path[MAX_OSPATH];

	ExtractFilePath(in, path);
	ExtractFileName(in, name);
	sprintf(in, "%s!%s", path, name);
}

void Script_Parse(char *filename, char *basepath)
{
	double cscale, aver, diff;
	byte *data, *scriptstring;
	char *t, *s;
	int scriptsize, n, len;
	bool bloodomnicide = false, bigfileinstall = false, litsprites = false, allowdebug = true, writingpk3 = false;
	int i, currentmodel = -1, minp, maxp, sargc, stt = 0, stt_total = 0, c[3], is_adpcm;
	char tempchar, **sargv, outfile[MAX_OSPATH], infile[MAX_OSPATH], cs[32];
	char soxparm1[1024], soxparm2[1024], soxparm3[1024], soxparm4[1024];
	bigfileentry_t *oldentry;
	rawinfo_t rawinfo;
	rawblock_t *rawblock;
	FILE *f;
	pk3_file_t *pk3;
	strcpy(path, basepath);
	
	// read file
	Verbose("%s:\n", filename);
	sargv = (char **)mem_alloc(sizeof(char *)*32);
	for (sargc = 0; sargc < 32; sargc++)
		sargv[sargc] = (char *)mem_alloc(128);
	scriptsize = LoadFile(filename, &scriptstring);
	FlushRawInfo(&rawinfo);

	// init
	strcpy(soxparm1, "");
	strcpy(soxparm2, "");
	strcpy(soxparm3, "");
	strcpy(soxparm4, "");

	// parse file
	s = (char *)scriptstring;
	n = 1;
	while (*s)
	{
		// parse line
		t = s;
		while(*s && *s != '\n' && *s != '\r')
			s++;
		if (!*s)
			break;
		tempchar = *s;
		*s = 0;
		// parse line
		if (t = COM_Parse(t))
		{
			if (stt_total) // show pacifier
			{
				i = (int)((stt * 100) / stt_total);
				PercentPacifier("%i", i);
			}
			// ---- General Part ----
			if (!strcmp(com_token, "path")) 
			{
				if (!(t = COM_Parse(t)))
					Error("path: error parsing parm 1 on line %i\n", n);
				else
				{
					if (writingpk3)
						sprintf(path, "%s/", com_token);
					else if (com_token[0])
						sprintf(path, "%s%s/", basepath, com_token);
					else
						strcpy(path, basepath);
				}
				goto next;
			}
			if (!strcmp(com_token, "print")) 
			{
				while (t = COM_Parse(t))
					printf("%s", com_token);
				printf("\n");
				goto next;
			}
			if (!strcmp(com_token, "option")) 
			{
				if (!(t = COM_Parse(t)))
					Error("option: error parsing parm 1 on line %i\n", n);
				else
				{
					if (!strcmp(com_token, "spr_parms"))
					{
						if (!(t = COM_Parse(t)))
							Error("option: error parsing parm 2 on line %i\n", n);
						else
							strcpy(spr_parms, com_token);
					}
					else if (!strcmp(com_token, "extract_parms"))
					{
						if (!(t = COM_Parse(t)))
							Error("option: error parsing parm 2 on line %i\n", n);
						else
							strcpy(extract_parms, com_token);
					}
					else if (!strcmp(com_token, "sox_general"))
					{
						if (!(t = COM_Parse(t)))
							Error("option: error parsing parm 2 on line %i\n", n);
						else
							strcpy(soxparm1, com_token);
					}
					else if (!strcmp(com_token, "sox_input"))
					{
						if (!(t = COM_Parse(t)))
							Error("option: error parsing parm 2 on line %i\n", n);
						else
							strcpy(soxparm2, com_token);
					}
					else if (!strcmp(com_token, "sox_output"))
					{
						if (!(t = COM_Parse(t)))
							Error("option: error parsing parm 2 on line %i\n", n);
						else
							strcpy(soxparm3, com_token);
					}
					else if (!strcmp(com_token, "sox_effect"))
					{
						if (!(t = COM_Parse(t)))
							Error("option: error parsing parm 2 on line %i\n", n);
						else
							strcpy(soxparm4, com_token);
					}
					else if (!strcmp(com_token, "omnicideinstall")) 
					{
						if (!(t = COM_Parse(t)))
							Error("option: error parsing parm 2 on line %i\n", n);
						else
						{
							bloodomnicide = true;
							stt_total = atoi(com_token);
						}
					}
					//else if (!strcmp(com_token, "bigfilerepack")) 
					//	bigfilerepack = true;
					else if (!strcmp(com_token, "litsprites"))
						litsprites = true;
				}
				goto next;
			}
			if (!strcmp(com_token, "export")) 
			{
				if (!(t = COM_Parse(t)))
					Error("export: error parsing parm 1 on line %i\n", n);
				else
				{
					if (!strcmp(com_token, "legacy.nsx"))
					{
						// Blood Omnicide - write legacy.nsx
						sprintf(outfile, "%slegacy.nsx", path);
						f = SafeOpenWrite(outfile);
						fputs("// Legacy stuff script file\n", f);
						fputs("\n[macromodels]name=colormap,speechofs\n", f);
						for (i = 0; i < legacymodels->num; i++)
							fprintf(f, "%s=%i,%i\n", legacymodels->models[i].name, legacymodels->models[i].colormapid, legacymodels->models[i].speechoffset);
						fputs("\n[feed_tags]name=offsets\n", f);
						for (i = 0; i < legacymodels->num; i++)
							if (legacymodels->models[i].feedoffsets[0])
								fprintf(f, "%s=%s\n", legacymodels->models[i].name, legacymodels->models[i].feedoffsets);
						fputs("\n[blood_tags]name=offsets\n", f);
						for (i = 0; i < legacymodels->num; i++)
							if (legacymodels->models[i].bloodoffsets[0])
								fprintf(f, "%s=%s\n", legacymodels->models[i].name, legacymodels->models[i].bloodoffsets);
						fputs("\n[spell_tags]name=offsets\n", f);
						for (i = 0; i < legacymodels->num; i++)
							if (legacymodels->models[i].spelloffsets[0])
								fprintf(f, "%s=%s\n", legacymodels->models[i].name, legacymodels->models[i].spelloffsets);
						fputs("\n[models]name={type,scale,paletteindex}\n", f);
						for (i = 0; i < legacymodelsubs->num; i++)
							fprintf(f, "%s=%s,%.2f,%i\n", legacymodelsubs->subs[i].name, legacymodelsubs->subs[i].orient, legacymodelsubs->subs[i].scale, legacymodels->models[legacymodelsubs->subs[legacymodelsubs->num].basemodel].colormapid);
						WriteClose(f);
					}
					else if (!strcmp(com_token, "colormaps.nsx"))
					{
						// Blood Omnicide - write colormaps.nsx
						sprintf(outfile, "%scolormaps.nsx", path);
						f =	SafeOpenWrite(outfile);
						fputs("// Particle colormaps  file\n", f);
						fputs("// colormaps 0-31 are system ones \n", f);
						fputs("\n[colormaps]index={colormap}\n", f);
						for (i = 0; i < legacycolormaps->num; i++)
							if (legacycolormaps->maps[i].map[0])
								fprintf(f, "%i=%s\n", i, legacycolormaps->maps[i].map);
						WriteClose(f);
					}
					else
						Error("export: unknown parm 1 on line %i\n", n);
				}
				goto next;
			}
			if (!strcmp(com_token, "extract")) 
			{
				// on each extract we are packing all wrapped files to PK3
				if (writingpk3)
				{
					PK3_AddWrappedFiles(pk3);
					WrapFileWritesToMemory();
				}
				// check if bigfile is opened
				if (!bigfile)
					Error("extract: requires bigfile on line %i\n", n);
				else
				{
					// find entry
					if (!(t = COM_Parse(t)))
						Error("extract: error parsing parm 1 on line %i\n", n);
					else
					{
						entry = BigfileGetEntry(bigfile, BigfileEntryHashFromString(com_token, true));
						if (entry == NULL)
							Error("extract: error getting entry '%s' on line %i\n", com_token, n);
						else
						{
							// build outfile
							if (!(t = COM_Parse(t)))
								Error("extract: error parsing parm 2 on line %i\n", n);
							else
							{
								sprintf(outfile, "%s%s", path, com_token);
								// build arguments string (global, then local)
								i = 1; // pacifier cost
								sargc = 0;
								// add extract_parms first
								data = (byte *)extract_parms;
								while(data = (byte *)COM_Parse((char *)data))
								{
									if (sargc >= 32)
										Error("extract: too many arguments!\n", n);
									else
										strncpy(sargv[sargc], com_token, 128);
									sargc++;
								}
								// then generic parms
								is_adpcm = 0;
								while (t = COM_Parse(t))
								{
									if (!strcmp(com_token, "-cost"))
									{
										if (t = COM_Parse(t))
											i = atoi(com_token);
										continue;
									}
									if (!strcmp(com_token, "-adpcm"))
									{
										if (t = COM_Parse(t))
											is_adpcm = atoi(com_token);
										continue;
									}
									// add
									if (sargc >= 32)
										Error("extract: too many arguments!\n", n);
									else
										strncpy(sargv[sargc], com_token, 128);
									sargc++;
								}
								// check for null entry
								if (!entry->size)
									Error("extract: null entry %X on line %i\n", entry->hash, n);

								// extract
								BigfileScanFiletype(bigfilehandle, entry, true, RAW_TYPE_UNKNOWN, true);
								if (is_adpcm && entry->type == BIGENTRY_UNKNOWN)
								{
									entry->type = BIGENTRY_RAW_ADPCM;
									entry->adpcmrate = is_adpcm;
								}
								//printf("entry(%s) %X = %s : %s\n", bigentryext[entry->type], entry->hash, entry->name, outfile);
								BigFile_ExtractEntry(sargc, sargv, bigfilehandle, entry, outfile);
								stt += i;
							}
						}
					}
				}
				goto next;
			}
			// convert external file
			if (!strcmp(com_token, "sox")) 
			{
				if (!(t = COM_Parse(t))) // general vmd
					Error("copy: error parsing parm 1 on line %i\n", n);
				else
				{
					GetRealPath(infile, com_token);
					if (!(t = COM_Parse(t)))
						Error("copy: error parsing parm 2 on line %i\n", n);
					else
					{
						sprintf(outfile, "%s%s", path, com_token);
						// then generic parms
						i = 0;
						while (t = COM_Parse(t))
						{
							if (!strcmp(com_token, "-cost"))
							{
								if (t = COM_Parse(t))
									i = atoi(com_token);
								continue;
							}
							if (!strcmp(com_token, "-c"))
							{
								if (t = COM_Parse(t))
									strcpy(soxparm1, com_token);
								continue;
							}
							if (!strcmp(com_token, "-i"))
							{
								if (t = COM_Parse(t))
									strcpy(soxparm2, com_token);
								continue;
							}
							if (!strcmp(com_token, "-o"))
							{
								if (t = COM_Parse(t))
									strcpy(soxparm3, com_token);
								continue;
							}
							if (!strcmp(com_token, "-e"))
							{
								if (t = COM_Parse(t))
									strcpy(soxparm4, com_token);
								continue;
							}
						}
						// convert
						if (!SoX_FileToData(infile, soxparm1, soxparm2, soxparm3, &len, &data, soxparm4))
							Error("sox: failed to process %s on line %i\n", infile, n);
						else
						{
							if (writingpk3)
								PK3_AddFile(pk3, outfile, data, len);
							else
								SaveFile(outfile, data, len);
							if (i)
								stt += i;
							else
								stt += (int)max(1, len / 1024 / 1024);
							mem_free(data);
						}
					}
				}
				goto next;
			}
			// copy infile outfile
			if (!strcmp(com_token, "copy")) 
			{
				if (!(t = COM_Parse(t)))
					Error("copy: error parsing parm 1 on line %i\n", n);
				else
				{
					strcpy(infile, com_token);
					if (!strcmp(infile, "*bigfile"))
						strcpy(infile, bigfilepath);
					if (!(t = COM_Parse(t)))
						Error("copy: error parsing parm 2 on line %i\n", n);
					else
					{
						sprintf(outfile, "%s%s", path, com_token);
						// additionsl parms
						i = 0;
						while (t = COM_Parse(t))
						{
							if (!strcmp(com_token, "-cost"))
							{
								if (t = COM_Parse(t))
									i = atoi(com_token);
								continue;
							}
						}
						// copy
						if (writingpk3)
						{
							len = LoadFile(infile, &data);
							PK3_AddFile(pk3, outfile, data, len);
							mem_free(data);
						}
						else
						{
							len = FileSize(infile);
							if (!CopyFile(infile, outfile, false))
								Error("CopyFile('%s'->'%s'): failed with error: %s", infile, outfile, strerror(GetLastError()));
						}
						if (i)
							stt += i;
						else
							stt += (int)max(1, len / 1024 / 1024);
					}
				}
				goto next;
			}
			// sprcopy infile outfile
			if (!strcmp(com_token, "sprcopy")) 
			{
				if (!(t = COM_Parse(t)))
					Error("copy: error parsing parm 1 on line %i\n", n);
				else
				{
					strcpy(infile, com_token);
					if (!(t = COM_Parse(t)))
						Error("copy: error parsing parm 2 on line %i\n", n);
					else
					{
						sprintf(outfile, "%s%s", path, com_token);
						if (litsprites)
							SpriteLitFileName(outfile);
						len = LoadFile(infile, &data);
						if (writingpk3)
							PK3_AddFile(pk3, outfile, data, len);
						else
							SaveFile(outfile, data, len);
						mem_free(data);
					}
					stt += 1;
				}
				goto next;
			}
			// pk3 file - begin a new pk3 file and set all output to it
			if (!strcmp(com_token, "pk3")) 
			{
				if (!(t = COM_Parse(t)))
					Error("pk3: error parsing parm 1 on line %i\n", n);
				else
				{
					// close old pk3 file
					if (writingpk3)
					{
						PK3_AddWrappedFiles(pk3);
						PK3_Close(pk3);
					}
					// fixme: make path consistent immediately?
					// begin new pk3 file
					sprintf(outfile, "%s%s", path, com_token);
					pk3 = PK3_Create(outfile);
					WrapFileWritesToMemory();
					writingpk3 = true;
				}
				goto next;
			}
			// pk3end - close current pk3 file
			if (!strcmp(com_token, "pk3end")) 
			{
				if (writingpk3)
				{
					PK3_AddWrappedFiles(pk3);
					PK3_Close(pk3);
				}
				writingpk3 = false;
			}
			// ---- Debug part ----
			if (allowdebug)
			{
				if (!strcmp(com_token, "break")) 
				{
					if (writingpk3)
					{
						PK3_AddWrappedFiles(pk3);
						PK3_Close(pk3);
						writingpk3 = false;
					}
					printf("script execution time: %f\n", I_DoubleTime() - scriptstarted);
					printf("%i statements\n", stt);
					printf("break statement\n");
					break;
				}
			}
			// ---- Blood Omnicide Part ----
			if (bloodomnicide)
			{
				// colormap 'name' - register colormap
				if (!strcmp(com_token, "colormap")) 
				{
					if (!(t = COM_Parse(t)))
						Error("colormap: error parsing parm 1 on line %i\n", n);
					else
					{
						// find colormap or allocate new
						for (i = 0; i < legacycolormaps->num; i++)
							if (!strcmp(legacycolormaps->maps[i].name, com_token))
								break;
						if (i == legacycolormaps->num)
						{
							if (legacycolormaps->num >= MAX_COLORMAPS)
								Error("colormap: MAX_COLORMAPS = %i exceded on line %i\n", MAX_COLORMAPS, n);
							else
								legacycolormaps->num = i + 1;
						}
						// set colormap
						strncpy(legacycolormaps->maps[i].name, com_token, MAX_COLORMAP_NAME);
						if (t = COM_Parse(t)) // values string
							strncpy(legacycolormaps->maps[i].map, com_token, MAX_COLORMAP_STRING);
					}
					goto next;
				}
				// state 'message' - casts verbose message
				if (!strcmp(com_token, "state")) 
				{
					if (!(t = COM_Parse(t)))
						Error("state: error parsing parm 1 on line %i\n", n);
					else
						Verbose("%s...\n", com_token);
					goto next;
				}
				// model 'modelname' [customcolormap]
				if (!strcmp(com_token, "model")) 
				{
					if (!(t = COM_Parse(t)))
						Error("model: error parsing parm 1 on line %i\n", n);
					else
					{
						// once beginning new model we are packing all wrapped files to PK3
						if (writingpk3)
						{
							PK3_AddWrappedFiles(pk3);
							WrapFileWritesToMemory();
						}
						// find model or allocate new
						for (i = 0; i < legacymodels->num; i++)
							if (!strcmp(legacymodels->models[i].name, com_token))
								break;
						if (i == legacymodels->num)
						{
							if (legacymodels->num >= MAX_LEGACYMODELS)
								Error("model: MAX_LEGACYMODELS = %i exceded on line %i\n", MAX_LEGACYMODELS, n);	
							else
								legacymodels->num = i + 1;
						}
						strncpy(legacymodels->models[i].name, com_token, MAX_COLORMAP_NAME);
						currentmodel = i;
						// allocate new colormap or use existing
						i = legacycolormaps->num;
						if (t = COM_Parse(t))
						{
							for (i = 0; i < legacycolormaps->num; i++)
								if (!strcmp(legacycolormaps->maps[i].name, com_token))
									break;
						}
						if (i == legacycolormaps->num)
						{
							if (legacycolormaps->num >= MAX_COLORMAPS)
								Error("model: MAX_COLORMAPS = %i exceded on line %i\n", MAX_COLORMAPS, n);	
							else
							{
								i = max(32, i); // first 32 colormaps are system ones
								legacycolormaps->num = i + 1; 
							}
						}
						legacymodels->models[currentmodel].colormapid = i;
						strcpy(legacycolormaps->maps[i].name, legacymodels->models[currentmodel].name);
					}
					goto next;
				}
				//  speech 'vertical_offset'
				if (!strcmp(com_token, "speech")) 
				{
					if (currentmodel < 0)
						Error("speech: requires model to be set first on line %i\n", n);
					else
					{
						if (!(t = COM_Parse(t)))
							Error("speech: error parsing parm 1 on line %i\n", n);
						else
							legacymodels->models[currentmodel].speechoffset = atoi(com_token);
					}
					goto next;
				}
				//  feed 'offsets'
				if (!strcmp(com_token, "feed")) 
				{
					if (currentmodel < 0)
						Error("feed: requires model to be set first on line %i\n", n);
					else
					{
						if (!(t = COM_Parse(t)))
							Error("feed: error parsing parm 1 on line %i\n", n);
						else
							strncpy(legacymodels->models[currentmodel].feedoffsets, com_token, 128);
					}
					goto next;
				}
				//  blood 'offsets'
				if (!strcmp(com_token, "blood")) 
				{
					if (currentmodel < 0)
						Error("blood: requires model to be set first on line %i\n", n);
					else
					{
						if (!(t = COM_Parse(t)))
							Error("blood: error parsing parm 1 on line %i\n", n);
						else
							strncpy(legacymodels->models[currentmodel].bloodoffsets, com_token, 128);
					}
					goto next;
				}
				//   spell 'offsets'
				if (!strcmp(com_token, "spell")) 
				{
					if (currentmodel < 0)
						Error("spell: requires model to be set first on line %i\n", n);
					else
					{
						if (!(t = COM_Parse(t)))
							Error("spell: error parsing parm 1 on line %i\n", n);
						else
							strncpy(legacymodels->models[currentmodel].spelloffsets, com_token, 128);
					}
					goto next;
				}
				//   mdlsub 'spr32name' 'scaletype' 'orientation'
				//    Scaletypes:
				//         "player" : kain models
				//         "monster": default monster models
				//         "bigger" : a slight bigger monster
				//         "effect" : not scaled (cos attached)
				//         "death"  : death animations is slight bigger because they are decals
				//    Orientations:
				//         "orient" : oriented in 3D space, hence 8 sprites representing eight directions
				//         "flat"   : always turned to player
				//         "decal"  : ignores viewer completely, used no death
				if (!strcmp(com_token, "sub")) 
				{	
					if (!(t = COM_Parse(t)))
						Error("sub: error parsing parm 1 on line %i\n", n);
					else
					{
						if (currentmodel < 0)
							Error("sub: requires model to be set first on line %i\n", n);
						else
						{
							// find sub or allocate new
							if (legacymodelsubs->num >= MAX_LEGACYMODELS)
								Error("sub: MAX_LEGACYMODELS = %i exceded on line %i\n", MAX_LEGACYMODELS, n);	
							else
							{
								strncpy(legacymodelsubs->subs[legacymodelsubs->num].name, com_token, MAX_COLORMAP_NAME);
								// read scaletype
								if (!(t = COM_Parse(t)))
									Error("sub: error parsing parm 2 on line %i\n", n);
								else
								{
									if (!strcmp(com_token, "player"))
										legacymodelsubs->subs[legacymodelsubs->num].scale = 0.82f;
									else if (!strcmp(com_token, "monster"))
										legacymodelsubs->subs[legacymodelsubs->num].scale = 0.92f;
									else if (!strcmp(com_token, "bigger"))
										legacymodelsubs->subs[legacymodelsubs->num].scale = 1.20f;
									else if (!strcmp(com_token, "effect"))
										legacymodelsubs->subs[legacymodelsubs->num].scale = 1.00f;
									else if (!strcmp(com_token, "death"))
										legacymodelsubs->subs[legacymodelsubs->num].scale = 1.00f;
									else
										legacymodelsubs->subs[legacymodelsubs->num].scale = (float)atof(com_token);
									// read orientation
									if (!(t = COM_Parse(t)))
										Error("sub: error parsing parm 3 on line %i\n", n);
									else
									{
										strncpy(legacymodelsubs->subs[legacymodelsubs->num].orient, com_token, 8);
										legacymodelsubs->subs[legacymodelsubs->num].basemodel = currentmodel;
										legacymodelsubs->num = legacymodelsubs->num + 1;
									}
								}
							}
						}
					}
					goto next;
				}
				//  spr 'entry' 'filename' commandlineargs
				//  spr merge 'base' 'add1' 'add2' ...
				if (!strcmp(com_token, "spr")) 
				{	
					// check if bigfile is opened
					if (!bigfile)
						Error("spr: requires bigfile on line %i\n", n);
					else
					{
						oldentry = entry;
						if (!(t = COM_Parse(t)))
							Error("spr: error parsing parm 1 on line %i\n", n);
						else
						{
							// find entry
							if (com_token[0] != '-')
								entry = BigfileGetEntry(bigfile, BigfileEntryHashFromString(com_token, true));
							if (entry == NULL)
								Error("spr: error getting entry on line %i\n", n);
							else
							{
								// build outfile
								if (!(t = COM_Parse(t)))
									Error("spr: error parsing parm 2 on line %i\n", n);
								else
								{
									sprintf(outfile, "%s%s.spr32", path, com_token);
									if (litsprites)
										SpriteLitFileName(outfile);
									// build arguments string (global, then local)
									sargc = 0;
									while (t = COM_Parse(t))
									{
										if (sargc >= 32)
											Error("spr: too many arguments!\n", n);
										else
										{
											strncpy(sargv[sargc], com_token, 128);
											sargc++;
										}
									}
									data = (byte *)spr_parms;
									while (data = (byte *)COM_Parse((char *)data))
									{
										if (sargc >= 32)
											Error("spr: too many arguments!\n", n);
										else
										{
											strncpy(sargv[sargc], com_token, 128);
											sargc++;
										}
									}
									// load rawblock
									if (!entry->data)
									{
										data = (byte *)mem_alloc(entry->size);
										BigfileSeekContents(bigfilehandle, data, entry);
										entry->data = (byte *)RawExtract(data, entry->size, &rawinfo, false, false, RAW_TYPE_UNKNOWN);
									}
									// do extract
									BigFile_ExtractRawImage(sargc, sargv, outfile, entry, (rawblock_t *)entry->data, "spr32");
									// unload old entry
									if (oldentry && oldentry->data && oldentry != entry)
									{
										FreeRawBlock((rawblock_t *)oldentry->data);
										oldentry->data = NULL;
									}
									stt += 2;
								}
							}
						}
					}
					goto next;
				}
				// makecolors 'min_index' 'max_index' 'colorscale' - should be called after spr, extracts palette to nsx-style colormap
				if (!strcmp(com_token, "makecolors")) 
				{	
					if (currentmodel < 0)
						Error("makecolors: requires model to be set first on line %i\n", n);
					else
					{
						if (!(t = COM_Parse(t))) 
							Error("makecolors: error parsing parm 1 on line %i\n", n);
						else
						{
							minp = min(255, max(0, atoi(com_token)));
							if (!(t = COM_Parse(t)))
								Error("makecolors: error parsing parm 2 on line %i\n", n);
							else
							{
								maxp = min(255, max(0, atoi(com_token)));
								if (!(t = COM_Parse(t)))
									Error("makecolors: error parsing parm 3 on line %i\n", n);
								else
								{
									cscale = atof(com_token);
									if (!entry->data)
										Error("makecolors: entry not loaded on line %i, try sub first\n", n);
									else
									{
										strcpy(legacycolormaps->maps[legacymodels->models[currentmodel].colormapid].map, "");
										rawblock = (rawblock_t *)entry->data;
										for (i = minp; i < maxp; i++)
										{
											c[0] = rawblock->colormap[i*3];
											c[1] = rawblock->colormap[i*3 + 1];
											c[2] = rawblock->colormap[i*3 + 2];
											aver = (c[0] + c[1] + c[2])/3;
											diff = max(c[0], max(c[1], c[2]));
											// reject any color thats too gray
											//if (!diff || aver/diff > 0.8)
											//	continue;
											sprintf(cs, "'%i %i %i'", (int)(c[0]*cscale), (int)(c[1]*cscale), (int)(c[2]*cscale));
											strcat(legacycolormaps->maps[legacymodels->models[currentmodel].colormapid].map, cs);
										}
										// unload entry
										if (entry && entry->data)
										{
											FreeRawBlock((rawblock_t *)entry->data);
											entry->data = NULL;
										}
									}
								}
							}
						}
					}
					goto next;
				}
			}
			Error("unexpected token '%s' on line %i\n", com_token, n); 
		}
	next:
		*s = tempchar;
		if (*s == '\r' || *s == '\n')
			s++;
		n++;
	}
	PacifierEnd();
	if (writingpk3)
	{
		PK3_AddWrappedFiles(pk3);
		PK3_Close(pk3);
		writingpk3 = false;
	}
	mem_free(scriptstring);
}

int Script_Main(int argc, char **argv)
{
	char basepath[MAX_OSPATH];
	bool debugon, oldverbose, oldnoprint;
	int i;

	Verbose("=== Script ===\n");

	// init
	legacycolormaps = (colormaps_t *)mem_alloc(sizeof(colormaps_t));
	legacymodels = (legacymodels_t *)mem_alloc(sizeof(legacymodels_t));
	legacymodelsubs = (legacymodelsubs_t *)mem_alloc(sizeof(legacymodelsubs_t));
	if (argc < 2)
		Error("not enough parms");

	// cmdline
	debugon = false;
	bigfile = NULL;
	bigklist = NULL;
	for (i = 2; i < argc; i++)
	{
		if (!strcmp(argv[i], "-debug"))
		{
			debugon = true;
			continue;
		}
		if (!strcmp(argv[i], "-bigfile"))
		{
			i++;
			if (i < argc)
			{
				if (bigfile)
					Error("-bigfile : bigfile alredy used");
				// load known-files-list, load bigfile
				oldverbose = verbose;
				oldnoprint = noprint;
				if (!debugon)
				{
					verbose = false;
					noprint = true;
				}
				strcpy(bigfilepath, argv[i]);
				bigklist = BigfileLoadKList("klist.txt", false);
				bigfilehandle = SafeOpen(argv[i], "rb");
				bigfile = ReadBigfileHeader(bigfilehandle, false, false);
				verbose = oldverbose;
				noprint = oldnoprint;
			}
			continue;
		}
		if (!strcmp(argv[i], "-path"))
		{
			i++;
			if (i < argc)
				sprintf(basepath, "%s/", argv[i]);
			continue;
		}
	}
	if (!debugon)
	{
		verbose = false;
		noprint = true;
	}

	// parse file
	scriptstarted = I_DoubleTime();
	Script_Parse(argv[1], basepath);

	// deinit
	mem_free(legacycolormaps);
	mem_free(legacymodels);
	mem_free(legacymodelsubs);
	if (bigfilehandle)
		fclose(bigfilehandle);
	if (bigfile)
		FreeBigfileHeader(bigfile);

	return 0;
}