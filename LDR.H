/**
 *      File: LDR.H
 *      Definitions for executable loader
 *      Copyright (c) 2025 by Will Klees
 */

#ifndef __LDR_H_
#define __LDR_H_

#include <TYPES.H>
#include <DOSCALLS.H>
#include <EXE.H>

#define DLL_NAME_SIZE 256

typedef struct _LDR_LIST_ENTRY {
    struct _LDR_LIST_ENTRY* Next;
    struct _LDR_LIST_ENTRY* Prev;
    DWORD DllBase;
    DWORD RefCount;
    CHAR  DllName[DLL_NAME_SIZE];
} LDR_LIST_ENTRY, *PLDR_LIST_ENTRY;

extern PLDR_LIST_ENTRY LoaderList;

typedef BOOL (__stdcall *PDLLMAIN)(PVOID hinstDLL, DWORD fdwReason, PVOID pvReserved);

/* Functions that affect the loader list */
PLDR_LIST_ENTRY LdrAllocEntry();
void            LdrFreeEntry(PLDR_LIST_ENTRY pLdrListEntry);
PLDR_LIST_ENTRY LdrFindEntry(CHAR* pszLibName);
PLDR_LIST_ENTRY LdrFindEntryByBase(DWORD DllBase);
SYSRESULT       LdrAddEntry(CHAR* pszLibName, DWORD DllBase);
void            LdrRemoveEntry(PLDR_LIST_ENTRY pLdrListEntry);

/* Functions that load images */
SYSRESULT       LdrWriteSections(PVOID pModule, HFILE hFile);
SYSRESULT       LdrWriteRelocs(PVOID pModule, DWORD dwDelta);
SYSRESULT       LdrResolveImports(PVOID pModule);
SYSRESULT       LdrOpenPE(CHAR* pszLibName, PVOID* pvModule, HFILE* phFile);

/* Useful macros */
#define LdrGetDosHeader(ImageBase)          ((PIMAGE_DOS_HEADER)(ImageBase))
#define LdrGetNtHeader(ImageBase)           ((PIMAGE_NT_HEADERS)((PBYTE)(ImageBase) + LdrGetDosHeader(ImageBase)->e_lfanew))
#define LdrGetFileHeader(ImageBase)         ((PIMAGE_FILE_HEADER)(&(LdrGetNtHeader(ImageBase)->FileHeader)))
#define LdrGetOptionalHeader(ImageBase)     ((PIMAGE_OPTIONAL_HEADER)(&(LdrGetNtHeader(ImageBase)->OptionalHeader)))
#define LdrGetSections(ImageBase)           ((PIMAGE_SECTION_HEADER)(LdrGetNtHeader(ImageBase) + 1))
#define LdrDataDir(ImageBase, Index)        ((PIMAGE_DATA_DIRECTORY)&(LdrGetOptionalHeader(ImageBase)->DataDirectory[Index]))

#endif
