#pragma once
#include "../pch.h"
#include "tools.h"
#define CONFIG_PATH ".\\engine_config.ini"
using namespace Tools;

typedef struct {
	DWORD ProcID;
	HANDLE hProc;
	wchar_t* Process;
	std::string DLL;
} Data;

__forceinline BOOL IsGameStarted(wchar_t* ProcName, DWORD& ProcessID)
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
		return FALSE;

	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(procEntry);
	if (!Process32Next(hSnap, &procEntry))
	{
		CloseHandle(hSnap);
		return FALSE;
	}

	do
	{
		if (!_wcsicmp(procEntry.szExeFile, ProcName))
		{
			ProcessID = procEntry.th32ProcessID;
			CloseHandle(hSnap);
			return TRUE;
		}

	} while (Process32Next(hSnap, &procEntry));
	CloseHandle(hSnap);
	return FALSE;
}

static bool isRunning{ 0 };
__forceinline BOOL Initialize_InjectionModule(Data Info)
{
	// Wait for Process
	while (!isRunning)
		isRunning = IsGameStarted(Info.Process, Info.ProcID);

	//	OBTAIN OPEN HANDLE TO PROCESS
	Info.hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Info.ProcID);
	if (!Info.hProc)
	{
		DWORD errorCode = GetLastError();
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (OpenProcess)\nProcess will now exit.", NULL);
		return FALSE;
	}

	//	LOAD LIBRARY
	void* addr = VirtualAllocEx(Info.hProc, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	std::string ABS_PATH = GetCurrentPath() + "\\" + Info.DLL;		// Absolute paths only. Do not ask me why. Im fucking done. jesus fucking christ
	if (!WriteProcessMemory(Info.hProc, addr, ABS_PATH.c_str(), strlen(ABS_PATH.c_str()) + 1, NULL))
	{
		DWORD errorCode = GetLastError();
		CloseHandle(Info.hProc);
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (WriteProcessMemory)\nProcess will now exit.", NULL);
		return FALSE;
	}

	//	INJECT DLL INTO PROCESS
	HANDLE hThread = CreateRemoteThread(Info.hProc, NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryA), addr, NULL, NULL);
	if (!hThread)
	{
		DWORD errorCode = GetLastError();
		VirtualFreeEx(Info.hProc, addr, NULL, MEM_RELEASE);
		CloseHandle(Info.hProc);
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (CreateRemoteThread)\nProcess will now exit.", NULL);
		return FALSE;
	}

	//	INJECTION SUCCESS, CLEANUP
	CloseHandle(hThread);
	CloseHandle(Info.hProc);
	return TRUE;
}

__forceinline Data GetData()
{
	Data PData;
	char buff[0x1024];
	memset(buff, NULL, sizeof(buff));
	auto fHandle = CreateFileA(CONFIG_PATH,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (fHandle == INVALID_HANDLE_VALUE) 
	{
		DWORD errorCode = GetLastError();
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (LOAD ini)\nProcess will now exit.", NULL);
		exit(errorCode);
	}

	// Read contents of file
	DWORD amount = 0;
	bool status = ReadFile(fHandle, buff, sizeof(buff), &amount, NULL);
	if (!status) 
	{
		DWORD errorCode = GetLastError();
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (READ ini)\nProcess will now exit.", NULL);
		exit(errorCode);
	}
	CloseHandle(fHandle);

	if (buff[0] == '\0') 
	{
		const char* data = "Potential infinite loop averted, please double check config\nProcess will now exit.";
		MessageBoxA(NULL, data, "ERROR (READ ini)", NULL);
		exit(EXIT_FAILURE);
	}
	
	// Manually Loop through char array ... im sure there is probably a function for this somewhere . . .
	std::string ProcessName;
	for (int i = 0; i < sizeof(buff); i++)
	{
		if (buff[i] == ',') 
			break;
		ProcessName += buff[i];
	}

	// Get Dll name the very same way we get process name
	bool FOUND = FALSE;
	for (int i = 0; i < sizeof(buff); i++)
	{
		if (!FOUND)
			if (buff[i] != ',') continue;	// Loop through char array until we reach our key identifier
		
		if (buff[i] == ',')
		{
			FOUND = TRUE;		// We trip our boolean so that future characters will not be hung up by our key identifier
			continue;			// Skip
		}

		PData.DLL += buff[i];		//	Append string
	}

	// Get our Process name to WideChar
	const char* ProcName_cBuf = ProcessName.c_str();
	wchar_t* NewWideString = new wchar_t[strlen(ProcName_cBuf) + 1];
	mbstowcs_s(NULL, NewWideString, (strlen(ProcName_cBuf) + 1), ProcName_cBuf, (strlen(ProcName_cBuf) + 1));
	PData.Process = NewWideString;

	// Return with our parsed data struct
	return PData;
}