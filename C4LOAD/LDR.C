/**
 *      File: LDR.C
 *      Internal routines for executable loader
 *      Copyright (c) 2025 by Will Klees
 */

#include <TYPES.H>
#include <DOSCALLS.H>
#include <EXE.H>
#include <DOSXPLOD.H>
#include <I386INS.H>
#include <LDR.H>

PLDR_LIST_ENTRY LoaderList = NULL;

/**
 *  LdrTrimPath procedure - Traverses a path to remove any path separators
 *  and return only the filename.
 * 
 *  @param pszPath: A pointer to a null-terminated string containing the
 *  path.
 * 
 *  @return: A pointer to the trimmed file name. Note that this is NOT a
 *  newly allocated string independent from pszPath; it is a string 
 *  pointing INSIDE of pszPath.
 */
CHAR* LdrTrimPath(CHAR* pszPath) {
    CHAR* LastSlash = pszPath;
    DWORD i;
    for (i = 0; pszPath[i] != 0; i++) {
        if (pszPath[i] == '\\') LastSlash = &(pszPath[i+1]);
    }
    return LastSlash;
}

/**
 *  LdrAllocEntry procedure - Allocates a new loader list entry.
 * 
 *  @return: A pointer to a new loader list entry if one is available, or
 *  NULL if one is not.
 */
PLDR_LIST_ENTRY LdrAllocEntry() {
    return SysMemAlloc(sizeof(LDR_LIST_ENTRY));
}

/**
 *  LdrFreeEntry procedure - Releases a loader list entry, returning it to the
 *  pool.
 */
void            LdrFreeEntry(PLDR_LIST_ENTRY pLdrListEntry) {
    SysMemFree(pLdrListEntry);
}

/**
 *  LdrFindEntry procedure - Searches the loader list for an entry with a
 *  matching name.
 * 
 *  @param pszLibName: A pointer to a null-terminated string containing the
 *  name of the module.
 * 
 *  @return: A pointer to the matching loader list entry if found, or NULL.
 */
PLDR_LIST_ENTRY LdrFindEntry(CHAR* pszLibName) {
    PLDR_LIST_ENTRY pLdrListEntry = LoaderList;
    if (pszLibName == NULL) return LoaderList; /* Return first entry if the input is NULL */
    while (pLdrListEntry) {
        if (strnicmp(pszLibName, LdrTrimPath(pLdrListEntry->DllName), DLL_NAME_SIZE) == 0) return pLdrListEntry;
        pLdrListEntry = pLdrListEntry->Next;
    }
    return NULL;
}

/**
 *  LdrFindEntryByBase procedure - Searches the loader list for an entry with a
 *  matching base address.
 * 
 *  @param DllBase: The base address of the module.
 * 
 *  @return: A pointer to the matching loader list entry if found, or NULL.
 */
PLDR_LIST_ENTRY LdrFindEntryByBase(DWORD DllBase) {
    PLDR_LIST_ENTRY pLdrListEntry = LoaderList;
    if (DllBase == 0) return LoaderList; /* Return first entry if the input is NULL */
    while (pLdrListEntry) {
        if (pLdrListEntry->DllBase == DllBase) return pLdrListEntry;
        pLdrListEntry = pLdrListEntry->Next;
    }
    return NULL;
}

/**
 *  LdrAddEntry procedure - Adds a new entry to the loader list.
 * 
 *  @param pszLibName: A pointer to a null-terminated string containing the
 *  name of the module.
 * 
 *  @param DllBase: The base address of the module.
 * 
 *  @return: A system status code, SYSERR_SUCCESS if successful or
 *      SYSERR_INSUFFICIENT_MEMORY
 */
SYSRESULT       LdrAddEntry(CHAR* pszLibName, DWORD DllBase) {
    /* Allocate the new loader list entry */
    PLDR_LIST_ENTRY pLdrListEntry = LdrAllocEntry();
    if (pLdrListEntry == NULL) return SYSERR_INSUFFICIENT_MEMORY;

    /* Initialize the fields of this new entry */
    pLdrListEntry->Next = NULL;
    pLdrListEntry->DllBase = DllBase;
    pLdrListEntry->RefCount = 1;
    strncpy(pLdrListEntry->DllName, LdrTrimPath(pszLibName), DLL_NAME_SIZE);

    if (LoaderList == NULL) { /* This is the first entry */
        pLdrListEntry->Prev = NULL;
        LoaderList = pLdrListEntry;
    } else {
        PLDR_LIST_ENTRY pListEnd = LoaderList;

        /* Walk to the end of the list such that pListEnd = the last entry */
        while (pListEnd->Next) {
            pListEnd = pListEnd->Next;
        }

        /* And insert it into the list */
        pListEnd->Next = pLdrListEntry;
        pLdrListEntry->Prev = pListEnd;
    }

    return SYSERR_SUCCESS;
}

/**
 *  LdrRemoveEntry procedure - Removes an entry from the loader list.
 * 
 *  @param pLdrListEntry: A pointer to the loader list entry to remove.
 */
void            LdrRemoveEntry(PLDR_LIST_ENTRY pLdrListEntry) {
    return;

    if (pLdrListEntry == LoaderList) { /* We're removing the first entry */
        LoaderList = pLdrListEntry->Next;
        LoaderList->Prev = NULL;
    } else {
        PLDR_LIST_ENTRY pPrev = pLdrListEntry->Prev;
        PLDR_LIST_ENTRY pNext = pLdrListEntry->Next;

        if (pPrev) pPrev->Next = pNext;
        if (pNext) pNext->Prev = pPrev;
    }

    LdrFreeEntry(pLdrListEntry);
}

/**
 *  LdrWriteSections procedure - Loads each of the COFF sections in the
 *  image file into memory.
 *  
 *  @param pModule: A pointer to the base of the module.
 * 
 *  @param hFile: An MS-DOS file handle to the disk file containing the image.
 * 
 *  @return: A system status code, SYSERR_SUCCESS if successful, or
 *      SYSERR_IO_ERROR
 */
SYSRESULT       LdrWriteSections(PVOID pModule, HFILE hFile) {
    INT i;
    ULONG ulRead;
    PIMAGE_SECTION_HEADER pSecHdr = LdrGetSections(pModule);
    DOSSTATUS dosStatus;

    for (i = 0; i < LdrGetFileHeader(pModule)->NumberOfSections; i++) {
        PVOID pvAddr = (PBYTE)pModule + pSecHdr[i].VirtualAddress;

        /*
        printf("%s\n", pSecHdr[i].Name);
        printf("\tPhysical Size:    %p\n", pSecHdr[i].SizeOfRawData);
        printf("\tVirtual Size:     %p\n", pSecHdr[i].Misc.VirtualSize);
        printf("\tPhysical Address: %p\n", pSecHdr[i].PointerToRawData);
        printf("\tVirtual Address:  %p\n", pSecHdr[i].VirtualAddress);
        */

        if (pSecHdr[i].Characteristics & 0x80) continue;

        if ((dosStatus = DosSetFilePtr(hFile, pSecHdr[i].PointerToRawData, SEEK_SET, &ulRead)) || /* Seek to the section data... */
            ulRead != pSecHdr[i].PointerToRawData ||
            (dosStatus = DosRead(hFile, pvAddr, pSecHdr[i].SizeOfRawData, &ulRead)) || ulRead < pSecHdr[i].SizeOfRawData /* And read it in */
        ) {
            /* Clean up the resources */
            DosClose(hFile);
            SysMemFree(pModule);
            /*printf("Error %x (%p)\n", dosStatus, ulRead);*/
            return SYSERR_IO_ERROR;
        }
    }

    return SYSERR_SUCCESS;
}

/**
 *  LdrWriteRelocs procedure - Processes the module's relocation table,
 *  fixing up any references to reflect the location where it's been loaded.
 * 
 *  @param pModule: A pointer to the base of the module.
 * 
 *  @param dwDelta: The delta between the address where the module was loaded
 *  and its desired image base address.
 * 
 *  @return: A system status code; SYSERR_SUCCESS if successful, or
 *      SYSERR_IMG_RELOCS: The relocation table is missing
 *      SYSERR_IMG_BAD_RELOC_TYPE: The relocation table is corrupt
 */
SYSRESULT       LdrWriteRelocs(PVOID pModule, DWORD dwDelta) {
    PIMAGE_DATA_DIRECTORY pRelocDataDir = LdrDataDir(pModule, IMAGE_DIRECTORY_ENTRY_BASERELOC);
    PIMAGE_BASE_RELOCATION pBaseReloc = (PBYTE)pModule + pRelocDataDir->VirtualAddress;

    /* Fail if there's no relocation table */
    if (pRelocDataDir->Size == 0) return SYSERR_IMG_RELOCS;

    while (pBaseReloc->VirtualAddress) {
        WORD *pwReloc = (WORD*)(pBaseReloc+1);
        DWORD dwNumBlocks = ((pBaseReloc->SizeOfBlock) - sizeof(IMAGE_BASE_RELOCATION)) / 2;
        INT i;

        for (i = 0; i < dwNumBlocks; i++) {
            /* First 4 bits is type, last 12 bits is offset */
            DWORD *patch_addr = (PBYTE)pModule + pBaseReloc->VirtualAddress + (pwReloc[i] & 0xFFF);

            switch (pwReloc[i] >> 12) { /* Apply the relocation type */
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;
                case IMAGE_REL_BASED_HIGHLOW:
                    *patch_addr += dwDelta;
                    break;
                default:
                    /*printf("Type %d\n", pwReloc[i] >> 12);*/
                    return SYSERR_IMG_BAD_RELOC_TYPE;
            }   
        }

        pBaseReloc = (PBYTE)(pBaseReloc) + pBaseReloc->SizeOfBlock;
    }

    return SYSERR_SUCCESS;
}

/**
 *  LdrResolveImports procedure - Resolves all imports in a module,
 *  populating the import table with the addresses of each entry point.
 * 
 *  @param pModule: A pointer to the base of the module.
 * 
 *  @return: A system status code, SYSERR_SUCCESS if successful, or
 *      SYSERR_IMG_MISSING_DEPENDENCY: An imported module could not be loaded
 *      SYSERR_IMG_MISSING_IMPORT: An imported entry point could not be found
 */
SYSRESULT       LdrResolveImports(PVOID pModule) {
    PIMAGE_DATA_DIRECTORY pImportDataDir = LdrDataDir(pModule, IMAGE_DIRECTORY_ENTRY_IMPORT);
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PBYTE)pModule + pImportDataDir->VirtualAddress;

    /* If there's no imports, just finish up */
    if (pImportDataDir->Size == 0) return SYSERR_SUCCESS;

    /* Start processing each imported DLL */
    while (pImportDesc->DUMMYUNIONNAME.OriginalFirstThunk) {
        PIMAGE_THUNK_DATA HINT_TABLE, IAT_TABLE;

        /* Load the imported DLL */
        PVOID pLibrary;
        CHAR* pszName = (PBYTE)pModule + pImportDesc->Name;
        SYSRESULT sysRes = SysLoadLibrary(pszName, &pLibrary);
        if (sysRes) {
            printf("The module %s could not be found.\n", pszName);
            return SYSERR_IMG_MISSING_DEPENDENCY;
        }

        HINT_TABLE = (PBYTE)pModule + pImportDesc->DUMMYUNIONNAME.OriginalFirstThunk;
        IAT_TABLE = (PBYTE)pModule + pImportDesc->FirstThunk;

        /* Process each imported function from that DLL */
        for (; HINT_TABLE->u1.AddressOfData; HINT_TABLE++, IAT_TABLE++) {
            DWORD fncAddr = HINT_TABLE->u1.AddressOfData;
            PVOID ProcAddr;

            if (fncAddr & IMAGE_ORDINAL_FLAG) { /* Import by ordinal */
                DWORD dwOrdinal = fncAddr & ~IMAGE_ORDINAL_FLAG;
                ProcAddr = SysGetProcAddress(pLibrary, (CHAR*)(dwOrdinal));

                if (ProcAddr == NULL) {
                    SysLogError("The ordinal %d could not be located in the dynamic link library %s.\n", dwOrdinal, pszName);
                    return SYSERR_IMG_MISSING_IMPORT;
                }

            } else { /* Import by name */
                PIMAGE_IMPORT_BY_NAME byName = (PBYTE)pModule + fncAddr;
                ProcAddr = SysGetProcAddress(pLibrary, &(byName->Name));

                if (ProcAddr == NULL) {
                    SysLogError("The procedure entry point %s could not be located in the dynamic link library %s.", &(byName->Name), pszName);
                    return SYSERR_IMG_MISSING_IMPORT;
                }
            }

            IAT_TABLE->u1.Function = ProcAddr;
        }

        pImportDesc++;
    }

    return SYSERR_SUCCESS;
}

/**
 *  LdrOpenPE procedure - Opens a Portable Executable (PE) file, loading the
 *  headers into memory.
 * 
 *  @param pszLibName: A pointer to a null-terminated string containing the
 *  path name of the library to load.
 * 
 *  @param pvModule: A pointer to receive the base address of the module,
 *  if the call is successful.
 * 
 *  @param phFile: A pointer to receive a handle to the image file
 * 
 *  @return: A system status code, SYSERR_SUCCESS if successful, or
 *      SYSERR_IMG_MISSING: The file is not found
 *      SYSERR_IO_ERROR: An I/O error prevented opening or reading the file
 *      SYSERR_IMG_FORMAT: The executable is not a valid PE
 *      SYSERR_IMG_MACHINE_TYPE: The executable is not targeted to the i386
 *      SYSERR_INSUFFICIENT_MEMORY: Not enough memory to load the executable
 */
SYSRESULT LdrOpenPE(CHAR* pszLibName, PVOID* pvModule, HFILE* phFile) {
    DOSSTATUS dosRes;
    SYSRESULT sysRes;
    IMAGE_DOS_HEADER dosHdr;
    IMAGE_NT_HEADERS ntHdr;
    ULONG ulRead;

    /* Open the image file */
    if (dosRes = DosOpen(pszLibName, FILE_READ, phFile)) {
        switch (dosRes) {
            case DOS_FILE_NOT_FOUND:
                return SYSERR_IMG_MISSING;
            default:
                return SYSERR_IO_ERROR;
        }
    }

    /* Read in the DOS MZ EXE header */
    if (DosRead(*phFile, &dosHdr, sizeof(dosHdr), &ulRead) || ulRead < sizeof(dosHdr) || /* Read in the MZ header */
        dosHdr.e_magic != MZ_MAGIC || /* Is it a valid MZ EXE? */
        DosSetFilePtr(*phFile, dosHdr.e_lfanew, SEEK_SET, &ulRead) || ulRead != dosHdr.e_lfanew || /* Did we successfully seek to the PE header? */
        DosRead(*phFile, &ntHdr, sizeof(ntHdr), &ulRead) || ulRead < sizeof(ntHdr) || /* And read the PE header */
        ntHdr.Signature != PE_MAGIC) { /* Is it a PE image? */
        sysRes = SYSERR_IMG_FORMAT;
        goto error;
    }

    /* Make sure that the image is targeted to the Intel 80386 */
    if (ntHdr.FileHeader.Machine != IMAGE_MACHINE_TYPE_I386) {
        sysRes = SYSERR_IMG_MACHINE_TYPE;
        goto error;
    }

    /* Allocate the memory block to store the image in memory */
    *pvModule = SysMemAlloc(ntHdr.OptionalHeader.SizeOfImage);
    if (*pvModule == 0) {
        sysRes = SYSERR_INSUFFICIENT_MEMORY;
        goto error;
    }
    stosb(*pvModule, 0, ntHdr.OptionalHeader.SizeOfImage);

    /* Load the headers into memory */
    if (DosSetFilePtr(*phFile, 0, SEEK_SET, &ulRead) || ulRead != 0) {
        sysRes = SYSERR_IO_ERROR;
        SysMemFree(*pvModule);
        goto error;
    }

    if (DosRead(*phFile, *pvModule, ntHdr.OptionalHeader.SizeOfHeaders, &ulRead) || 
        ulRead < ntHdr.OptionalHeader.SizeOfHeaders) {
        sysRes = SYSERR_IO_ERROR;
        SysMemFree(*pvModule);
        goto error;
    }

    return SYSERR_SUCCESS;

    error:
        DosClose(*phFile);
        return sysRes;
}
