#include "cmdlib.h"

#define BLOODPILL_VERSION "v0.8c"
#define BLOODPILL_WELCOME "----------------------------------------\nBlood Pill %s by VorteX and Mean Person\nA set of utils to manipulate Blood Omen: Legacy Of Kain files\nportions of code by LordHavoc\nAdditional thanks to Balder and Zench from XentaX forums (www.xentax.com)\nTIM specs by Klarth and Raul Sobon\nBigfile specs by Andrey [Rackot] Grachev\nRaw Type 7 (Tiles) specs by Ben Lincoln\n----------------------------------------\n"
#define MAX_BLOODPATH 1024

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
