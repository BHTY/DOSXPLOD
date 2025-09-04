#include <stdio.h>
#include <string.h>
#include <DOSCALLS.H>
#include <DPMI.H>
#include <DOSXPLOD.H>
#include <EXE.H>
#include <I386INS.H>
#include <LDR.H>
#include <biocalls.h>

int DbgLoadSymbol(DWORD dwPointer, HFILE hFile, INT i, PIMAGE_COFF_SYMBOL pSym) {
    ULONG ulRead;
    DWORD dwDestPointer = dwPointer + i * sizeof(IMAGE_COFF_SYMBOL);

    if (DosSetFilePtr(hFile, dwDestPointer, SEEK_SET, &ulRead) || /* Seek to the symbol */
        ulRead != dwDestPointer ||
        DosRead(hFile, pSym, sizeof(IMAGE_COFF_SYMBOL), &ulRead) || ulRead < sizeof(IMAGE_COFF_SYMBOL) /* Read in the symbol */
    ) {
        return 1;
    }

    return 0;
}

int DbgLoadString(PIMAGE_FILE_HEADER pFileHdr, DWORD dwOffset, HFILE hFile, CHAR* psz) {
    DWORD offsStrTabStart = pFileHdr->PointerToSymbolTable + pFileHdr->NumberOfSymbols * sizeof(IMAGE_COFF_SYMBOL);
    ULONG ulRead;

    /* Seek to the string */
    if (DosSetFilePtr(hFile, dwOffset + offsStrTabStart, SEEK_SET, &ulRead) || 
        ulRead != dwOffset + offsStrTabStart
    ) {
        return 1;
    }

    /* Read the string in, one byte at a time */
    do {
        if (DosRead(hFile, psz, 1, &ulRead)) return 1;
    } while (*(psz++));

    return 0;
}

int DbgLoadSymbols(CHAR* pszImageName, PVOID pModule) {
    IMAGE_COFF_SYMBOL coffSym;
    PIMAGE_FILE_HEADER pFileHdr = LdrGetFileHeader(pModule);
    INT i;
    HFILE hFile;
    CHAR szSymName[256];

    /* Return immediately if there's no symbols */
    if (pFileHdr->PointerToSymbolTable == 0 || pFileHdr->NumberOfSymbols == 0) return 1;

    /* Try to open the file */
    if (DosOpen(pszImageName, FILE_READ, &hFile)) return 1;

    /* Read in each symbol */
    for (i = 0; i < pFileHdr->NumberOfSymbols; i++) {
        if (DbgLoadSymbol(pFileHdr->PointerToSymbolTable, hFile, i, &coffSym)) return 1;

        if (coffSym.Name.Table.Zeroes == 0) { /* Read a string from the string table */
            if (DbgLoadString(pFileHdr, coffSym.Name.Table.Offset, hFile, szSymName)) return 1;
        } else {
            memcpy(szSymName, coffSym.Name.Name, 8);
            szSymName[8] = 0; /* Null-terminate */
        }

        printf("%p: %s\n", coffSym.Value, szSymName);
    }

    return 0;
}

/* Install exception handlers */

typedef int (*EXEEntry)();

SYSRESULT LaunchEXE(CHAR* pszExeName, DWORD* pdwRes) {
    EXEEntry pExeEntry;
    PIMAGE_OPTIONAL_HEADER pOptHdr;
    PIMAGE_FILE_HEADER pFileHdr;
    PVOID pModule;
    SYSRESULT sysRes = SysLoadLibrary(pszExeName, &pModule);

    if (sysRes) {
        return sysRes;
    }

    pOptHdr = LdrGetOptionalHeader(pModule);
    pFileHdr = LdrGetFileHeader(pModule);

    if (pFileHdr->Characteristics & IMAGE_FILE_DLL) {
        return SYSERR_NOT_EXE;
    }

    pExeEntry = (PBYTE)pModule + pOptHdr->AddressOfEntryPoint;

    *pdwRes = pExeEntry();
    
    //DbgLoadSymbols(pszExeName, pModule);

    return SYSERR_SUCCESS;
}


void LdrPrintError(SYSRESULT sysRes, CHAR* pszLibName) {
    switch (sysRes) {
        case SYSERR_INSUFFICIENT_MEMORY:
            printf("ERROR: There was insufficient memory to load the image file %s.\n", pszLibName);
            break;
        case SYSERR_IO_ERROR:
            printf("ERROR: An I/O error occured while trying to load the image file %s.\n", pszLibName);
            break;
        case SYSERR_IMG_FORMAT:
            printf("ERROR: The file %s is not a valid executable.\n", pszLibName);
            break;
        case SYSERR_IMG_MACHINE_TYPE:
            printf("ERROR: The image file %s is valid, but for a machine type other than the current machine.\n", pszLibName);
            break;
        case SYSERR_IMG_RELOCS:
        case SYSERR_IMG_BAD_RELOC_TYPE:
            printf("ERROR: Valid relocations were not found in the image file %s.\n", pszLibName);
            break;
        case SYSERR_IMG_ENTRY_FAILED:
            printf("ERROR: The module %s failed to initialize.\n", pszLibName);
            break;
        case SYSERR_IMG_MISSING_DEPENDENCY:
            printf("ERROR: A module imported by the image file %s could not be loaded.\n", pszLibName);
            break;
        case SYSERR_IMG_MISSING_IMPORT:
            printf("ERROR: An entry point imported by the image file %s could not be loaded.\n", pszLibName);
            break;
        case SYSERR_NOT_EXE:
            printf("ERROR: The image file %s is not executable.\n", pszLibName);
            break;
        case SYSERR_SUCCESS:
            break;
        default:
            printf("ERROR: The image file %s failed to load (error %d).\n", sysRes);
            break;
    }
}

void SetHandlers();

void aprintf(char* str, ...) {
    while (*str) {
        char c = *(str++);

        __asm {
            mov ah, 0eh
            mov al, c
            int 10h
        }

        if (c == '\n') {
            __asm {
                mov ah, 0eh
                mov al, 0dh
                int 10h
            }
        }
    }
}

// Base=00000000  Limit=00000000  CODE32  Ring3\n"); /* NOT PRESENT or SYSTEM */

void PrintSelector(PBYTE pszSelName, WORD wSel) {
    SEG_DESC desc;

    SysLogError("%s=  %04X  ", pszSelName, wSel);

    if (DpmiGetDescriptor(wSel, (BYTE*)&desc) || !SEG_PRESENT(desc)) {
        SysLogError("NOT PRESENT\n\r");
        return;
    } else if (!SEG_TYPE(desc)) {
        SysLogError("SYSTEM\n\r");
    }

    SysLogError("Base=%08X  Limit=%08X  %s%d  Ring%d\n\r", SEG_BASE(desc), SEG_LIMIT(desc),
        SEG_EXEC(desc) ? "CODE" : "DATA", SEG_SIZE(desc) ? 32 : 16, SEG_DPL(desc));
}

void cdecl ExceptionPrint(PEXCEPT_CONTEXT pContext) {
    PLDR_LIST_ENTRY pListEntry = LoaderList;
    BYTE cException = pContext->ExceptionNumber;
    
    __asm {
        mov ah, 0
        mov al, 3
        int 10h
    }

    //pContext = (DWORD)pContext + 0xd0010;

    //return;

    SysLogError("CRITICAL ERROR - Exception %02Xh at %04X:%08X\n\r", cException, pContext->CS, pContext->EIP);

    switch (cException) {
        case 0:
            aprintf("Divide Fault\n");
            break;
        case 1:
            aprintf("Unhandled Debug Exception\n");
            break;
        case 2:
            aprintf("Unhandled NMI\n");
            break;
        case 3:
            aprintf("Unhandled Breakpoint Trap\n");
            break;
        case 4:
            aprintf("Overflow Trap\n");
            break;
        case 5:
            aprintf("Bound Fault\n");
            break;
        case 6:
            aprintf("Invalid Opcode Fault\n");
            break;
        case 7:
            aprintf("Device Not Available Fault\n");
            break;
        case 8:
            aprintf("Double Fault\n");
            break;
        case 0xA:
            SysLogError("Invalid TSS Fault: Sel=%04Xh\n\r", pContext->ErrorCode);
            break;
        case 0xB:
            SysLogError("Segment Not Present Fault: Sel=%04Xh\n\r", pContext->ErrorCode);
            break;
        case 0xC:
            SysLogError("Stack Segment Fault: Sel=%04Xh\n\r", pContext->ErrorCode);
            break;
        case 0xD:
            SysLogError("General Protection Fault: Sel=%04Xh\n\r", pContext->ErrorCode);
            break;
        case 0xE:
            SysLogError("Page Fault %s %08Xh %s\n\r", (pContext->ErrorCode & 2) ? "Writing" : "Reading",
                getCR2(), (pContext->ErrorCode & 1) ? "" : "(NP)");
            break;
        default:
            break;
    }

#define FLAG(mask,set,clear)    ((pContext->EFLAGS & (1 << mask)) ? set : clear)
    /* Print further exception info (varies depending on type) */
    SysLogError("EFLAGS= %08X [%s %s %s %s %s %s %s %s]  CS:[EIP] = %02X %02X %02X %02X %02X %02X %02X %02X\n\r",
        pContext->EFLAGS, 
        FLAG(I386_FLAG_OV,"OV","NV"), FLAG(I386_FLAG_DIR,"DN","UP"), 
        FLAG(I386_FLAG_IE,"EI","DI"), FLAG(I386_FLAG_SIGN,"NG","PL"), 
        FLAG(I386_FLAG_ZR,"ZR","NZ"), FLAG(I386_FLAG_AC,"AC","NA"), 
        FLAG(I386_FLAG_PF,"PE","PO"), FLAG(I386_FLAG_CY,"CY","NC"), 
        readcsbyte(pContext->EIP), readcsbyte(pContext->EIP+1), readcsbyte(pContext->EIP+2), readcsbyte(pContext->EIP+3), 
        readcsbyte(pContext->EIP+4), readcsbyte(pContext->EIP+5), readcsbyte(pContext->EIP+6), readcsbyte(pContext->EIP+7));
#undef FLAG
    SysLogError("EAX= %08X  ESI= %08X  IOPL=%d CPL=%d      SS:[ESP+00]= %08X %08X\n\r",
        pContext->EAX, pContext->ESI, (pContext->EFLAGS & 0x3000) >> 12, pContext->CS & 3, readssdword(pContext->ESP), readssdword(pContext->ESP + 4));
    SysLogError("EBX= %08X  EDI= %08X  MODE=%s         SS:[ESP+08]= %08X %08X\n\r",
        pContext->EBX, pContext->EDI, 
        (pContext->EFLAGS & 0x20000) ? "VM86" : "PROT",
        readssdword(pContext->ESP + 8), readssdword(pContext->ESP + 0xc));
    SysLogError("ECX= %08X  EBP= %08X                    SS:[ESP+10]= %08X %08X\n\r",
        pContext->ECX, pContext->EBP, readssdword(pContext->ESP + 0x10), readssdword(pContext->ESP + 0x14));
    SysLogError("EDX= %08X  ESP= %08X                    SS:[ESP+18]= %08X %08X\n\r",
        pContext->EDX, pContext->ESP, readssdword(pContext->ESP + 0x18), readssdword(pContext->ESP + 0x1c));
    PrintSelector("CS", pContext->CS);
    PrintSelector("DS", pContext->DS);
    PrintSelector("ES", pContext->ES);
    PrintSelector("SS", pContext->SS);
    PrintSelector("FS", pContext->FS);
    PrintSelector("GS", pContext->GS);
    SysLogError("Loaded modules:\n\r");
    while (pListEntry) {
        SysLogError("    %08X: %s\n\r", pListEntry->DllBase, pListEntry->DllName);
        pListEntry = pListEntry->Next;
    }

    goto skipbt;

    VioSetCursorPosition(0, 7, 61);
    SysLogError("FramePtr RetAddr\n\r");
    {
        DWORD FramePtr = pContext->EBP;
        int i = 1;

        while (FramePtr) {
    VioSetCursorPosition(0, 7+i, 61);

        SysLogError("%08X %08X\n\r", FramePtr, fpeekd(pContext->SS, FramePtr+4));

        FramePtr = fpeekd(pContext->SS, FramePtr);
        i++;
        }
    }
    VioSetCursorPosition(0, 23, 0);

    /* Write a crash dump including hooked interrupt vectors, more stack and instruction dump, stack trace */
    /* And undummy the eflags, iopl, cpl */

    skipbt:

    DosExit(-1);

}



int main(int argc, char** argv) {
    PVOID pDosXplod;
    ULONG ulWritten;
    PVOID pTestExe;
    DWORD dwResult;
    EXCEPT_CONTEXT except;

    SetHandlers();
    
    printf("C4 80386 DOS Extender\nCopyright (c) 2025 by Will Klees\n");

    if (argc < 2) {
        printf("Please pass an EXE to launch on the command line.\n");
        return 0;
    }

    LdrPrintError(LaunchEXE(argv[1], &dwResult), argv[1]);  

    return dwResult;
}

