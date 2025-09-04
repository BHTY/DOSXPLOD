#include <stdio.h>
#include <string.h>
#include <DOSCALLS.H>
#include <DPMI.H>
#include <DOSXPLOD.H>
#include <EXE.H>
#include <I386INS.H>


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
        mov edi, pdwLinAddr             ;   *pdwLinAddr = BX:CX
        mov [edi], cx
        mov [edi+2], bx
        mov edi, phBlock                ;   *phBlock = SI:DI
        mov [edi], di
        mov [edi+2], si

        done:
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

        done:
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
 *  DosExit procedure - Terminates the current process.
 * 
 *  @param cExitCode: Return code
 */
void      DosExit(BYTE cRetCode) {
    __asm {
        mov ah, 4ch                ; DOS Entry Point - Terminate Process With Return Code
        mov al, cRetCode           ; AL = Return code
        int 21h
    }
}

/**
 *  DosOpen procedure - Opens a file or device using a handle.
 * 
 *  @param pszName: Pointer to a null-terminated path representing the file or
 *  device.
 * 
 *  @param iMode: The mode in which to open the file: FILE_READ, FILE_WRITE,
 *  or FILE_RDWR.
 * 
 *  @param pHf: Pointer to receive a file handle on success.
 * 
 *  @return: An MS-DOS error code, or 0 if the file opens successfully.
 */
DOSSTATUS DosOpen(CHAR* pszName, BYTE iMode, HFILE* pHf) {
    __asm {
        mov edx, pszName        ; DS:EDX <- ASCIIZ filename
        mov al, iMode           ; AL <- Mode
        mov ah, 3dh             ; DOS Entry Point - Open File Handle
        int 21h
        jc done                 ; If call failed, AX = error code
        mov edi, pHf            
        mov [edi], ax           ; *pHf = file handle
        xor ax, ax              ; Call succeeded, clear AX (no error)

        done:
    }
}

/**
 *  DosClose procedure - Closes a file using a handle.
 * 
 *  @param hFile: The file handle to close.
 * 
 *  @return: An MS-DOS error code, or 0 if the file was closed successfully.
 */
DOSSTATUS DosClose(HFILE hFile) {
    __asm {
        mov ah, 3eh             ; DOS Entry Point - Close File Handle
        mov bx, hFile           ; BX <- File handle
        int 21h
        jc done                 ; If call failed, AX = error code
        xor ax, ax              ; Call succeeded, clear AX

        done:
    }
}

/**
 *  DosRead procedure - Reads from a file or device using a handle.
 * 
 *  @param hFile: The file handle to read from.
 * 
 *  @param pBuffer: A pointer to the read buffer.
 * 
 *  @param cbRead: The number of bytes to read.
 * 
 *  @param pcbActual: A pointer to receive the number of bytes read if the
 *  call is successful.
 * 
 *  @return: An MS-DOS error code, or 0 if the file is read from successfully.
 */
DOSSTATUS DosRead(HFILE hFile, PVOID pBuffer, ULONG cbRead, ULONG* pcbActual) {
    __asm {
        mov ah, 3fh             ; DOS Entry Point - Read from File Handle
        mov bx, hFile           ; BX <- File handle
        mov ecx, cbRead         ; ECX <- Number of bytes to read
        mov edx, pBuffer        ; EDX <- Pointer to read buffer
        int 21h
        jc done                 ; If the call failed, AX = error code
        mov edi, pcbActual      ; *pcbActual = number of bytes read
        mov [edi], eax
        xor ax, ax              ; Call succeeded, clear AX (no error)

        done:
    }
}

/**
 *  DosWrite procedure - Writes to a file or device using a handle.
 * 
 *  @param hFile: The file handle to write to.
 * 
 *  @param pBuffer: A pointer to the write buffer.
 * 
 *  @param cbRead: The number of bytes to write, a zero value truncates/extends
 *  the file to the current file position.
 * 
 *  @param pcbActual: A pointer to receive the number of bytes written if the
 *  call is successful.
 * 
 *  @return: An MS-DOS error code, or 0 if the file is written successfully.
 */
DOSSTATUS DosWrite(HFILE hFile, PVOID pBuffer, ULONG cbRead, ULONG* pcbActual) {
    __asm {
        mov ah, 40h             ; DOS Entry Point - Write to File Handle
        mov bx, hFile           ; BX <- File handle
        mov ecx, cbRead         ; ECX <- Number of bytes to write
        mov edx, pBuffer        ; EDX <- Pointer to write buffer
        int 21h
        jc done                 ; If the call failed, AX = error code
        mov edi, pcbActual
        mov [edi], eax          ; *pcbActual = number of bytes written
        xor ax, ax              ; Call succeeded, clear AX (no error)

        done:
    }
}

/**
 *  DosSetFilePtr procedure - Moves the file pointer using handle.
 * 
 *  @param hFile: The file handle.
 * 
 *  @param ib: The number of bytes to move
 * 
 *  @param method: The origin of the move
 *      SEEK_SET: Beginning of file plus offset
 *      SEEK_CUR: Current location plus offset
 *      SEEK_END: End of file plus offset
 * 
 *  @param ibActual: A pointer to receive the new file pointer location if
 *  the call is successful.
 * 
 *  @return: An MS-DOS error code, or 0 if the file pointer is changed
 *  successfully.
 */
DOSSTATUS DosSetFilePtr(HFILE hFile, LONG ib, BYTE method, ULONG* ibActual) {
    __asm {
        mov ah, 42h             ; DOS Entry Point - Move File Pointer Using Handle
        mov al, method          ; AL = Origin of move
        mov bx, hFile           ; BX = File handle
        xor ecx, ecx
        mov dx, word ptr [ib]   ; CX:DX = DWORD of number of bytes to move
        mov cx, word ptr [ib+2]
        int 21h
        jc done                 ; If the call failed, AX = error code
        mov edi, ibActual       ; *ibActual = DX:AX
        mov [edi], ax
        mov [edi+2], dx
        xor ax, ax              ; Call succeeded, clear AX (no error)

        done:
    }
}

/**
 *  VioSetCursorPos procedure - Sets the position of the cursor
 * 
 *  @param cPgNum: 
 * 
 *  @param cRow: 
 * 
 *  @param cCol: 
 * 
 *  @return: 
 */
void VioSetCursorPosition(BYTE cPgNum, BYTE cRow, BYTE cCol) {
    __asm {
        mov ah, 2
        mov bh, cPgNum
        mov dh, cRow
        mov dl, cCol
        int 10h
    }
}

