/**
 *      File: VIOCALLS.C
 *      C call interface for video BIOS
 *      Copyright (c) 2025 by Will Klees
 */

#include "../BIOCALLS.H"

/**
 *  VioSetMode procedure - Sets the display mode.
 * 
 *  @param cVideoMode: The video mode to set.
 * 
 *  @return: The video mode flag / CRT controller mode byte.
 */
BYTE          VioSetMode(BYTE cVideoMode) {
    __asm {
        mov ah, 0
        mov al, cVideoMode
        int 10h
    }
}
