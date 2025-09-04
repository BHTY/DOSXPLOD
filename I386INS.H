/**
 *      File: I386INS.H
 *      Intrinsics for commonly used Intel 80386 instructions
 *      Copyright (c) 2025 by Will Klees
 */

#ifndef __I386INS_H_
#define __I386INS_H_

/* TODO: Add far-pointer versions of intrinsics that take pointers (like movsb) */
/* readfsdword, ... */

#include "TYPES.H"

#define inline static

/* General-purpose-register data structure */
struct DWORDREGS386 {
    DWORD eax, ebx, ecx, edx, esi, edi, cflag;
};

struct WORDREGS386 {
    WORD ax, filler1, bx, filler2, cx, filler3, dx, filler4, si, filler5, di, filler6, cflag, filler7;
};

struct BYTEREGS386 {
    BYTE al, ah, filler1, filler2;
    BYTE bl, bh, filler3, filler4;
    BYTE cl, ch, filler5, filler6;
    BYTE dl, dh, filler7, filler8;
};

typedef union _REGS386 {
    struct DWORDREGS386 x;
    struct WORDREGS386  w;
    struct BYTEREGS386  h;
} REGS386;

/* Segment register data structure */
typedef struct _SREGS386 {
    WORD es, cs, ss, ds, fs, gs;
} SREGS386;

/* Segment Descriptor data structure */
typedef struct _SEG_DESC {
    WORD wLimitLow;
    WORD wBaseLow;
    BYTE cBaseHighLow;
    BYTE cAccess;
    BYTE cLimitHighFlags;
    BYTE cBaseHighHigh; 
} SEG_DESC, *PSEG_DESC;

#define SEG_PRESENT(desc)   ((desc).cAccess & 0x80)
#define SEG_TYPE(desc)      ((desc).cAccess & 0x10)
#define SEG_DPL(desc)       (((desc).cAccess >> 5) & 3)
#define SEG_EXEC(desc)      ((desc).cAccess & 8)
#define SEG_GRAN(desc)      ((desc).cLimitHighFlags & 0x80)
#define SEG_SIZE(desc)      ((desc).cLimitHighFlags & 0x40)
#define SEG_BASE(desc)      ((desc).wBaseLow | ((desc).cBaseHighLow << 16) | ((desc).cBaseHighHigh << 24))
#define SEG_LMT(desc)       ((desc).wLimitLow | (((desc).cLimitHighFlags & 0xF) << 16))
#define SEG_LIMIT(desc)     (SEG_GRAN(desc) ? ((SEG_LMT(desc) << 12) | 0xFFF) : SEG_LMT(desc))

/* i386 flag masks */
#define I386_FLAG_CY 0
#define I386_FLAG_PF 2
#define I386_FLAG_AC 4
#define I386_FLAG_ZR 6
#define I386_FLAG_SIGN 7
#define I386_FLAG_TRAP 8
#define I386_FLAG_IE 9
#define I386_FLAG_DIR 10
#define I386_FLAG_OV 11

/**
 *  Performs an interrupt in protected-mode.
 */
inline DWORD int386(BYTE intr, REGS386* pRegs) {

}

/**
 *  Performs an interrupt in protected-mode with segment registers.
 */
inline DWORD int386x(BYTE intr, REGS386* pRegs, SREGS386* pSegRegs) {

}

/**
 *  Returns the value of control register 2
 */
inline DWORD getCR2() {
    __asm {
        mov eax, cr2
    }
}

/**
 *  Returns the value of control register 3
 */
inline DWORD getCR3() {
    __asm {
        mov eax, cr3
    }
}

/**
 *  Reads a BYTE from the offset relative to CS
 */
inline BYTE readcsbyte(DWORD dwOff) {
    __asm {
        mov eax, dwOff
        mov al, cs:[eax]
    }
}

/**
 *  Reads a DWORD from the offset relative to SS
 */
inline DWORD readssdword(DWORD dwOff) {
    __asm {
        mov eax, dwOff
        mov eax, ss:[eax]
    }
}

/**
 *  Reads a BYTE from the specified 16:32 48-bit far pointer
 */
inline BYTE fpeekb(WORD wSeg, DWORD dwOff) {
    __asm {
        mov bx, ds
        mov ax, wSeg
        mov ds, ax
        mov eax, dwOff
        mov al, [eax]
        mov ds, bx
    }
}

/**
 *  Reads a WORD from the specified 16:32 48-bit far pointer
 */
inline WORD fpeekw(WORD wSeg, DWORD dwOff) {
    __asm {
        mov bx, ds
        mov ax, wSeg
        mov ds, ax
        mov eax, dwOff
        mov ax, [eax]
        mov ds, bx
    }
}

/**
 *  Reads a DWORD from the specified 16:32 48-bit far pointer
 */
inline DWORD fpeekd(WORD wSeg, DWORD dwOff) {
    __asm {
        mov bx, ds
        mov ax, wSeg
        mov ds, ax
        mov eax, dwOff
        mov eax, [eax]
        mov ds, bx
    }
}

/**
 *  Writes a BYTE to the specified 16:32 48-bit far pointer
 */
inline void fpokeb(WORD wSeg, DWORD dwOff, BYTE cVal) {
    __asm {
        mov bx, ds
        mov ax, wSeg
        mov ds, ax
        mov eax, dwOff
        mov cl, cVal
        mov [eax], cl
        mov ds, bx
    }
}

/**
 *  Writes a WORD to the specified 16:32 48-bit far pointer
 */
inline void fpokew(WORD wSeg, DWORD dwOff, WORD wVal) {
    __asm {
        mov bx, ds
        mov ax, wSeg
        mov ds, ax
        mov eax, dwOff
        mov cx, wVal
        mov [eax], cx
        mov ds, bx
    }
}

/**
 *  Writes a DWORD to the specified 16:32 48-bit far pointer
 */
inline void fpoked(WORD wSeg, DWORD dwOff, DWORD dwVal) {
    __asm {
        mov bx, ds
        mov ax, wSeg
        mov ds, ax
        mov eax, dwOff
        mov ecx, dwVal
        mov [eax], ecx
        mov ds, bx
    }
}

/**
 *  Reads segments into a struct
 */
inline void readseg(SREGS386* pSegRegs) {

}

/**
 *  getCS procedure - Retrieves the value of the CS register
 */
inline WORD getCS() {
    __asm {
        mov ax, cs
    }
}

/**
 *  getDS procedure - Retrieves the value of the DS register
 */
inline WORD getDS() {
    __asm {
        mov ax, ds
    }
}

/**
 *  getES procedure - Retrieves the value of the ES register
 */
inline WORD getES() {
    __asm {
        mov ax, es
    }
}

/**
 *  getSS procedure - Retrieves the value of the SS register
 */
inline WORD getSS() {
    __asm {
        mov ax, ss
    }
}

/**
 *  getFS procedure - Retrieves the value of the FS register
 */
inline WORD getFS() {
    __asm {
        mov ax, fs
    }
}

/**
 *  getGS procedure - Retrieves the value of the GS register
 */
inline WORD getGS() {
    __asm {
        mov ax, gs
    }
}

/**
 *  movsb procedure - Copies a non-overlapping memory block
 */
inline void movsb(CHAR* pcDst, CHAR* pcSrc, DWORD dwLen) {
    __asm {
        mov edi, pcDst
        mov esi, pcSrc
        mov ecx, dwLen
        rep movsb
    }
}

/**
 *  movsw procedure - Copies a non-overlapping memory block
 */
inline void movsw(WORD* pwDst, WORD* pwSrc, DWORD dwLen) {
    __asm {
        mov edi, pwDst
        mov esi, pwSrc
        mov ecx, dwLen
        rep movsw
    }
}

/**
 *  movsd procedure - Copies a non-overlapping memory block
 */
inline void movsd(DWORD* pdwDst, DWORD* pdwSrc, DWORD dwLen) {
    __asm {
        mov edi, pdwDst
        mov esi, pdwSrc
        mov ecx, dwLen
        rep movsd
    }
}

/**
 *  stosb procedure - Fills a memory block with BYTEs
 */
inline void stosb(CHAR* pcDst, CHAR cVal, DWORD dwLen) {
    __asm {
        mov edi, pcDst
        mov al, cVal
        mov ecx, dwLen
        rep stosb
    }
}

/**
 *  stosw procedure - Fills a memory block with WORDs
 */
inline void stosw(WORD* pwDst, WORD wVal, DWORD dwLen) {
    __asm {
        mov edi, pwDst
        mov ax, wVal
        mov ecx, dwLen
        rep stosw
    }
}

/**
 *  stosd procedure - Fills a memory block with DWORDs
 */
inline void stosd(DWORD* pdwDst, DWORD dwVal, DWORD dwLen) {
    __asm {
        mov edi, pdwDst
        mov eax, dwVal
        mov ecx, dwLen
        rep stosd
    }
}

/**
 *  outb - Writes a BYTE to an I/O port
 */
inline void outb(WORD wPort, BYTE cVal) {
    __asm {
        mov dx, wPort
        mov al, cVal
        out dx, al
    }
}

/**
 *  outw - Writes a WORD to an I/O port
 */
inline void outw(WORD wPort, WORD wVal) {
    __asm {
        mov dx, wPort
        mov ax, wVal
        out dx, ax
    }
}

/**
 *  outd - Writes a DWORD to an I/O port
 */
inline void outd(WORD wPort, DWORD dwVal) {
    __asm {
        mov dx, wPort
        mov eax, dwVal
        out dx, eax
    }
}

/**
 *  outsb - Writes a string of BYTEs to an I/O port
 */
inline void outsb(WORD wPort, BYTE* pcSrc, DWORD dwLen) {
    __asm {
        mov dx, wPort
        mov esi, pcSrc
        mov ecx, dwLen
        rep outsb
    }
}

/**
 *  outsw - Writes a string of WORDs to an I/O port
 */
inline void outsw(WORD wPort, WORD* pwSrc, DWORD dwLen) {
    __asm {
        mov dx, wPort
        mov esi, pwSrc
        mov ecx, dwLen
        rep outsw
    }
}

/**
 *  outsd - Writes a string of DWORDs to an I/O port
 */
inline void outsd(WORD wPort, DWORD* pdwSrc, DWORD dwLen) {
    __asm {
        mov dx, wPort
        mov esi, pdwSrc
        mov ecx, dwLen
        rep outsd
    }
}

/**
 *  inb - Reads a BYTE from an I/O port
 */
inline BYTE inb(WORD wPort) {
    __asm {
        mov dx, wPort
        in al, dx
    }
}

/**
 *  inw - Reads a WORD from an I/O port
 */
inline WORD inw(WORD wPort) {
    __asm {
        mov dx, wPort
        in ax, dx
    }
}

/**
 *  ind - Reads a DWORD from an I/O port
 */
inline DWORD ind(WORD wPort) {
    __asm {
        mov dx, wPort
        in eax, dx
    }
}

/**
 *  insb - Reads a string of BYTEs from an I/O port
 */
inline void insb(BYTE* pcDst, WORD wPort, DWORD dwLen) {
    __asm {
        mov dx, wPort
        mov edi, pcDst
        mov ecx, dwLen
        rep insb
    }
}

/**
 *  insw - Reads a string of WORDs from an I/O port
 */
inline void insw(WORD* pwDst, WORD wPort, DWORD dwLen) {
    __asm {
        mov dx, wPort
        mov edi, pwDst
        mov ecx, dwLen
        rep insw
    }
}

/**
 *  insd - Reads a string of DWORDs from an I/O port
 */
inline void insd(DWORD* pdwDst, WORD wPort, DWORD dwLen) {
    __asm {
        mov dx, wPort
        mov edi, pdwDst
        mov ecx, dwLen
        rep insd
    }
}

#endif
