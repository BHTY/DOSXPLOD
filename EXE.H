/**
 *      File: EXE.H
 *      Definitions for executable formats
 *      Copyright (c) 2025 by Will Klees
 */

#ifndef __EXE_H_
#define __EXE_H_

#include "TYPES.H"

/* Useful constants */
#define MZ_MAGIC                            0x5A4D
#define PE_MAGIC                            0x4550
#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00
#define IMAGE_MACHINE_TYPE_I386             0x14C
#define IMAGE_NT_OPTIONAL_HDR_MAGIC         0x10B
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16
#define IMAGE_SIZEOF_SHORT_NAME             8
#define IMAGE_ORDINAL_FLAG                  0x80000000

/* Image characteristics */
#define IMAGE_FILE_RELOCS_STRIPPED          0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE         0x0002
#define IMAGE_FILE_DLL                      0x2000

/* Image section attributes */
#define IMAGE_SCN_CNT_CODE                  0x00000020  /* The section contains executable code */
#define IMAGE_SCN_CNT_INITIALIZED_DATA      0x00000040  /* The section contains initialized data */
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA    0x00000080  /* The section contains uninitialized data */
#define IMAGE_SCN_MEM_EXECUTE               0x20000000  /* The section can be executed as code */
#define IMAGE_SCN_MEM_READ                  0x40000000  /* The section can be read */
#define IMAGE_SCN_MEM_WRITE                 0x80000000  /* The section can be written to */

/* Data directories */
#define IMAGE_DIRECTORY_ENTRY_EXPORT        0   
#define IMAGE_DIRECTORY_ENTRY_IMPORT        1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE      2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION     3
#define IMAGE_DIRECTORY_ENTRY_SECURITY      4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC     5
#define IMAGE_DIRECTORY_ENTRY_DEBUG         6
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE  7
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR     8
#define IMAGE_DIRECTORY_ENTRY_TLS           9
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG   10
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT  11
#define IMAGE_DIRECTORY_ENTRY_IAT           12
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT  13
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14

/* Relocation types */
#define IMAGE_REL_BASED_ABSOLUTE                0
#define IMAGE_REL_BASED_HIGH                    1
#define IMAGE_REL_BASED_LOW                     2
#define IMAGE_REL_BASED_HIGHLOW                 3

/* Debug types */
#define IMAGE_DEBUG_TYPE_COFF       1

/* The "MZ" header for a standard DOS executable */
typedef struct _IMAGE_DOS_HEADER {
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

/* Data directory */
typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD VirtualAddress;
    DWORD Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

/* COFF section header */
typedef struct _IMAGE_SECTION_HEADER {
    BYTE  Name[IMAGE_SIZEOF_SHORT_NAME];    /* An 8-byte, null-padded ASCII string */
    union {
        DWORD PhysicalAddress;              
        DWORD VirtualSize;                  /* The total size of the section when loaded into memory, in bytes, valid only for executable images */
    } Misc;
    DWORD VirtualAddress;                   /* The address of the first byte of the section when loaded into memory, relative to the image base */
    DWORD SizeOfRawData;                    /* The size of the initialized data on disk, in bytes */
    DWORD PointerToRawData;                 /* A file pointer to the first page within the section */
    DWORD PointerToRelocations;
    DWORD PointerToLinenumbers;
    WORD  NumberOfRelocations;
    WORD  NumberOfLinenumbers;
    DWORD Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

/* The COFF header for an object file */
typedef struct _IMAGE_FILE_HEADER {
    WORD  Machine;                  /* What computer type is this executable designed for */
    WORD  NumberOfSections;         /* How many entries in the section table */
    DWORD TimeDateStamp;            /* UNIX timestamp of when the linker created this file */
    DWORD PointerToSymbolTable;     /* Offset of the COFF symbol table */
    DWORD NumberOfSymbols;          /* Number of COFF symbols */
    WORD  SizeOfOptionalHeader;     /* Size, in bytes, of the optional header */
    WORD  Characteristics;          /* See above */ 
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

/* COFF optional header for executable images */
typedef struct _IMAGE_OPTIONAL_HEADER {
    /* Standard fields (COFF) */
    WORD                    Magic;
    BYTE                    MajorLinkerVersion;
    BYTE                    MinorLinkerVersion;
    DWORD                   SizeOfCode;
    DWORD                   SizeOfInitializedData;
    DWORD                   SizeOfUninitializedData;
    DWORD                   AddressOfEntryPoint;
    DWORD                   BaseOfCode;
    DWORD                   BaseOfData;

    /* PE/COFF fields */
    DWORD                   ImageBase;
    DWORD                   SectionAlignment;
    DWORD                   FileAlignment;
    WORD                    MajorOperatingSystemVersion;
    WORD                    MinorOperatingSystemVersion;
    WORD                    MajorImageVersion;
    WORD                    MinorImageVersion;
    WORD                    MajorSubsystemVersion;
    WORD                    MinorSubsystemVersion;
    DWORD                   Win32VersionValue;
    DWORD                   SizeOfImage;
    DWORD                   SizeOfHeaders;
    DWORD                   CheckSum;
    WORD                    Subsystem;
    WORD                    DllCharacteristics;
    DWORD                   SizeOfStackReserve;
    DWORD                   SizeOfStackCommit;
    DWORD                   SizeOfHeapReserve;
    DWORD                   SizeOfHeapCommit;
    DWORD                   LoaderFlag;
    DWORD                   NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY    DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

/* Portable Executable header */
typedef struct _IMAGE_NT_HEADERS {
    DWORD                   Signature;          /* Signature identifying this image as a PE */
    IMAGE_FILE_HEADER       FileHeader;         /* COFF header */
    IMAGE_OPTIONAL_HEADER   OptionalHeader;     /* Executable header */
} IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;

/* Import stuff */
typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        DWORD Characteristics;
        DWORD OriginalFirstThunk;   /* RVA of the ILT */
    } DUMMYUNIONNAME;
    DWORD TimeDateStamp;
    DWORD ForwarderChain;           /* */
    DWORD Name;                     /* RVA of an ASCII string containing the name */
    DWORD FirstThunk;               /* RVA of the IAT */
} IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_THUNK_DATA {
    union {
        DWORD ForwarderString;      /* PBYTE */
        DWORD Function;             /* PDWORD */
        DWORD Ordinal;
        DWORD AddressOfData;        /* PIMAGE_IMPORT_BY_NAME */
    } u1;
} IMAGE_THUNK_DATA, *PIMAGE_THUNK_DATA;

typedef struct _IMAGE_IMPORT_BY_NAME {
    WORD Hint;
    CHAR Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

/* Export stuff */
typedef struct _IMAGE_EXPORT_DIRECTORY {
    DWORD Characteristics;          /* */
    DWORD TimeDateStamp;            /* */
    WORD  MajorVersion;             /* */
    WORD  MinorVersion;             /* */
    DWORD Name;                     /* */
    DWORD Base;                     /* */
    DWORD NumberOfFunctions;        /* */
    DWORD NumberOfNames;            /* */
    DWORD AddressOfFunctions;       /* */
    DWORD AddressOfNames;           /* */
    DWORD AddressOfNameOrdinals;    /* */
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

/* Reloc stuff */
typedef struct _IMAGE_BASE_RELOCATION {
    DWORD VirtualAddress;
    DWORD SizeOfBlock;
} IMAGE_BASE_RELOCATION, *PIMAGE_BASE_RELOCATION;

/* Debug stuff */
typedef struct _IMAGE_DEBUG_DIRECTORY {
    DWORD Characteristics;
    DWORD TimeDateStamp;
    WORD  MajorVersion;
    WORD  MinorVersion;
    DWORD Type;
    DWORD SizeOfData;
    DWORD AddressOfRawData;
    DWORD PointerToRawData;
} IMAGE_DEBUG_DIRECTORY, *PIMAGE_DEBUG_DIRECTORY;

typedef struct _IMAGE_COFF_SYMBOLS_HEADER {
    DWORD NumberOfSymbols;
    DWORD LvaToFirstSymbol;
    DWORD NumberOfLinenumbers;
    DWORD LvaToFirstLinenumber;
    DWORD RvaToFirstByteOfCode;
    DWORD RvaToLastByteOfCode;
    DWORD RvaToFirstByteOfData;
    DWORD RvaToLastByteOfData;
} IMAGE_COFF_SYMBOLS_HEADER, *PIMAGE_COFF_SYMBOLS_HEADER;

#pragma pack(push, 1)
typedef struct _IMAGE_COFF_SYMBOL {
    union {
        char Name[8];
        struct {
            DWORD Zeroes;
            DWORD Offset;
        } Table;
    } Name;
    DWORD Value;
    WORD  SectionNumber;
    WORD  Type;
    BYTE  StorageClass;
    BYTE  NumberOfAuxiliarySymbols;
} IMAGE_COFF_SYMBOL, *PIMAGE_COFF_SYMBOL;
#pragma pack(pop)

#endif
