#include "cmdlib.h"
#include "dll.h"
#include <vector>
#include "math.h"

using namespace std;

#ifndef BLOODPILL_VERSION
#define BLOODPILL_VERSION "v1.04"
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
#define MAX_OSPATH 1024
#endif

// global switches
extern bool waitforkey;
extern bool error_waitforkey;
extern bool memstats;
extern bool verbose;
extern bool noprint;
extern bool solidpacifier;
extern bool errorlog;

extern char progname[MAX_OSPATH];
extern char progpath[MAX_OSPATH];

// printing
void Print (char *str, ...);
void Verbose (char *str, ...);
void Warning (char *str, ...);
void Pacifier(char *str, ...);
void PercentPacifier(char *str, ...);
void PacifierEnd();
