// SoX support
// pretty hacky yet
// need a proper soxlib support

#include "bloodpill.h"
#include "cmdlib.h"

#ifdef WIN32
#include "windows.h"
#include <shellapi.h> 
#pragma comment(lib, "shell32.lib") 
#endif

qboolean soxfound = false;

qboolean CheckSoX()
{
#ifdef WIN32
	soxfound = false;
	if (!FileExists("sox.exe"))
		return soxfound;
	soxfound = true;
#endif
	return soxfound;
}

qboolean RunSoX(char *cmdline)
{	
#ifdef WIN32
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	DWORD exitcode = 0; 
	char cmd[2048];

	if (!soxfound)
		Error("attempt to run SoX, but util is not presented");

	sprintf(cmd, "sox.exe %s", cmdline);

	// run procress
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(PROCESS_INFORMATION)); 
	if (!CreateProcess(NULL, cmd, NULL, NULL, false, 0, NULL, NULL, &si, &pi))
	{
		Error("SoX Execution Error(%s): #%i", cmd, GetLastError());
		return false;
	}
	exitcode = WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
	/*
	SHELLEXECUTEINFO shell;
	shell.cbSize = sizeof(SHELLEXECUTEINFO);
	shell.fMask = 0x00000100;
	shell.lpFile = "sox.exe";
	shell.lpParameters = cmdline;
	shell.lpVerb = NULL;
	shell.nShow = SW_HIDE;
	if (!ShellExecuteEx(&shell))
	{
		Error("Shell execure error: #%i", GetLastError());
		return false;
	}
	return true;
	*/
#endif
}