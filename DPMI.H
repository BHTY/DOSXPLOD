/**
 *      File: DPMI.H
 *      Function prototypes and C call interface for DOS Protect Mode Interface
 *      Copyright (c) 2025 by Will Klees
 */

#ifndef __DPMI_H_
#define __DPMI_H_

#include "TYPES.H"

/* DPMI real-mode register structure */
typedef struct _DPMIREGS {
    DWORD EDI, ESI, EBP, Reserved, EBX, EDX, ECX, EAX;
    WORD FLAGS, ES, DS, FS, GS, CS, IP, SP, SS;
} DPMIREGS;

/* DPMI version structure */
typedef struct _DPMIVER {
    WORD wFlags;
    BYTE bType;
    BYTE MasterPICBase;
    BYTE SlavePICBase;
} DPMIVER;

/* 16:32 far pointer */
#pragma pack(push, 2)
typedef struct _DPMIFPTR32 {
    DWORD dwOffset;
    WORD  wSelector;
} DPMIFPTR32;
#pragma pack(pop)

/* DPMI memory info structure */
typedef struct _DPMIMEMINFO {
    DWORD dwLargestFreeBlock;       /* in bytes */
    DWORD dwMaxUnlockedAllocation;  /* in pages */
    DWORD dwMaxLockedAllocation;    /* in pages */
    DWORD dwAddrSpaceSize;          /* in pages */
    DWORD dwTotalUnlockedPages;
    DWORD dwTotalFreePages;
    DWORD dwTotalPhysPages;
    DWORD dwFreeAddrSpace;          /* in pages */
    DWORD dwSwapSize;               /* paging file size in pages */
    DWORD Reserved[3];              /* all set to 0FFh */
} DPMIMEMINFO;

/* DPMI typedefs */
typedef WORD DPMISTATUS;
typedef DWORD HMEMBLOCK;
typedef DWORD DPMIFPTR16;

/* DPMI error codes */
#define DPMI_SUCCESS                    0
#define DPMI_MCB_DAMAGED                7
#define DPMI_INSUFFICIENT_MEM           8
#define DPMI_INCORRECT_MEM_SEG          9
#define DPMI_UNSUPPORTED_FN             0x8001
#define DPMI_INVALID_STATE              0x8002
#define DPMI_SYS_INTEGRITY              0x8003
#define DPMI_DEADLOCK                   0x8004
#define DPMI_REQ_CANCELLED              0x8005
#define DPMI_RES_UNAVAILABLE            0x8010
#define DPMI_DESC_UNAVAILABLE           0x8011
#define DPMI_LIN_MEM_UNAVAILABLE        0x8012
#define DPMI_PHYS_MEM_UNAVAILABLE       0x8013
#define DPMI_BACKING_STORE_UNAVAILABLE  0x8014
#define DPMI_CALLBACK_UNAVAILABLE       0x8015
#define DPMI_HANDLE_UNAVAILABLE         0x8016
#define DPMI_LOCK_COUNT_EXCEEDED        0x8017
#define DPMI_RES_OWNED_EXCLUSIVE        0x8018
#define DPMI_RES_OWNED_SHARED           0x8019
#define DPMI_INVALID_VALUE              0x8021
#define DPMI_INVALID_SELECTOR           0x8022
#define DPMI_INVALID_HANDLE             0x8023
#define DPMI_INVALID_CALLBACK           0x8024
#define DPMI_INVALID_LIN_ADDR           0x8025
#define DPMI_INVALID_REQUEST            0x8026

/* Selector management services */
DPMISTATUS DpmiAllocSelectors(WORD nSelectors, WORD* pwBaseSel);
DPMISTATUS DpmiFreeSelector(WORD wSelector);
DPMISTATUS DpmiMapSegmentSelector(WORD wRealSeg, WORD* pwSel);
WORD       DpmiGetSelectorIncrement();
DPMISTATUS DpmiGetSelectorBase(WORD wSelector, DWORD* pdwBase);
DPMISTATUS DpmiSetSelectorBase(WORD wSelector, DWORD dwBase);
DPMISTATUS DpmiSetSelectorLimit(WORD wSelector, DWORD dwLimit);
DPMISTATUS DpmiSetSelectorAccess(WORD wSelector, WORD wAccess);
DPMISTATUS DpmiAllocAlias(WORD wSelector, WORD *pwAliasSel);
DPMISTATUS DpmiGetDescriptor(WORD wSelector, BYTE* pDescriptor);
DPMISTATUS DpmiSetDescriptor(WORD wSelector, BYTE* pDescriptor);
DPMISTATUS DpmiAllocSpecificSelector(WORD wSelector);

/* Extended memory management services */
void       DpmiMemInfo(DPMIMEMINFO* pMemInfo);
DPMISTATUS DpmiMemAlloc(DWORD dwBlockSize, DWORD* pdwLinAddr, HMEMBLOCK* phBlock);
DPMISTATUS DpmiMemFree(HMEMBLOCK hBlock);
DPMISTATUS DpmiMemResize(DWORD dwNewSize, HMEMBLOCK hBlock, DWORD* pdwLinAddr, HMEMBLOCK* phBlock);
DPMISTATUS DpmiMapLinear(DWORD dwPhysAddr, DWORD dwRegionSize, DWORD* pdwLinAddr);

/* DOS memory management services */
DPMISTATUS DpmiDosAlloc(WORD wParagraphs, WORD* pwSegment, WORD *pwSelector, WORD* pwLargestBlock);
DPMISTATUS DpmiDosFree(WORD wSelector);
DPMISTATUS DpmiDosResize(WORD wNewBlockSize, WORD wSelector, WORD* pwMaxBlockSize);

/* Interrupt management services */
DPMIFPTR16 DpmiGetRealModeIntVect(BYTE intr);
void       DpmiSetRealModeIntVect(BYTE intr, DPMIFPTR16 intvect);
DPMISTATUS DpmiGetExceptionHandler(BYTE exception, DPMIFPTR32* pHandler);
DPMISTATUS DpmiSetExceptionHandler(BYTE exception, DPMIFPTR32* pHandler);
PVOID      DpmiGetProtModeIntVect(BYTE intr, WORD* pwSel);
DPMISTATUS DpmiSetProtModeIntVect(BYTE intr, PVOID intvect, WORD wSel);
BYTE       DpmiGetAndDisableVIF();
BYTE       DpmiGetAndDisableVIF();
BYTE       DpmiGetVIF();

/* Translation services */
DPMISTATUS DpmiSimulateRealModeInt(BYTE intr, WORD wStackCopyWords, DPMIREGS* pRegs);
DPMISTATUS DpmiCallRealModeProcFar(WORD wStackCopyWords, DPMIREGS* pRegs);
DPMISTATUS DpmiCallRealModeProcInt(WORD wStackCopyWords, DPMIREGS* pRegs);
DPMISTATUS DpmiAllocRealModeCallback(PVOID ProcAddr, DPMIREGS* pRegs, DPMIFPTR16* pfpCallback);
DPMISTATUS DpmiFreeRealModeCallback(DPMIFPTR16 fpCallback);
WORD       DpmiGetSaveRestoreAddr(DPMIFPTR16* pRealModeAddr, DPMIFPTR32* pProtModeAddr);
DPMIFPTR16 DpmiGetRawModeSwitchAddr(DPMIFPTR32* pAddr);

/* Page management services */
DPMISTATUS DpmiLock(DWORD dwLinAddr, DWORD dwRegionSize);
DPMISTATUS DpmiUnlock(DWORD dwLinAddr, DWORD dwRegionSize);
DPMISTATUS DpmiMarkPageable(DWORD dwLinAddr, DWORD dwRegionSize); /* conventional */
DPMISTATUS DpmiRelockConventional(DWORD dwLinAddr, DWORD dwRegionSize);
DWORD      DpmiGetPageSize();
DPMISTATUS DpmiMarkDemandPaging(DWORD dwLinAddr, DWORD dwRegionSize);
DPMISTATUS DpmiDiscardPage(DWORD dwLinAddr, DWORD dwRegionSize);

/* Debug support services */
DPMISTATUS DpmiSetWatchpoint(DWORD dwLinAddr, BYTE cSize, BYTE cType, WORD* phWp);
DPMISTATUS DpmiClearWatchpoint(WORD hWp);
DPMISTATUS DpmiGetWatchpointState(WORD hWp, WORD* pwStatus);
DPMISTATUS DpmiResetWatchpoint(WORD hWp);

/* Misc services */
WORD       DpmiGetVersion(DPMIVER* pVer);

#endif
