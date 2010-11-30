#include "cmdlib.h"
#include "dll.h"

#ifndef BLOODPILL_VERSION
#define BLOODPILL_VERSION "v0.8c"
static char *BLOODPILL_WELCOME =
"----------------------------------------\n"
"Blood Pill %s by VorteX and Mean Person\n"
"A set of utils to manipulate Blood Omen: Legacy Of Kain files\n"
"portions of code by LordHavoc\n"
"Additional thanks to Balder and Zench from XentaX forums (www.xentax.com)\n"
"TIM specs by Klarth and Raul Sobon\n"
"Bigfile specs by Andrey [Rackot] Grachev\n"
"Raw Type 7 (LZW Tiles) specs by Ben Lincoln\n"
"JAM decoder by MisterGrim\n"
"----------------------------------------\n"
;
#define MAX_BLOODPATH 512
#endif

// global switches
qboolean waitforkey;
qboolean memstats;
qboolean verbose;
qboolean noprint;

extern char progname[128];

// printing
void Print (char *str, ...);
void Verbose (char *str, ...);
void Warning (char *str, ...);
void Pacifier(char *str, ...);
void PacifierEnd();
