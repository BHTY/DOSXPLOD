/**
 *      File: SYSLDR.C
 *      Exported routines for executable loader
 *      Copyright (c) 2025 by Will Klees
 */


#include <TYPES.H>
#include <DOSCALLS.H>
#include <EXE.H>
#include <DOSXPLOD.H>
#include <I386INS.H>
#include <LDR.H>

/**
 *  SysLoadLibrary procedure - Loads the specified module into the address
 *  space of the calling process. The specified module may cause other
 *  modules to be loaded. If the specified module is a DLL that is not
 *  already loaded for the calling process, the system calls the DLL's
 *  DllMain function with the DLL_PROCESS_ATTACH value. if DllMain returns
 *  TRUE, SysLoadLibrary returns a pointer to the module. If DllMain returns
 *  FALSE, the system unloads the DLL from the process address space and
 *  SysLoadLibrary returns SYSERR_IMG_ENTRY_FAILED.
 * 
 *  @param pszLibName: A pointer to a null-terminated string containing the
 *  name of the module. If the function cannot find the module, the function
 *  fails.
 * 
 *  @param pvModule: A pointer to receive the base address of the module, if
 *  successful.
 * 
 *  @return: A system status code, SYSERR_SUCCESS if successful or
 *      SYSERR_IMG_MISSING: The file is not found
 *      SYSERR_IO_ERROR: An I/O error prevented opening or reading the file
 *      SYSERR_IMG_FORMAT: The executable is not a valid PE
 *      SYSERR_IMG_MACHINE_TYPE: The executable is not targeted to the i386
 *      SYSERR_INSUFFICIENT_MEMORY: Not enough memory to load the executable
 *      SYSERR_IMG_RELOCS: The image requires relocations but they're missing
 *      SYSERR_IMG_ENTRY_FAILED: The entry point of the module returned FALSE
 *      SYSERR_IMG_MISSING_DEPENDENCY: An imported module could not be loaded
 *      SYSERR_IMG_MISSING_IMPORT: An imported entry point could not be found
 *      SYSERR_IMG_BAD_RELOC_TYPE: A corrupt relocation table was found
 */
SYSRESULT SysLoadLibrary(CHAR* pszLibName, PVOID* pvModule) {
    HFILE hFile;
    SYSRESULT sysRes;
    PLDR_LIST_ENTRY pLdrListEntry;
    DWORD dwDelta;

    /* Check if it's already loaded */
    if (pLdrListEntry = LdrFindEntry(pszLibName)) {
        *pvModule = pLdrListEntry->DllBase;
        pLdrListEntry->RefCount++;
        return SYSERR_SUCCESS;
    }

    /* If not, start loading it */
    if (sysRes = LdrOpenPE(pszLibName, pvModule, &hFile)) return sysRes;

    /* Write sections */
    if (sysRes = LdrWriteSections(*pvModule, hFile)) return sysRes;
    DosClose(hFile);

    /* Write relocations */
    dwDelta = (DWORD)(*pvModule) - LdrGetOptionalHeader(*pvModule)->ImageBase;
    if (dwDelta) {
        if (LdrGetFileHeader(*pvModule)->Characteristics & IMAGE_FILE_RELOCS_STRIPPED) {
            sysRes = SYSERR_IMG_RELOCS;
            goto error;
        } else {
            if (sysRes = LdrWriteRelocs(*pvModule, dwDelta)) {
                goto error;
            }
        }
    }

    /* Insert into loader list */
    if (sysRes = LdrAddEntry(pszLibName, *pvModule)) goto error;

    /* Resolve imports */
    if (sysRes = LdrResolveImports(*pvModule)) {
        SysMemFree(*pvModule);
        LdrRemoveEntry(pLdrListEntry);
        return sysRes;
    }

    /* Call entry point */
    if (LdrGetFileHeader(*pvModule)->Characteristics & IMAGE_FILE_DLL) {
        PDLLMAIN pDllEntry = (PBYTE)(*pvModule) + LdrGetOptionalHeader(*pvModule)->AddressOfEntryPoint;

        if (!pDllEntry(*pvModule, DLL_PROCESS_ATTACH, 0)) {
            sysRes = SYSERR_IMG_ENTRY_FAILED;
            SysMemFree(*pvModule);
            LdrRemoveEntry(pLdrListEntry);
        }
    }

    return sysRes;

    error:
        SysMemFree(*pvModule);
        return sysRes;

}

/**
 *  SysFreeLibrary procedure - Frees the loaded dynamic-link library module and,
 *  if necessary, decrements its reference count. When the reference count 
 *  reaches zero, the module is unloaded from the address space of the calling
 *  process and the pointer is no longer valid. When loaded, a module has a
 *  reference count of one. The reference count is incremented each time the 
 *  module is loaded by a call to SysLoadLibrary, and decremented each time the
 *  SysFreeLibrary function is called for that module. When the module's
 *  reference count reaches zero, the system enables the module to detach from
 *  the process by calling the module's DllMain function with the 
 *  DLL_PROCESS_DETACH value. Doing so gives the library module an opportunity to
 *  clean up resources allocated on behalf of the current process. After the
 *  entry-point function returns, the library module is removed from the address
 *  space of the current process.
 * 
 *  @param pModule: A pointer to the base of the module to unload.
 * 
 *  @return: If the function succeeds, the return value is nonzero. If the 
 *  function fails, the return value is zero.
 */
BOOL      SysFreeLibrary(PVOID pModule) {
    PLDR_LIST_ENTRY pLdrListEntry = LdrFindEntryByBase(pModule);

    if (pLdrListEntry) {
        pLdrListEntry->RefCount--;

        if (pLdrListEntry->RefCount == 0) {
            PDLLMAIN pDllEntry = (PBYTE)(pModule) + LdrGetOptionalHeader(pModule)->AddressOfEntryPoint;
            pDllEntry(pModule, DLL_PROCESS_DETACH, 0);
            SysMemFree(pModule);
            LdrRemoveEntry(pLdrListEntry);
        }

        return TRUE;
    }

    return FALSE;
}

/**
 *  SysGetModuleHandle procedure - Retrieves a module handle for the specified
 *  module. The module must have been loaded by the calling process.
 * 
 *  @param pszModuleName: Pointer to a null-terminated string containing the
 *  name of the module. If this parameter is NULL, SysGetModuleHandle retrieves
 *  the base address of the executable file of the current process.
 * 
 *  @return: The base address of the DLL module corresponding to the provided
 *  name if one such is loaded, or NULL otherwise.
 */
PVOID     SysGetModuleHandle(CHAR* pszModuleName) {
    PLDR_LIST_ENTRY pLdrListEntry = LdrFindEntry(pszModuleName);

    if (pLdrListEntry) {
        return pLdrListEntry->DllBase;
    }

    return NULL;
}

/**
 *  SysGetModuleFileName procedure - Retrieves the filename for the file that
 *  contains the specified module.
 * 
 *  @param pModule: A pointer to the base of the loaded module whose path is
 *  being requested. If this paremeter is NULL, SysGetModuleFileName retrieves
 *  the filename of the executable file of the current process.
 * 
 *  @return: A pointer to the file name of the desired image if successful,
 *  and NULL if it's not loaded. Note that this pointer SHOULD NOT be written
 *  to; it is valid ONLY for reading.
 */
PCHAR     SysGetModuleFileName(PVOID pModule) {
    PLDR_LIST_ENTRY pLdrListEntry = LdrFindEntryByBase(pModule);

    if (pLdrListEntry) {
        return pLdrListEntry->DllName;
    }

    return NULL;
}

/**
 *  SysGetProcAddress procedure - Retrieves the address of an exported function
 *  or variable from the specified dynamic-link library.
 * 
 *  @param pModule: The base address of the DLL module.
 * 
 *  @param pszProcName: The function or variable name, or the function's ordinal
 *  value. If this parameter is an ordinal value, it must be in the low-order
 *  word; the high-order word must be zero.
 * 
 *  @return: If the function succeeds, the return value is the address of the
 *  exported function or variable. If the function fails, the return value is
 *  NULL.
 */
PVOID     SysGetProcAddress(PVOID pModule, CHAR* pszProcName) {
    PIMAGE_NT_HEADERS pNtHdr = (CHAR*)pModule + ((PIMAGE_DOS_HEADER)pModule)->e_lfanew;
    PIMAGE_OPTIONAL_HEADER pOptHdr = &(pNtHdr->OptionalHeader);
    PIMAGE_DATA_DIRECTORY pExportDataDir = &(pOptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    PIMAGE_EXPORT_DIRECTORY pExportDir;
    DWORD* pExportAddressTable;
    WORD* pNameOrdinalsPointer;
    DWORD* pExportNamePointerTable;
    INT i;

    /* Check to make sure that there's actually an export data directory */
    if (pExportDataDir->Size == 0) {
        return 0;
    }

    pExportDir = (CHAR*)pModule + pExportDataDir->VirtualAddress;
    pExportAddressTable = (CHAR*)pModule + pExportDir->AddressOfFunctions;
    pNameOrdinalsPointer = (CHAR*)pModule + pExportDir->AddressOfNameOrdinals;
    pExportNamePointerTable = (CHAR*)pModule + pExportDir->AddressOfNames;

    /* IMAGE_ORDINAL_FLAG */

    for (i = 0; i < pExportDir->NumberOfNames; i++) {
        WORD wOrdinalIndex = pNameOrdinalsPointer[i];
        BYTE* pProcName = (CHAR*)pModule + pExportNamePointerTable[i];
        BYTE* fncAddress = (CHAR*)pModule + pExportAddressTable[wOrdinalIndex];

        if (0) { /* forwarded export */

        }

        if ((DWORD)pszProcName > 0xFFFF && strcmp(pszProcName, pProcName) == 0) {
            return fncAddress;
        } else if ((WORD)pszProcName == wOrdinalIndex + 1) {
            return fncAddress;
        }
    }

    return NULL;
}

