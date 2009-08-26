// soxsupp.h

extern qboolean soxfound;

qboolean SoX_Init(char *customsoxpath);

qboolean SoX(char *in, char *generalcmd, char *inputcmd, char *outputcmd, char *out);

qboolean SoX_DataToData(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, int *outdatabytesptr, void **outdataptr);

qboolean SoX_DataToFile(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, char *outfile);

qboolean SoX_FileToData(char *in, char *generalcmd, char *inputcmd, char *outputcmd, int *outdatabytesptr, void **outdataptr);

// SoX errors
#define SOXSUPP_ERROR_SOXNOTFOUND		-1
#define SOXSUPP_ERROR_PROCESSFAIL		-2
