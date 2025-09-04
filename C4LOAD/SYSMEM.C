/**
 *      File: SYSMEM.C
 *      Exported routines for memory manager
 *      Copyright (c) 2025 by Will Klees
 *  
 *      The purpose of these routines are to provide a more standard interface
 *      over allocating memory. While the DPMI requires calls to resize or free
 *      memory use handles instead of addresses, this interface provides a 
 *      layer of translation. Note that while this interface currently 
 *      leverages the DPMI memory pool, this is an implementation detail that
 *      is subject to change at any time.
 */

#include <DOSXPLOD.H>
#include <DPMI.H>

/* An entry in the translation table */
typedef struct _MEM_TABLE_ENTRY {
    HMEMBLOCK hMemBlock;
    PVOID ptr;
} MEM_TABLE_ENTRY;

#define NUM_TABLE_ENTRIES 128

MEM_TABLE_ENTRY MemTable[NUM_TABLE_ENTRIES];

/**
 *  MemFindFreeTblEntry routine - Finds a free entry in the translation table.
 * 
 *  @return: An index into the table if there is a free entry, or -1 if not.
 */
INT MemFindFreeTblEntry() {
    INT i;

    for (i = 0; i < NUM_TABLE_ENTRIES; i++) {
        if (MemTable[i].hMemBlock == 0) return i;
    }

    return -1;
}

/**
 *  MemFindMatchingTblEntry routine - Finds a matching entry in the translation
 *  table.
 * 
 *  @param ptr: A pointer to the first byte in the linear memory block.
 * 
 *  @return: An index into the table if a matching block is found, or -1 if 
 *  not.
 */
INT MemFindMatchingTblEntry(PVOID ptr) {
    INT i;

    for (i = 0; i < NUM_TABLE_ENTRIES; i++) {
        if (MemTable[i].ptr == ptr) return i;
    }

    return -1;
}

/**
 *  SysMemAlloc routine - Allocates and commits a block of linear memory.
 * 
 *  @param dwLen: The number of bytes to allocate.
 * 
 *  @return: A pointer to the first byte of the allocated block if successful,
 *  or NULL if not.
 */
PVOID     SysMemAlloc(DWORD dwLen) {
    INT iTblIndex = MemFindFreeTblEntry();
    HMEMBLOCK hMemBlock;
    DWORD dwLinAddr;

    /* Try to allocate a table entry and then the memory itself */
    if (iTblIndex == -1) return NULL;
    if (DpmiMemAlloc(dwLen, &dwLinAddr, &hMemBlock)) return NULL;

    /* Add an entry into the table */
    MemTable[iTblIndex].hMemBlock = hMemBlock;
    MemTable[iTblIndex].ptr = dwLinAddr;

    return dwLinAddr;
}

/**
 *  SysMemReAlloc routine - Resizes an allocated block of linear memory.
 * 
 *  @param ptr: A pointer that was previously returned by a call to SysMemAlloc
 *  or SysMemReAlloc.
 * 
 *  @param dwNewLen: The new desired size of the linear memory block.
 * 
 *  @return: A pointer to the first byte of the resized (shrunk or extended)
 *  block if the call is successful, or NULL if not. Note that this pointer
 *  may be the same as ptr, but that this cannot be assumed.
 */
PVOID     SysMemReAlloc(PVOID ptr, DWORD dwNewLen) {
    INT iTblIndex = MemFindMatchingTblEntry(ptr);
    HMEMBLOCK hMemBlock;
    DWORD dwLinAddr;

    /* Find the matching table entry */
    if (iTblIndex == -1) return NULL;

    /* Try resizing the memory block */
    if (DpmiMemResize(dwNewLen, MemTable[iTblIndex].hMemBlock, &dwLinAddr, &hMemBlock)) return NULL;

    /* Adjust the table entry */
    MemTable[iTblIndex].hMemBlock = hMemBlock;
    MemTable[iTblIndex].ptr = dwLinAddr;

    return dwLinAddr;
}

/**
 *  SysMemFree routine - Frees a block of linear memory.
 * 
 *  @param ptr: A pointer that was previously returned by a call to SysMemAlloc
 *  or SysMemReAlloc.
 */
void      SysMemFree(PVOID ptr) {
    /* If there is a matching table entry, delete it */
    INT iTblIndex = MemFindMatchingTblEntry(ptr);
    if (iTblIndex != -1) {
        DpmiMemFree(MemTable[iTblIndex].hMemBlock);
        MemTable[iTblIndex].hMemBlock = 0;
    }
}
