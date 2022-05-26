#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

const wchar_t* Char2WChar(const char* in)
{
	if (in == NULL) {
		wchar_t* Out = NULL;
		return Out;
	}
	wchar_t* Out = new wchar_t[strlen(in) + 1];
	mbstowcs_s(NULL, Out, strlen(in) + 1, in, strlen(in));
	return Out;
}

DWORD GetProcId(const wchar_t* procName)
{
	DWORD procId = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(procEntry);

		if (Process32First(hSnap, &procEntry))
		{
			do
			{
				if (!_wcsicmp(procEntry.szExeFile, procName))
				{
					procId = procEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &procEntry));
		}
	}
	CloseHandle(hSnap);
	return procId;
}

bool Inject(HANDLE Process, const char* PATH)
{
    void* addr = VirtualAllocEx(Process, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    WriteProcessMemory(Process, addr, PATH, strlen(PATH) + 1, 0);
    HANDLE hThread = CreateRemoteThread(Process, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, addr, 0, 0);
    if (!hThread)
    {
        VirtualFreeEx(Process, addr, 0, MEM_RELEASE);
        return FALSE;
    }
    CloseHandle(hThread);
    return TRUE;
}


DWORD PID;
HANDLE hProc;
char PROCESS[128]{};
char PATH[128]{};
int exit(const char* Text)
{
	printf(Text);
	Sleep(2000);
	memset(PROCESS, 0, 128);
	memset(PATH, 0, 128);
	return 0;
}
int main()
{
	//	Get Process
	printf("PROCESS: ");
	scanf("%s127\n", PROCESS);

	//	Get Dll Path
	printf("DLL PATH: ");
	scanf("%s127\n", PATH);

	//	Get Proc ID
	PID = GetProcId(Char2WChar(PROCESS));
	if (PID == NULL)
		return exit("PROCESS NOT FOUND\nEXITING . . .\n");
	system("cls");

	//	Open Handle to Process
	hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if (hProc == NULL)
		return exit("COULDN'T OPEN HANDLE TO PROCESS\nEXITING . . .\n");

	//	Debug Print
	printf("SELECTED PROCESS: %s\n", PROCESS);
	printf("SELECTED DLL: %s\n", PATH);
	printf("PID: %d\n", PID);

	//	Inject
	if (!Inject(hProc, PATH))
		return exit("INJECTION FAILED!\n");
	
	//	Exit
	return exit("INJECTION SUCCESS!\n");
}
