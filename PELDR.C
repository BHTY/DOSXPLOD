#include "types.h"
#include "../doscalls.h"
#include "../dpmi.h"
#include "exe.h"

typedef DWORD SYSRESULT;

SYSRESULT SysLoadLibrary(CHAR* pszLibName, PVOID* pvModule) {
    SYSRESULT sysRes;
    IMAGE_DOS_HEADER dosHdr;
    IMAGE_NT_HEADERS ntHdr;
    ULONG ulRead;
    INT i;
    BYTE* pModule;

    HFILE hFile;
    HMEMBLOCK hMemBlock;
    DWORD dwDelta;

    /* Open the image file */
    if (DosOpen(pszLibName, FILE_READ, &hFile)) {
        return io error;
    }

    /* Read in the DOS MZ EXE header */
    if (DosRead(hFile, &dosHdr, sizeof(dosHdr), &ulRead) || ulRead < sizeof(dosHdr) || /* Read in the MZ header */
        dosHdr.e_magic != MZ_MAGIC || /* Is it a valid MZ EXE? */
        DosSetFilePtr(hFile, dosHdr.e_lfanew, SEEK_SET, &ulRead) || ulRead != dosHdr.e_lfanew || /* Did we successfully seek to the PE header? */
        DosRead(hFile, &ntHdr, sizeof(ntHdr), &ulRead) || ulRead < sizeof(ntHdr) || /* And read the PE header */
        ntHdr.Signature != PE_MAGIC) { /* Is it a PE image? */
        sysRes = bad format;
        goto done;
    }

    /* Make sure the image is targeted to the Intel 80386 (our current machine) */
    if (ntHdr.FileHeader.Machine != IMAGE_MACHINE_TYPE_I386) {
        sysRes = bad machine;
        goto done;
    }

    /* Allocate the memory block to store the image in memory */
    if (DpmiMemAlloc(ntHdr.OptionalHeader.SizeOfImage, (BYTE**)&pModule, &hMemBlock)) {
        sysRes = not enough memory;
        goto done;
    }

    dwDelta = (DWORD)(pModule) - ntHdr.OptionalHeader.ImageBase; /* Calculate delta between desired image base and loaded location */

    /* If the image needs to be relocated, make sure that a relocation data directory exists */
    if (dwDelta && (ntHdr.FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)) {
        DpmiMemFree(hMemBlock);
        sysRes = relocs;
        goto done;
    }

    /* Load each section into memory */
    for (i = 0; i < ntHdr.FileHeader.NumberOfSections; i++) {
        IMAGE_SECTION_HEADER secHdr;
        DWORD dwOffset;

        if (DosRead(hFile, &secHdr, sizeof(secHdr), &ulRead) || /* Read current section header*/
            DosSetFilePtr(hFile, 0, SEEK_CUR, &dwOffset) || /* Save current file offset */
            DosSetFilePtr(hFile, secHdr.PointerToRawData, SEEK_SET, &ulRead) || /* And reposition to point to section data */
            DosRead(hFile, pModule + secHdr.VirtualAddress, secHdr.SizeOfRawData, &ulRead) || /* And read the section data */
            DosSetFilePtr(hFile, dwOffset, SEEK_SET, &ulRead)) /* And point back to the next section header */
        {
            DpmiMemFree(hMemBlock);
            goto done;
        }

        /* there's still just a little bit more to do */
    }

    /* Parse relocations */
    if (dwDelta) {
        PIMAGE_DATA_DIRECTORY pRelocDataDir = &(ntHdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
        PIMAGE_BASE_RELOCATION pBaseReloc = pModule + pRelocDataDir->VirtualAddress;

        while (pBaseReloc->VirtualAddress) {

        }
    }

    /* Insert into loader list */

    /* Parse imports */

    /* Call entry point */

    sysRes = 0;
    *pvModule = pModule;

    done: /* All done, cleanup */
        DosClose(hFile);
        return sysRes;
}