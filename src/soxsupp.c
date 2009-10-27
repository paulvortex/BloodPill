// SoX support
// pretty hacky yet
// need a proper soxlib support

#include "bloodpill.h"
#include "cmdlib.h"
#include "mem.h"
#include "soxsupp.h"

#ifdef WIN32
#include "windows.h"
#include <shellapi.h> 
#pragma comment(lib, "shell32.lib") 
#endif

qboolean soxfound = false;
char soxpath[MAX_BLOODPATH];

/*
==========================================================================================

  General SoX support

==========================================================================================
*/

qboolean SoX_Init(char *pathtoexe)
{
#ifdef WIN32
	soxfound = false;
	sprintf(soxpath, "%s", pathtoexe);
	if (!FileExists(soxpath))
	{
		// try search sox/
		sprintf(soxpath, "sox\\%s", pathtoexe);
		if (!FileExists(soxpath))
			soxfound = false;
		else
			soxfound = true;
	}
	else
		soxfound = true;
#else
	soxfound = false;
#endif
	return soxfound;
}

// runs SoX on files presented, returs TRUE if succesful
// Error("SoX Execution Error: #%i", GetLastError());
qboolean SoX(char *in, char *generalcmd, char *inputcmd, char *outputcmd, char *out)
{
#ifdef WIN32
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	DWORD exitcode = 0; 
	char cmd[2048];

	if (!soxfound)
	{
		SetLastError(SOXSUPP_ERROR_SOXNOTFOUND);
		return false;
	}

	// run progress
	GetStartupInfo(&si);
	sprintf(cmd, "%s %s %s \"%s\" %s \"%s\"", soxpath, generalcmd, inputcmd, in, outputcmd, out);
	//Print("cmd %s\n", cmd);
	memset(&pi, 0, sizeof(PROCESS_INFORMATION)); 
	if (!CreateProcess(NULL, cmd, NULL, NULL, false, 0, NULL, NULL, &si, &pi))
		return false;
	exitcode = WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
#else
	SetLastError(SOXSUPP_ERROR_PROCESSFAIL);
	return false;
#endif
}

// runs SoX on data presented and allocates output data
qboolean SoX_DataToData(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, int *outdatabytesptr, void **outdataptr)
{	
	char in[MAX_BLOODPATH], out[MAX_BLOODPATH], *outdata;
	int outdatabytes;
	qboolean sox;
	FILE *f;

	strcpy(out, "soxout.tmp");
	//tmpnam(out);

	// make input
	strcpy(in, "soxin.tmp");
	//tmpnam(in);
	SaveFile(in, data, databytes);

	// run
	sox = SoX(in, generalcmd, inputcmd, outputcmd, out);
	if (!sox)
		return false;

	// read contents of out file
	f = SafeOpen(out, "rb");
	outdatabytes = Q_filelength(f);
	outdata = qmalloc(outdatabytes);
	fread(outdata, outdatabytes, 1, f);
	*outdataptr = outdata;
	*outdatabytesptr = outdatabytes;
	fclose(f);

	// remove tempfiles
	remove(in);
	remove(out);

	return true;
}

// runs SoX on data presented and saves output file
qboolean SoX_DataToFile(byte *data, int databytes, char *generalcmd, char *inputcmd, char *outputcmd, char *outfile)
{	
	char in[MAX_BLOODPATH];
	qboolean sox;

	// make input
	strcpy(in, "soxin.tmp");
	//tmpnam(in);
	strcpy(in, "soxin.tmp");
	SaveFile(in, data, databytes);

	// run
	sox = SoX(in, generalcmd, inputcmd, outputcmd, outfile);
	if (!sox)
		return false;

	// remove tempfiles
	remove(in);

	return true;
}

// runs SoX on file and allocates output data
qboolean SoX_FileToData(char *in, char *generalcmd, char *inputcmd, char *outputcmd, int *outdatabytesptr, void **outdataptr)
{	
	char out[MAX_BLOODPATH], *outdata;
	int outdatabytes;
	qboolean sox;
	FILE *f;

	strcpy(out, "soxout.tmp");
	//tmpnam(out);

	// run
	sox = SoX(in, generalcmd, inputcmd, outputcmd, out);
	if (!sox)
		return false;

	// read contents of out file
	f = SafeOpen(out, "rb");
	outdatabytes = Q_filelength(f);
	outdata = qmalloc(outdatabytes);
	fread(outdata, outdatabytes, 1, f);
	*outdataptr = outdata;
	*outdatabytesptr = outdatabytes;
	fclose(f);

	remove(out);

	return true;
}

/*
==========================================================================================

  Vag2Wav converter

==========================================================================================
*/

int VagConvert_Main(int argc, char **argv)
{
	char filename[MAX_BLOODPATH], outfile[MAX_BLOODPATH], basefilename[MAX_BLOODPATH], ext[15];
	char inputcmd[2048], outputcmd[2048], *c;
	int rate, i = 1;
	double vorbisquality;
	qboolean wavpcm;

	Print("=== VagConvert ==\n");
	if (i < 1)
		Error("not enough parms");

	// get inner file
	strcpy(filename, argv[i]);
	StripFileExtension(filename, basefilename);
	ExtractFileExtension(filename, ext);
	Q_strlower(ext);
	i++;

	// get out file
	sprintf(outfile, "%s.wav", basefilename); 
	if (i < argc)
	{
		c = argv[i];
		if (c[0] != '-')
			strcpy(outfile, c);
	}

	// parse cmdline
	sprintf(outputcmd, "-t wav");
	wavpcm = false;
	rate = 22050;
	for (; i < argc; i++)
	{
		if (!strcmp(argv[i], "-pcm"))
		{
			wavpcm = true;
			Print("Option: export 16-bit PCM RIFF WAVE\n");
			sprintf(outputcmd, "-t wav -e signed-integer");
		}
		else if (!strcmp(argv[i], "-oggvorbis"))
		{
			i++;
			vorbisquality = 5.0f;
			if (i < argc)
				vorbisquality = atof(argv[i]);
			if (vorbisquality > 10.0f)
				vorbisquality = 10.0f;
			if (vorbisquality < 0.0f)
				vorbisquality = 0.0f;
			Print("Option: export Ogg Vorbis (quality %f)\n", vorbisquality);
			sprintf(outputcmd, "-t ogg -C %f", vorbisquality);
		}
		else if (!strcmp(argv[i], "-custom"))
		{
			i++;
			if (i < argc)
				sprintf(outputcmd, "%s", argv[i]);
			Print("Option: custom SoX commandline (%s)\n", outputcmd);
		}
		else if (!strcmp(argv[i], "-rate"))
		{
			i++;
			if (i < argc)
				rate = atoi(argv[i]);
			Print("Option: rate %ihz\n", rate);
		}
	}

	// build outputcmd
	if (!strcmp(ext, "vag"))
		sprintf(inputcmd, "-t ima -r %i -c 1", rate);
	else
	{
		Warning("file %s is not a VAG file, detecting type automatically\n", outputcmd);
		sprintf(inputcmd, "");
	}

	// run SOX
	Print("conversion in progress...\n");
	if (!SoX(filename, "", inputcmd, outputcmd, outfile))
		Error("SoX Error: #%i", GetLastError());
	Print("done.\n");
	return 0;
}
