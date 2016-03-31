// bloodpill.h
#ifndef F_BLOODPILL_H
#define F_BLOODPILL_H

// dependencies
#include "cmdlib.h"
#include "dll.h"
#include "mem.h"
#include <vector>
#include <math.h>
using namespace std;

#ifndef BLOODPILL_VERSION
#define BLOODPILL_VERSION "v1.1"
static char *BLOODPILL_WELCOME =
"-------------------------------------------------------------------------------\n"
" Blood Pill %s by Pavel [VorteX] Timofeyev\n"
"-------------------------------------------------------------------------------\n"
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

#endif