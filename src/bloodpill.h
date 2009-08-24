#include "cmdlib.h"

#define BLOODPILL_WELCOME	"----------------------------------------\nBlood Pill v0.2 by VorteX\nA set of utils to manipulate Blood Omen: Legacy Of Kain files\nbased on portions of code by LordHavoc\npill.big specs by XentaX community (www.xentax.com)\nTIM specs by Klarth and Raul Sobon\n----------------------------------------\n"

#define MAX_BLOODPATH 512

// global switches
qboolean waitforkey;
qboolean memstats;

char progname[128];

// soxsupp.c
extern qboolean CheckSoX();
extern qboolean RunSoX(char *cmdline);



