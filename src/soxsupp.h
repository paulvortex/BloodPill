// soxsupp.h

extern bool soxfound;

bool SoX_Init(char *customsoxpath);

bool SoX(char *in, char *generalcmd, char *inputcmd, char *outputcmd, char *out, char *effects);

bool SoX_DataToData(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, byte **outdataptr, int *outdatabytesptr, char *effects);

bool SoX_DataToFile(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, char *outfile, char *effects);

bool SoX_FileToData(char *in, char *generalcmd, char *inputcmd, char *outputcmd, int *outdatabytesptr, byte **outdataptr, char *effects);

// SoX errors
#define SOXSUPP_ERROR_SOXNOTFOUND		-1
#define SOXSUPP_ERROR_PROCESSFAIL		-2
