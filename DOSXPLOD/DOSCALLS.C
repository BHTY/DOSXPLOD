/**
 *      File: DOSCALLS.C
 *      C call interface for DOS flat mode API
 *      Copyright (c) 2025 by Will Klees
 */

#include "../DOSCALLS.H"
#include "../I386INS.H"

/**
 *  DllMainCRTStartup - Entry procedure for DLL, executed when image is loaded
 *  into memory. Simply returns 1, indicating success.
 * 
 *  @param hinstDll: Pointer to the base address where the image is loaded.
 * 
 *  @param fdwReason: 
 * 
 *  @param pReserved: Reserved, must be NULL.
 * 
 *  @return: TRUE for success, FALSE for failure.
 */
int __stdcall _DllMainCRTStartup(void* hinstDll, DWORD fdwReason, PVOID pReserved) {
    return 1;
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
        mov edx, ib             ; EDX = Low-order DWORD of number of bytes to move
        int 21h
        jc done                 ; If the call failed, AX = error code
        mov edi, ibActual       ; *ibActual = EAX (DX:AX)
        mov [edi], eax
        xor ax, ax              ; Call succeeded, clear AX (no error)

        done:
    }
}


