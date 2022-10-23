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

BOOL IsGameStarted(wchar_t* ProcName, DWORD& ProcessID)
{
	// SNAPSHOT RUNNING PROCESSES
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

	//	LOOP PROCESSES UNTIL MATCH IS FOUND
	do
	{
		//	COMPARE PROCESS NAMES
		if (!_wcsicmp(procEntry.szExeFile, ProcName))
		{
			//	MATCH FOUND
			ProcessID = procEntry.th32ProcessID;
			CloseHandle(hSnap);
			return TRUE;
		}

	} while (Process32Next(hSnap, &procEntry));
	CloseHandle(hSnap);
	return FALSE;
}

BOOL Initialize_InjectionModule(Data Info)
{
	//	WAIT FOR PROCESS
	while (!IsGameStarted(Info.Process, Info.ProcID))
		Sleep(50);

	//	OBTAIN OPEN HANDLE TO PROCESS
	Info.hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Info.ProcID);
	if (!Info.hProc)
	{
		DWORD errorCode = GetLastError();
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (OpenProcess)", NULL);
		return FALSE;
	}

	//	LOAD LIBRARY
	void* addr = VirtualAllocEx(Info.hProc, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	std::string ABS_PATH = GetCurrentPath() + "\\" + Info.DLL;		// Absolute paths only. Do not ask me why. Im fucking done. jesus fucking christ
	if (!WriteProcessMemory(Info.hProc, addr, ABS_PATH.c_str(), strlen(ABS_PATH.c_str()) + 1, NULL))
	{
		DWORD errorCode = GetLastError();
		CloseHandle(Info.hProc);
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (WriteProcessMemory)", NULL);
		return FALSE;
	}

	//	INJECT DLL INTO PROCESS
	HANDLE hThread = CreateRemoteThread(Info.hProc, NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryA), addr, NULL, NULL);
	if (!hThread)
	{
		DWORD errorCode = GetLastError();
		VirtualFreeEx(Info.hProc, addr, NULL, MEM_RELEASE);
		CloseHandle(Info.hProc);
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (CreateRemoteThread)", NULL);
		return FALSE;
	}

	//	INJECTION SUCCESS, CLEANUP
	CloseHandle(hThread);
	CloseHandle(Info.hProc);
	return TRUE;
}

Data GetData()
{
	Data PData;
	char buff[0x1024];
	memset(buff, NULL, sizeof(buff));

	//	OBTAIN HANDLE TO FILE SO WE CAN READ THE CONTENTS (or create file if it is not yet made)
	auto fHandle = CreateFileA(CONFIG_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	//	ERROR HANDLING
	if (fHandle == INVALID_HANDLE_VALUE) 
	{
		DWORD errorCode = GetLastError();
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (LOAD ini)", NULL);
		memset(buff, 0, sizeof(buff));
		exit(errorCode);
	}

	//	READ CONTENTS OF FILE
	DWORD amount = NULL;
	bool status = ReadFile(fHandle, buff, sizeof(buff), &amount, NULL);
	if (!status) 
	{
		DWORD errorCode = GetLastError();
		MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (READ ini)", NULL);
		memset(buff, 0, sizeof(buff));
		exit(errorCode);
	}
	amount = NULL;
	status = FALSE;
	CloseHandle(fHandle);

	//	CHECK FILE CONTENTS
	int empty{ 0 };	// NULL CHARS
	int whitespace{ 0 };	// SPACE CHARS
	for (int i = 0; i < sizeof(buff); i++)
	{
		if (buff[i] == '\0')
			empty++;

		if (buff[i] == ' ')
			whitespace++;
	}

	// CHECK IF DOCUMENT IS EMPTY
	if (empty == sizeof(buff))
	{
		char data[0x256];
		const char* data2 = "PLEASE DOUBLE CHECK CONFIG\n\nDATA: [%s]\n(ERROR: config was empty)\n\nPROCESS WILL NOW EXIT";
		sprintf_s(data, data2, buff);
		MessageBoxA(NULL, data, "ERROR (READ ini)", NULL);
		empty = NULL;
		whitespace = NULL;
		memset(buff, 0, sizeof(buff));
		memset(data, 0, sizeof(data));
		exit(EXIT_FAILURE);
	}

	// CHECK FOR EMPTY SPACES
	if (whitespace > NULL)
	{
		char data[0x256];
		const char* data2 = "POTENTIAL INFINITE LOOP AVERTED\nPLEASE DOUBLE CHECK CONFIG\n\nDATA: [%s]\n(ERROR: empty spaces are not allowed)\n\nPROCESS WILL NOW EXIT";
		sprintf_s(data, data2, buff);
		MessageBoxA(NULL, data, "ERROR (READ ini)", NULL);
		empty = NULL;
		whitespace = NULL;
		memset(buff, 0, sizeof(buff));
		memset(data, 0, sizeof(data));
		exit(EXIT_FAILURE);
	}

	// MANUALLY LOOP THROUGH CHAR ARRAY FOR PROCESS & DLL NAMES ... im sure there is probably a function for this somewhere . . .
	bool bKey{ 0 };
	std::string ProcessName;
	for (int i = 0; i < sizeof(buff); i++)
	{
		if (!bKey) {
			if (buff[i] == ',') {
				bKey = TRUE;
				continue;
			}
			ProcessName += buff[i];
		}
		else
			PData.DLL += buff[i];
	}
	memset(buff, NULL, sizeof(buff));

	// CONVERT PROCESS NAME TO WIDE CHAR
	const char* ProcName_cBuf = ProcessName.c_str();
	wchar_t* NewWideString = new wchar_t[strlen(ProcName_cBuf) + 1];
	mbstowcs_s(NULL, NewWideString, (strlen(ProcName_cBuf) + 1), ProcName_cBuf, (strlen(ProcName_cBuf) + 1));
	PData.Process = NewWideString;

	/// VERIFY THAT DLL EXISTS
	{
		//	(we are doing the very same thing we did in the beginning, same variables are being used)
		std::string PATH = ".\\" + PData.DLL;
		unsigned long szFile = GetFileSize(PATH);					// Get File Size
		unsigned char* moduleDataBuffer = (unsigned char*)malloc(szFile);		// Allocate size
		fHandle = CreateFileA(PATH.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fHandle == INVALID_HANDLE_VALUE)
		{
			DWORD errorCode = GetLastError();
			MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (LOAD DLL)", NULL);
			memset(buff, 0, sizeof(buff));
			exit(errorCode);
		}

		//	READ CONTENTS OF FILE 
		status = ReadFile(fHandle, moduleDataBuffer, szFile, &amount, NULL);
		if (!status)
		{
			DWORD errorCode = GetLastError();
			MessageBoxA(NULL, ParseError(errorCode).c_str(), "ERROR (READ DLL)", NULL);
			memset(buff, 0, sizeof(buff));
			exit(errorCode);
		}
		amount = NULL;
		status = FALSE;
		CloseHandle(fHandle);

		// CHECK FIRST 2 BYTES FOR MAGIC NUMBER
		if (moduleDataBuffer[0] != 'M' && moduleDataBuffer[1] != 'Z') {
			// THIS MEANS THE FILE WAS CREATED BY OUR PROCESS, INJECTING THIS WILL DO NOTHING
			const char* data = "INPUT DLL FILE NOT FOUND";
			MessageBoxA(NULL, data, "ERROR (DLL NOT FOUND)", NULL);
			memset(buff, 0, sizeof(buff));
			DeleteFileA(PATH.c_str());		//	DELETE FILE SINCE WE CREATED IT
			exit(EXIT_FAILURE);
		}

		// Since all the bytes are in the buffer, this is a perfect opportunity to map the dll by other means. Perhaps a future update can expand upon this
		// For now, we have simply verified that the dll is not empty and that is good enough to inject it. Its on the user at this point to understand what they are doing.
		free(moduleDataBuffer);
	}

	// RETURN WITH DATA STRUCT
	return PData;
}