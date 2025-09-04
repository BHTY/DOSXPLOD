/**
 *      File: DPMI.C
 *      C call interface for DOS Protect Mode Interface
 *      Copyright (c) 2025 by Will Klees
 */

#include "../DPMI.H"

/**
 *  DpmiAllocSelectors procedure - Allocates one or more descriptors in the
 *  task's Local Descriptor Table (LDT). The descriptor(s) allocated must be
 *  initialized by the application with other function calls. If more than one
 *  descriptor was requested, the function returns a base selector referencing
 *  the first of a contiguous array of descriptors. The selector values for
 *  subsequent descriptors in the array can be calculated by adding the value
 *  returned by DpmiGetSelectorIncrement. The allocated descriptors will be
 *  set to "data" with the present bit set and a base and limit of 0. The
 *  privilege level of the descriptors matches that of the code segment.
 * 
 *  @param nSelectors: Number of descriptors to allocate
 * 
 *  @param pwBaseSel: A pointer to receive the base selector if the call is
 *  successful
 * 
 *  @return: 0 if the call is successful, a DPMI error code otherwise:
 *      DPMI_DESC_UNAVAILABLE
 */
DPMISTATUS DpmiAllocSelectors(WORD nSelectors, WORD* pwBaseSel) {
    __asm {
        mov ax, 0                   ; DPMI call: Allocate LDT Descriptors
        mov cx, nSelectors          ; CX = Number of selectors
        int 31h
        jc failure                  ; Did the call fail?
        mov edi, pwBaseSel          ;   No, *pwBaseSel = AX
        mov [edi], ax
        xor ax, ax                  ;   Clear AX (no error)

        failure:                    ; Yes, AX = error code
    }
}

/**
 *  DpmiFreeSelector procedure - Frees an LDT descriptor. Each descriptor
 *  allocates with INT 31H Function 0000H must be freed individually, even
 *  if it was previously allocated as part of a contiguous array.
 * 
 *  @param wSelector: Selector for the descriptor to free.
 * 
 *  @return: 0 if the call is successful, a DPMI error code otherwise:
 *      DPMI_INVALID_SELECTOR
 */
DPMISTATUS DpmiFreeSelector(WORD wSelector) {
    __asm {
        mov ax, 1                   ; DPMI call: Free LDT Descriptor
        mov bx, wSelector           ; BX = selector for the descriptor to free
        int 31h
        jc failure                  ; Did the call fail?
        xor ax, ax                  ;   No, clear AX (no error)

        failure:                    ; Yes, AX = error code
    }
}

/**
 *  DpmiMapSegmentSelector procedure - Maps a real-mode segment (paragraph)
 *  address onto an LDT descriptor that can be used by a protected-mode
 *  program to access the same memory.
 * 
 *  @param wRealSeg: The real-mode segment address.
 * 
 *  @param pwSel: A pointer to receive the protected-mode selector if the call
 *  succeeds.
 * 
 *  @return: 0 if the call is successful, a DPMI error code otherwie:
 *      DPMI_DESC_UNAVAILABLE
 */
DPMISTATUS DpmiMapSegmentSelector(WORD wRealSeg, WORD* pwSel) {
    __asm {
        mov ax, 2                   ; DPMI call: Segment to Descriptor
        mov bx, wRealSeg            ; BX = real mode segment address
        int 31h
        jc failure                  ; Did the call fail?
        mov edi, pwSel              ;   No, *pwSel = AX
        mov [edi], ax
        xor ax, ax                  ;   Clear AX

        failure:                    ;   Yes, AX = error code
    }
}

/**
 *  DpmiGetSelectorIncrement procedure - DPMI functions returning selectors
 *  can allocate an array of contiguous selectors, but return only the 
 *  first.
 * 
 *  @return: Returns the increment value to advance between subsequent 
 *  selectors in the same allocation.
 */
WORD       DpmiGetSelectorIncrement() {
    __asm {
        mov ax, 3                   ; DPMI call: Get Selector Increment Value
        int 31h                     ; AX = Selector increment value
    }
}

/**
 *  DpmiGetSelectorBase procedure -  Returns the 32-bit linear base address 
 *  from the LDT descriptor for the specified segment.
 * 
 *  @param wSelector: The selector to retrieve the base address for.
 * 
 *  @param pdwBase: A pointer to receive the linear base address.
 * 
 *  @return: 0 if the call is successful, a DPMI error code otherwise:
 *      DPMI_INVALID_SELECTOR
 */
DPMISTATUS DpmiGetSelectorBase(WORD wSelector, DWORD* pdwBase) {
    __asm {
        mov ax, 6                   ; DPMI call: Get Segment Base Address
        mov bx, wSelector           ; BX = selector
        int 31h
        jc failure                  ; Did the call fail?
        xor ax, ax                  ;   No, clear AX (no error)
        mov edi, pdwBase            ;   *pdwBase = CX:DX
        mov [edi], dx
        mov [edi+2], cx

        failure:                    ;   Yes, AX = error code
    }
}

/**
 *  DpmiSetSelectorBase procedure - Sets the 32-bit linear base address field
 *  in the LDT descriptor for the specified segment.
 * 
 *  @param wSelector: The selector.
 * 
 *  @param dwBase: The 32-bit linear base address of the segment.
 * 
 *  @return: 0 if the call is successful, a DPMI error code otherwise
 *      DPMI_INVALID_SELECTOR
 *      DPMI_INVALID_LIN_ADDR
 */
DPMISTATUS DpmiSetSelectorBase(WORD wSelector, DWORD dwBase) {

}

/**
 *  DpmiSetSelectorLimit procedure - Sets the limit field in the LDT descriptor
 *  for the specified segment. Note that there is no corresponding function to
 *  retrieve the limit of a selector; client programs must use the LSL
 *  instruction.
 * 
 *  @param wSelector: The selector.
 * 
 *  @param dwLimit: The 32-bit segment limit.
 * 
 *  @return: 0 if the call is successful, a DPMI error code otherwise
 *      DPMI_INVALID_VALUE
 *      DPMI_INVALID_SELECTOR
 *      DPMI_INVALID_LIN_ADDR
 */
DPMISTATUS DpmiSetSelectorLimit(WORD wSelector, DWORD dwLimit) {

}

/**
 * 
 */
DPMISTATUS DpmiSetSelectorAccess(WORD wSelector, WORD wAccess) {

}

/**
 * 
 */
DPMISTATUS DpmiAllocAlias(WORD wSelector, WORD *pwAliasSel) {

}

/**
 *  DpmiGetDescriptor procedure - Copies the LDT entry for the specified 
 *  selector into an 8-byte buffer.
 * 
 *  @param wSelector: The selector
 * 
 *  @param pDescriptor: A pointer to the buffer receiving the descriptor
 * 
 *  @return: 0 if the call is successful, a DPMI error code otherwise
 *      DPMI_INVALID_SELECTOR
 */
DPMISTATUS DpmiGetDescriptor(WORD wSelector, BYTE* pDescriptor) {
    __asm {
        mov ax, 0bh             ; DPMI call: Get Descriptor
        mov bx, wSelector       ; BX = selector
        mov edi, pDescriptor    ; ES:EDI = pointer to buffer
        int 31h
        jc failure
        xor ax, ax              ; Success, clear AX

        failure:
    }
}

/**
 * 
 */
DPMISTATUS DpmiSetDescriptor(WORD wSelector, BYTE* pDescriptor) {

}

/**
 * 
 */
DPMISTATUS DpmiAllocSpecificSelector(WORD wSelector) {

}

/**
 *  DpmiMemInfo procedure - 
 */
void       DpmiMemInfo(DPMIMEMINFO* pMemInfo) {

}

/**
 *  DpmiMemAlloc procedure - Allocates and commits a block of linear memory.
 * 
 *  @param dwBlockSize: The number of bytes to allocate (bytes, must be nonzero).
 *  
 *  @param pdwLinAddr: A pointer to receive the linear address of the allocated
 *  memory block, if successful.
 * 
 *  @param phBlock: A pointer to receive a handle to the allocated memory block
 *  (used for resize and free), if successful.
 * 
 *  @return: 0 if successful, a DPMI error code otherwise
 *      DPMI_LIN_MEM_UNAVAILABLE
 *      DPMI_PHYS_MEM_UNAVAILABLE
 *      DPMI_BACKING_STORE_UNAVAILABLE
 *      DPMI_HANDLE_UNAVAILABLE
 *      DPMI_INVALID_VALUE (dwBlockSize = 0)
 */
DPMISTATUS DpmiMemAlloc(DWORD dwBlockSize, DWORD* pdwLinAddr, HMEMBLOCK* phBlock) {
    __asm {
        mov ax, 501h                    ; DPMI call: Allocate Memory Block
        mov cx, word ptr [dwBlockSize]  ; BX:CX = Size of block
        mov bx, word ptr [dwBlockSize+2]
        int 31h
        jc done                         ; Did the call fail?
        xor ax, ax                      ;   No, clear AX
        mov edx, pdwLinAddr             ;   *pdwLinAddr = BX:CX
        mov [edx], cx
        mov [edx+2], bx
        mov edx, phBlock                ;   *phBlock = SI:DI
        mov [edx], di
        mov [edx+2], si

        done:                           ;   Yes, AX = error
    }
}

/**
 *  DpmiMemFree procedure - Frees a memory block that was previously allocated
 *  with the DpmiMemAlloc function.
 * 
 *  @param hBlock: Handle of the memory block to free.
 * 
 *  @return: 0 if successful, a DPMI error code otherwise
 *      DPMI_INVALID_HANDLE
 */
DPMISTATUS DpmiMemFree(HMEMBLOCK hMemBlock) {
    __asm {
        mov ax, 502h                    ; DPMI call: Free Memory Block
        mov di, word ptr [hMemBlock]    ; SI:DI = Memory block handle
        mov si, word ptr [hMemBlock+2]
        int 31h
        jc done                         ; Did the call fail?
        xor ax, ax                      ;   No, clear AX

        done:                           ;   Yes, AX = error
    }
}

/**
 *  DpmiMemResize procedure - Changes the size of a memory block that was
 *  previously allocated with the DpmiMemAlloc or DpmiMemResize function.
 * 
 *  @param dwNewSize: The new desired size for the memory block.
 * 
 *  @param hBlock: The handle of the memory block to be resized.
 * 
 *  @param pdwLinAddr: A pointer to receive the new linear address of the
 *  memory block, if successful.
 * 
 *  @param phBlock: A pointer to receive a new handle for the memory block,
 *  if successful.
 * 
 *  @return: 0 if successful, a DPMI error code otherwise
 *      DPMI_LIN_MEM_UNAVAILABLE
 *      DPMI_PHYS_MEM_UNAVAILABLE
 *      DPMI_BACKING_STORE_UNAVAILABLE
 *      DPMI_HANDLE_UNAVAILABLE
 *      DPMI_INVALID_VALUE (new block size = 0)
 *      DPMI_INVALID_HANDLE (invalid hBlock)
 */
DPMISTATUS DpmiMemResize(DWORD dwNewSize, HMEMBLOCK hBlock, DWORD* pdwLinAddr, HMEMBLOCK* phBlock) {
    __asm {
        mov ax, 503h                    ; DPMI call: Resize Memory Block
        mov cx, word ptr [dwNewSize]    ; BX:CX = new size of block (bytes)
        mov bx, word ptr [dwNewSize+2]
        mov di, word ptr [hBlock]       ; SI:DI = memory block handle
        mov si, word ptr [hBlock+2]     
        int 31h
        jc failure                      ; Did the call fail?
        xor ax, ax                      ;   No, clear AX
        mov edx, pdwLinAddr             ;   *pdwLinAddr = BX:CX
        mov [edx], cx
        mov [edx+2], bx
        mov edx, phBlock                ;   *phBlock = SI:DI
        mov [edx], di
        mov [edx+2], si

        failure:
    }
}

/**
 *  DpmiMapLinear - Converts a physical address into a linear address.
 * 
 *  @param dwPhysAddr: The physical address of memory.
 * 
 *  @param dwRegSize: The size of the region to map, in bytes.
 * 
 *  @param pdwLinAddr: A pointer to receive a linear address that can be used
 *  to access the physical memory, if the call is successful.
 * 
 *  @return: 0 if successful, a DPMI error code otherwise
 *      DPMI_SYS_INTEGRITY (DPMI host memory region)
 *      DPMI_INVALID_VALUE (address is below 1MB boundary)
 */
DPMISTATUS DpmiMapLinear(DWORD dwPhysAddr, DWORD dwRegSize, DWORD* pdwLinAddr) {
    __asm {
        mov ax, 800h                    ; DPMI call: Physical Address Mapping
        mov cx, word ptr [dwPhysAddr]   ; BX:CX = Physical Address
        mov bx, word ptr [dwPhysAddr+2] 
        mov di, word ptr [dwRegSize]    ; SI:DI = Size of region to map (bytes)
        mov si, word ptr [dwRegSize+2]
        int 31h
        jc failure                      ; Did this fail?
        mov edi, pdwLinAddr             ;   Nope, linear address is in BX:CX
        mov [edi], cx                   ; Store into pdwLinAddr
        mov [edi+2], bx

        failure:                        ;   Yes, it failed, AX = error
    }
}

/**
 *  DpmiDosAlloc procedure - Allocates a block of conventional memory from the
 *  DOS memory pool, usually used to exchange data with real-mode software.
 *  Internally, this calls INT 21H AH=48H.
 * 
 *  @param nParagraphs: The number of 16-byte blocks desired
 * 
 *  @param pwSegment: A pointer to receive the real-mode segment base address
 *  of the allocated block, if the call is successful.
 * 
 *  @param pwSelector: A pointer to receive the protected-mode selector for
 *  the allocated block, if the call is successful.
 * 
 *  @param pwLargestBlock: A pointer to receive the size of the largest 
 *  available block in paragraphs, if the call is unsuccessful.
 * 
 *  @return: 0 if the call is successful, a DPMI error code otherwise
 *      DPMI_MCB_DAMAGED
 *      DPMI_INSUFFICIENT_MEM
 *      DPMI_DESC_UNAVAILABLE
 */
DPMISTATUS DpmiDosAlloc(WORD wParagraphs, WORD* pwSegment, WORD *pwSelector, WORD* pwLargestBlock) {
    __asm {
        mov ax, 100h                ; DPMI call: Allocate DOS Memory Block
        mov bx, wParagraphs         ; BX = number of paragraphs desired
        int 31h
        jc failure                  ; Did this fail?
        mov edi, pwSegment          ;   Nope, *pwSegment = AX
        mov [edi], ax               
        mov edi, pwSelector
        mov [edi], dx               ;   *pwSelector = DX
        jmp done

        failure:                    ; Yes, it did fail
        mov edi, pwLargestBlock
        mov [edi], bx               ;   *pwLargestBlock = BX

        done:
    }
}

/**
 *  DpmiDosFree procedure - Frees a memory block previously allocated by
 *  DpmiDosAlloc.
 * 
 *  @param wSelector: The protected-mode selector of the block to be freed.
 * 
 *  @return: 0 if the call is successful, a DPMI error code otherwise
 *      DPMI_MCB_DAMAGED
 *      DPMI_INCORRECT_MEM_SEG
 *      DPMI_INVALID_SELECTOR
 */
DPMISTATUS DpmiDosFree(WORD wSelector) {
    __asm {
        mov ax, 101h                ; DPMI call: Free DOS Memory Block
        mov dx, wSelector           ; DX = Selector block to be freed
        int 31h
        jc failure                  ; Did the call fail?
        xor ax, ax                  ;   No, clear AX

        failure:                    ;   Yes, AX = error
    }
}

/**
 *  DpmiDosResize procedure - Changes the size of a memory block that was
 *  previously allocated with DpmiDosAlloc function.
 * 
 *  @param wNewBlockSize: The new block size in 16-byte paragraphs.
 * 
 *  @param wSelector: The protected-mode selector of the block to modify.
 * 
 *  @param pwMaxBlockSize: A pointer to receive the maximum possible block
 *  size in paragraphs, if the call is unsuccessful.
 * 
 *  @return: 0 if successful, a DPMI error code otherwise. This function may
 *  fail to increase the size of an existing DOS memory block due to subsequent
 *  DOS memory block allocations causing fragmentation of remaining DOS memory.
 *      DPMI_MCB_DAMAGED
 *      DPMI_INSUFFICIENT_MEM
 *      DPMI_INCORRECT_MEM_SEG
 *      DPMI_DESC_UNAVAILABLE
 *      DPMI_INVALID_SELECTOR
 */
DPMISTATUS DpmiDosResize(WORD wNewBlockSize, WORD wSelector, WORD* pwMaxBlockSize) {
    __asm {
        mov ax, 102h                ; DPMI call: Resize DOS Memory Block
        mov bx, wNewBlockSize       ; BX = New block size in paragraphs
        mov dx, wSelector           ; DX = Selector of block to modify
        int 31h
        jc failure                  ; Did the call fail?
        xor ax, ax                  ;   No, clear AX
        jmp done
    
        failure:                    ;   Yes, it did
        mov edi, pwMaxBlockSize
        mov [edi], bx               ;   *pwMaxBlockSize = BX

        done:
    }
}

/**
 *  DpmiGetRealModeIntVect procedure - Returns the contents of the current
 *  virtual machine's real-mode interrupt vector for the specified interrupt.
 * 
 *  @param intr: The interrupt number.
 * 
 *  @return: A segment:offset real-mode far pointer to the interrupt handler.
 */
DPMIFPTR16 DpmiGetRealModeIntVect(BYTE intr) {
    __asm {
        mov ax, 200h                ; DPMI call: Get Real Mode Interrupt Vector
        mov bl, intr                ; BL = interrupt number
        int 31h
        mov eax, ecx                ; Move CX:DX -> EAX
        shl eax, 10h
        mov ax, dx
    }
}

/**
 *  DpmiSetRealModeIntVect procedure - Sets the current virtual machine's real-
 *  mode interrupt vector for the specified interrupt.
 * 
 *  @param intr: The interrupt number.
 * 
 *  @param intvect: A segment:offset real-mode far pointer to the interrupt
 *  handler.
 */
void       DpmiSetRealModeIntVect(BYTE intr, DPMIFPTR16 intvect) {
    __asm {
        mov ax, 201h                ; DPMI call: Set Real Mode Interrupt Vector
        mov bl, intr
        mov dx, word ptr [intvect]  ; CX:DX = interrupt handler
        mov cx, word ptr [intvect+2]
        int 31h
    }
}

/**
 *  DpmiSimulateRealModeInt procedure - Simulates an interrupt in real-mode.
 *  The function transfers control to the address specified by the real-mode
 *  interrupt vector. The real-mode handler must return by executing an IRET.
 * 
 *  @param intr: The interrupt number.
 * 
 *  @param wStackCopyWords: The number of words to copy from the protected-mode
 *  stack to the real-mode stack.
 * 
 *  @param pRegs: A pointer to the real-mode register data structure.
 * 
 *  @return: 0 if the call was successful, a DPMI error code otherwise
 *      DPMI_LIN_MEM_UNAVAILABLE (stack)
 *      DPMI_PHYS_MEM_UNAVAILABLE (stack)
 *      DPMI_BACKING_STORE_UNAVAILABLE (stack)
 *      DPMI_INVALID_VALUE (wStackCopyWords too large)
 */
DPMISTATUS DpmiSimulateRealModeInt(BYTE intr, WORD wStackCopyWords, DPMIREGS* pRegs) {
    __asm {
        mov ax, 300h                ; DPMI call: Simulate Real Mode Interrupt
        mov bl, intr                ; BL = interrupt number
        xor bh, bh                  ; BH = flags
        mov cx, wStackCopyWords     ; CX = Number of words to copy from protected mode to real mode stack
        mov edi, pRegs              ; ES:EDI = Address of real mode register data structure
        int 31h
        jc failure                  ; Did the call fail?
        xor ax, ax                  ;   No, clear AX (no error)

        failure:                    ;   Yes, AX = error code
    }
}

/**
 * 
 */
DPMISTATUS DpmiCallRealModeProcFar(WORD wStackCopyWords, DPMIREGS* pRegs) {
    
    __asm {
        mov ax, 301h                ; DPMI call: Call Real Mode Procedure With Far Return Frame
        xor bh, bh                  ; BH = flags
        mov cx, wStackCopyWords     ; CX = Number of words to copy from protected mode to real mode stack
        mov edi, pRegs              ; ES:EDI = Address of real mode register data structure
        int 31h
        jc failure                  ; Did the call fail?
        xor ax, ax                  ;   No, clear AX (no error)

        failure:                    ;   Yes, AX = error code
    }
}
