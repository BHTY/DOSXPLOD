#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <wchar.h>
#include <tchar.h>

#include <psapi.h>
#include <strsafe.h>
#include <winternl.h>
#include <winnt.h>

#include "ctx386.h"

#define BUFSIZE 512

HMODULE hDebugHelp;

BOOL (WINAPI *pSymInitialize)(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess) = NULL;

VOID TrimName(LPWSTR lpPath, LPSTR lpName) {
	DWORD LastSlash = 0;
	DWORD LastPeriod = 0;
	INT i;

	for (i = 0; lpPath[i] != NULL; i++) {
		if (lpPath[i] == '\\') LastSlash = i + 1;
		if (lpPath[i] == '.') LastPeriod = i;
	}

	WideCharToMultiByte(CP_ACP, 0, lpPath + LastSlash, LastPeriod - LastSlash, lpName, MAX_PATH, NULL, NULL);

	lpName[LastPeriod - LastSlash] = 0;
}

BOOL GetFileNameFromHandle(HANDLE hFile, WCHAR* pszFilename)
{
	BOOL bSuccess = FALSE;
	//TCHAR pszFilename[MAX_PATH + 1];
	HANDLE hFileMap;

	// Get the file size.
	DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

	if (dwFileSizeLo == 0 && dwFileSizeHi == 0)
	{
		_tprintf(TEXT("Cannot map a file with a length of zero.\n"));
		return FALSE;
	}

	// Create a file mapping object.
	hFileMap = CreateFileMapping(hFile,
		NULL,
		PAGE_READONLY,
		0,
		1,
		NULL);

	if (hFileMap)
	{
		// Create a file mapping to get the file name.
		void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

		if (pMem)
		{
			if (GetMappedFileNameW(GetCurrentProcess(),
				pMem,
				pszFilename,
				MAX_PATH))
			{

				// Translate path with device name to drive letters.
				WCHAR szTemp[BUFSIZE];
				szTemp[0] = '\0';

				if (GetLogicalDriveStringsW(BUFSIZE - 1, szTemp))
				{
					WCHAR szName[MAX_PATH];
					WCHAR szDrive[3] = TEXT(" :");
					BOOL bFound = FALSE;
					TCHAR* p = szTemp;

					do
					{
						// Copy the drive letter to the template string
						*szDrive = *p;

						// Look up each device name
						if (QueryDosDeviceW(szDrive, szName, MAX_PATH))
						{
							size_t uNameLen = _tcslen(szName);

							if (uNameLen < MAX_PATH)
							{
								bFound = _wcsnicmp(pszFilename, szName, uNameLen) == 0
									&& *(pszFilename + uNameLen) == _T('\\');

								if (bFound)
								{
									// Reconstruct pszFilename using szTempFile
									// Replace device path with DOS path
									WCHAR szTempFile[MAX_PATH];
									StringCchPrintfW(szTempFile,
										MAX_PATH,
										TEXT("%s%s"),
										szDrive,
										pszFilename + uNameLen);
									StringCchCopyN(pszFilename, MAX_PATH + 1, szTempFile, _tcslen(szTempFile));
								}
							}
						}

						// Go to the next NULL character.
						while (*p++);
					} while (!bFound && *p); // end of string
				}
			}
			bSuccess = TRUE;
			UnmapViewOfFile(pMem);
		}

		CloseHandle(hFileMap);
	}
	//_tprintf(TEXT("File name is %s\n"), pszFilename);
	return(bSuccess);
}

// TRACE386 /pid=abcdefghi
// TRACE386 /wd="" thing.exe commandline

LPWSTR GetProcessCommandLine(int hadCurDir) {
	LPWSTR lpCmdLine = GetCommandLineW();

	return lpCmdLine;
}

HANDLE hProcess;

#pragma pack(push,1)
typedef struct _CoffSymbol {
	char name[4];
	DWORD index;
	DWORD value;
	WORD section_number;
	WORD type;
	BYTE storage_class;
	BYTE number_of_aux_symbols;
} CoffSymbol, * PCoffSymbol;
#pragma pack(pop)

VOID LoadSymbols(HANDLE hFile, PBYTE ImageBase, LPSTR ImageName, LPWSTR ImagePath) { // add PDB symbols
	HANDLE hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	PBYTE pData;
	PIMAGE_DOS_HEADER pDosHdr;
	PIMAGE_NT_HEADERS pNtHdr;
	PIMAGE_DATA_DIRECTORY pDataDir;
	PCoffSymbol pSyms;
	PBYTE pStrTable;
	PBYTE hMod;

	if (!hMapping) {
		printf("(no symbols found)\n");
		return;
	}

	pData = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);

	if (!pData) {
		printf("(no symbols found)\n");
		return;
	}

	pDosHdr = pData;
	pNtHdr = pData + pDosHdr->e_lfanew;
	pDataDir = pNtHdr->OptionalHeader.DataDirectory;
	pSyms = pData + pNtHdr->FileHeader.PointerToSymbolTable;
	pStrTable = (PBYTE)pSyms + pNtHdr->FileHeader.NumberOfSymbols * sizeof(CoffSymbol);

	if (pNtHdr->FileHeader.NumberOfSymbols && pNtHdr->FileHeader.PointerToSymbolTable) {
		int i;
		char* temp_str;
		char string[9];

		printf("(coff symbols)\n");

		for (i = 0; i < pNtHdr->FileHeader.NumberOfSymbols; i++) {
			if (pSyms[i].name[0] == 0) {
				temp_str = malloc(strlen(pStrTable + pSyms[i].index) + 1 + strlen(ImageName) + 1);
				strcpy(temp_str + strlen(ImageName) + 1, pStrTable + pSyms[i].index);
				memcpy(temp_str, ImageName, strlen(ImageName));
				temp_str[strlen(ImageName)] = '!';
			}
			else {
				temp_str = malloc(9 + strlen(ImageName) + 1);
				memcpy(string, pSyms[i].name, 8);
				string[8] = 0;
				strcpy(temp_str + strlen(ImageName) + 1, string);
				memcpy(temp_str, ImageName, strlen(ImageName));
				temp_str[strlen(ImageName)] = '!';
			}

			if ((pSyms[i].storage_class == 3 || pSyms[i].storage_class == 2)) {
				printf("%p: %s\n", ImageBase + pSyms[i].value, temp_str);
				DbgAddSym(0, 0, ImageBase + pSyms[i].value, temp_str);
			}
		}

	}
	else {
		PIMAGE_SECTION_HEADER pSectionHdr = pNtHdr + 1;
		IMAGE_DATA_DIRECTORY exportDataDir = pDataDir[IMAGE_DIRECTORY_ENTRY_EXPORT];

		hMod = LoadLibraryW(ImagePath);

		if (exportDataDir.Size) {
			IMAGE_EXPORT_DIRECTORY* pExportDir = hMod + exportDataDir.VirtualAddress;

			DWORD* exportAddressTable = hMod + pExportDir->AddressOfFunctions;
			WORD* nameOrdinalsPointer = hMod + pExportDir->AddressOfNameOrdinals;
			DWORD* exportNamePointerTable = hMod + pExportDir->AddressOfNames;
			int nameIndex;

			printf("(export symbols)\n");

			//printf("num names: %d | num functions: %d\n", pExportDir->NumberOfFunctions, pExportDir->NumberOfNames);

			for (nameIndex = 0; nameIndex < pExportDir->NumberOfNames; nameIndex++) {
				//printf("%X     ", nameOrdinalsPointer[nameIndex]);

				LPSTR Name = hMod + exportNamePointerTable[nameIndex];
				DWORD Address = ImageBase + exportAddressTable[nameOrdinalsPointer[nameIndex]];

				//printf("%p: %s!%s\n", Address, ImageName, Name);
			}

		}
		else {
			char* temp_str = malloc(strlen(ImageName) + 1);
			DbgAddSym(0, 0, ImageBase, temp_str);
			printf("(no symbols found)\n");

			/* parse section headers */
		}
	}

	UnmapViewOfFile(pData);
	CloseHandle(hMapping);
}

VOID LoadPEImage(LPVOID lpBaseOfImage, HANDLE hFile) {
	WCHAR DllPath[MAX_PATH];
	CHAR DllName[MAX_PATH];

	GetFileNameFromHandle(hFile, DllPath);

	wprintf(L"ModLoad: %p   %s ", lpBaseOfImage, DllPath);

	TrimName(DllPath, DllName);

	LoadSymbols(hFile, lpBaseOfImage, DllName, DllPath);
}

DWORD OnCreateThread(LPDEBUG_EVENT pDebugEvent) {
	// break new thread into debugger?

	printf("Thread Create: Process=%X, Thread=%X\n", pDebugEvent->dwProcessId, pDebugEvent->dwThreadId);

	return DBG_CONTINUE;
}

DWORD OnCreateProcess(LPDEBUG_EVENT pDebugEvent) {
	LoadPEImage(pDebugEvent->u.CreateProcessInfo.lpBaseOfImage, pDebugEvent->u.CreateProcessInfo.hFile);
	return DBG_CONTINUE;
}

DWORD OnLoadDLL(LPDEBUG_EVENT pDebugEvent) {
	LoadPEImage(pDebugEvent->u.LoadDll.lpBaseOfDll, pDebugEvent->u.LoadDll.hFile);
	return DBG_CONTINUE;
}

DWORD OnDebugString(LPDEBUG_EVENT pDebugEvent) {
	PVOID DebugStr;
	BOOLEAN Valid;
	SIZE_T Read;
	
	if (pDebugEvent->u.DebugString.fUnicode) {
		DebugStr = HeapAlloc(GetProcessHeap(), 0, pDebugEvent->u.DebugString.nDebugStringLength * 2 + 2);
		Valid = ReadProcessMemory(hProcess, pDebugEvent->u.DebugString.lpDebugStringData, DebugStr, pDebugEvent->u.DebugString.nDebugStringLength * 2 + 2, &Read);
		if (Valid) wprintf(L"%s", DebugStr);
	}
	else {
		DebugStr = HeapAlloc(GetProcessHeap(), 0, pDebugEvent->u.DebugString.nDebugStringLength + 1);
		Valid = ReadProcessMemory(hProcess, pDebugEvent->u.DebugString.lpDebugStringData, DebugStr, pDebugEvent->u.DebugString.nDebugStringLength + 1, &Read);
		if (Valid) printf("%s", DebugStr);
	}

	HeapFree(GetProcessHeap(), 0, DebugStr);

	return Valid ? DBG_CONTINUE : DBG_CONTINUE;
}

DWORD OnException(LPDEBUG_EVENT pDebugEvent) {
	HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, pDebugEvent->dwThreadId);
	CONTEXT context;
	BOOL bSuccess;

	ctx386 exception_frame;
	error386 error_info;
	
	context.ContextFlags = CONTEXT_ALL;
	bSuccess = GetThreadContext(hThread, &context);

	switch (pDebugEvent->u.Exception.ExceptionRecord.ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
			error_info.error_type = PAGE_FAULT;
			error_info.error_subcode = pDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[1];

			switch (pDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[0]) {
				case 0: // Reading
					break;
					error_info.error_code |= (1 << 1);
				case 1: // Writing
					break;
				case 8: // DEP violation
					error_info.error_code |= (1 << 4);
					break;
			}

			error_info.error_code |= (1 << 2); // this occurred in Ring 3
			break;

		case EXCEPTION_BREAKPOINT:
			error_info.error_type = BREAKPOINT_TRAP;
			break;

		case EXCEPTION_SINGLE_STEP:
			error_info.error_type = DEBUG_EXCEPTION;
			break;

		case DBG_CONTROL_C:
			error_info.error_type = NMI;
			break;

		default:
			printf("Exception 0x%08X\n", pDebugEvent->u.Exception.ExceptionRecord.ExceptionCode);
			return DBG_EXCEPTION_NOT_HANDLED;
	}

	// translate Win32 CONTEXT structure to exception frame
	exception_frame.eip = context.Eip;
	exception_frame.eflags = context.EFlags;

	exception_frame.dr0 = context.Dr0;
	exception_frame.dr1 = context.Dr1;
	exception_frame.dr2 = context.Dr2;
	exception_frame.dr3 = context.Dr3;
	exception_frame.dr6 = context.Dr6;
	exception_frame.dr7 = context.Dr7;

	exception_frame.gs = context.SegGs;
	exception_frame.fs = context.SegFs;
	exception_frame.es = context.SegEs;
	exception_frame.ds = context.SegDs;
	exception_frame.cs = context.SegCs;
	exception_frame.ss = context.SegSs;

	exception_frame.edi = context.Edi;
	exception_frame.esi = context.Esi;
	exception_frame.ebx = context.Ebx;
	exception_frame.edx = context.Edx;
	exception_frame.ecx = context.Ecx;
	exception_frame.eax = context.Eax;
	exception_frame.ebp = context.Ebp;
	exception_frame.esp = context.Esp;

	// run the debugger
	DbgMain(&exception_frame, &error_info);

	// translate exception frame back into Win32 CONTEXT structure
	context.Eip = exception_frame.eip;
	context.EFlags = exception_frame.eflags;

	context.Dr0 = exception_frame.dr0;
	context.Dr1 = exception_frame.dr1;
	context.Dr2 = exception_frame.dr2;
	context.Dr3 = exception_frame.dr3;
	context.Dr6 = exception_frame.dr6;
	context.Dr7 = exception_frame.dr7;

	context.SegGs = exception_frame.gs;
	context.SegFs = exception_frame.fs;
	context.SegEs = exception_frame.es;
	context.SegDs = exception_frame.ds;
	context.SegCs = exception_frame.cs;
	context.SegSs = exception_frame.ss;

	context.Edi = exception_frame.edi;
	context.Esi = exception_frame.esi;
	context.Ebx = exception_frame.ebx;
	context.Edx = exception_frame.edx;
	context.Ecx = exception_frame.ecx;
	context.Eax = exception_frame.eax;
	context.Ebp = exception_frame.ebp;
	context.Esp = exception_frame.esp;

	SetThreadContext(hThread, &context);

	CloseHandle(hThread);
	
	return DBG_CONTINUE;
}

void DebugLoop() { // CTRL-C as hotkey
	DEBUG_EVENT debugEvent;
	DWORD dwContinueStatus;

	while (1) {
		WaitForDebugEvent(&debugEvent, INFINITE);

		dwContinueStatus = DBG_CONTINUE;

		switch (debugEvent.dwDebugEventCode) {
			case CREATE_PROCESS_DEBUG_EVENT:
				dwContinueStatus = OnCreateProcess(&debugEvent);
				break;

			case CREATE_THREAD_DEBUG_EVENT:
				dwContinueStatus = OnCreateThread(&debugEvent);
				
				break;

			case EXIT_PROCESS_DEBUG_EVENT:
				printf("Process Exit: Code 0x%08X\n", debugEvent.u.ExitProcess.dwExitCode);
				break;
			
			case EXIT_THREAD_DEBUG_EVENT:
				printf("Thread Exit: Thread=%X, Code=0x%08X\n", debugEvent.dwThreadId, debugEvent.u.ExitThread.dwExitCode);
				break;
			
			case LOAD_DLL_DEBUG_EVENT:
				dwContinueStatus = OnLoadDLL(&debugEvent);
				break;
			
			case UNLOAD_DLL_DEBUG_EVENT:
				break;
			
			case OUTPUT_DEBUG_STRING_EVENT:
				dwContinueStatus = OnDebugString(&debugEvent);
				break;
			
			case EXCEPTION_DEBUG_EVENT:
				dwContinueStatus = OnException(&debugEvent);
				break;
		}

		ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, dwContinueStatus);
	}
}

// Stubs
uint32_t GetCr0() { return 1; }
uint32_t GetCr3() { return 0; }

int wmain(int argc, wchar_t** argv) {
	hDebugHelp = LoadLibraryA("DBGHELP.DLL");

	if (hDebugHelp) {
		pSymInitialize = GetProcAddress(hDebugHelp, "SymInitialize");
	}

	// Process command-line arguments
	if (argc == 1) {
		printf("TRACENT.EXE /pid=12345789 (for debugging existing process)\n");
		printf("TRACENT.EXE [/wd=PATH] filename.exe commandline (for debugging new process - current directory inherited if not specified)\n");
		return 0;
	}
	
	if (wmemcmp(L"/pid=", argv[1], 5) == 0) {
		DWORD pid = _wtoi(argv[1] + 5);
		printf("I would open PID %d, but I don't support that yet\n", pid);
	}
	else {
		LPWSTR exePath;
		WCHAR curDir[MAX_PATH];
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		int hadCurDir = 0;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		if (wmemcmp(L"/wd=", argv[1], 4) == 0) {
			int i, p;
			p = 0;

			for (i = 4; i < wcslen(argv[1]); i++) {
				if (argv[1][i] != L'"') {
					curDir[p++] = argv[1][i];
				}
			}

			curDir[p] = 0;
			hadCurDir = 1;
			exePath = argv[2];
		}
		else {
			GetCurrentDirectoryW(MAX_PATH, curDir);
			exePath = argv[1];
		}

		wprintf(L"TRACE386 Debugger v1.0 for Windows NT\n");
		wprintf(L"CommandLine: %s\n", GetProcessCommandLine(hadCurDir));
		CreateProcessW(exePath, GetProcessCommandLine(hadCurDir), NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE, NULL, curDir, &si, &pi);
		hProcess = pi.hProcess;

		pSymInitialize(hProcess, NULL, FALSE);
	}

	DebugLoop();

	return 0;
}