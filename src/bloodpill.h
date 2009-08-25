#include "cmdlib.h"

#define BLOODPILL_WELCOME	"----------------------------------------\nBlood Pill v0.2 by VorteX\nA set of utils to manipulate Blood Omen: Legacy Of Kain files\nportions of code by LordHavoc\npill.big specs by XentaX community (www.xentax.com)\nTIM specs by Klarth and Raul Sobon\n----------------------------------------\n"

#define MAX_BLOODPATH 512

// global switches
qboolean waitforkey;
qboolean memstats;

extern char progname[128];

// soxsupp.c
qboolean CheckSoX();

qboolean SoX(char *in, char *generalcmd, char *inputcmd, char *outputcmd, char *out);

qboolean SoX_DataToData(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, int *outdatabytesptr, byte **outdataptr);

qboolean SoX_DataToFile(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, char *outfile);

qboolean SoX_FileToData(char *in, char *generalcmd, char *inputcmd, char *outputcmd, int *outdatabytesptr, byte **outdataptr);



