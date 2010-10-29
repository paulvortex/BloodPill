// soxsupp.h

// temp files
#define SOXTEMP_IN	"soxin.tmp"
#define SOXTEMP_OUT	"soxout.tmp"

extern qboolean soxfound;

qboolean SoX_Init(char *customsoxpath);

qboolean SoX(char *in, char *generalcmd, char *inputcmd, char *outputcmd, char *out, char *effects);

qboolean SoX_DataToData(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, void **outdataptr, int *outdatabytesptr, char *effects);

qboolean SoX_DataToFile(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, char *outfile, char *effects);

qboolean SoX_FileToData(char *in, char *generalcmd, char *inputcmd, char *outputcmd, int *outdatabytesptr, void **outdataptr, char *effects);

// SoX errors
#define SOXSUPP_ERROR_SOXNOTFOUND		-1
#define SOXSUPP_ERROR_PROCESSFAIL		-2
