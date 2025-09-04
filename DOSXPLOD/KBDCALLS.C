/**
 *      File: KBDCALLS.C
 *      C call interface for keyboard BIOS
 *      Copyright (c) 2025 by Will Klees
 */

#include "../BIOCALLS.H"

/**
 *  KbdRead procedure - Waits for a keypress to be made available from the
 *  keyboard buffer and reads it.
 * 
 *  @return: Returns the ASCII character of the button pressed in the low
 *  byte of the returned word, and the scan code of the key pressed down
 *  in the high byte.
 */
WORD         KbdRead() {
    __asm {
        xor ah, ah          ; Keyboard BIOS: Read key press
        int 16h
    }
}

/**
 *  KbdHit procedure - Peeks the state of the keyboard buffer.
 * 
 *  @return: 0 if there are no presses in the keyboard buffer. Otherwise, it
 *  returns the ASCII character of the top button pressed in the low byte of
 *  the returned word, and the scan code in the high byte.
 */
WORD         KbdHit() {
    __asm {
        mov ah, 1       ; Keyboard BIOS: Get keyboard buffer state
        int 16h
        
    }
}

/**
 *  KbdGetState - Returns the BIOS keyboard flags.
 * 
 *  @return: Returns a bitmask described below
 *      Bit 0 -> Right shift key depressed
 *      Bit 1 -> Left shift key depressed
 *      Bit 2 -> CTRL key depressed
 *      Bit 3 -> ALT key depressed
 *      Bit 4 -> Scroll-lock is active
 *      Bit 5 -> Num-lock is active
 *      Bit 6 -> Caps-lock is active
 *      Bit 7 -> Insert is active
 */
BYTE         KbdGetState() {
    __asm {
        mov ah, 2   ; Keyboard BIOS: Get the state of the keyboard
        int 16h     
    }
}

/**
 * 
 */
void         KbdSetRepeat(BYTE cMode, BYTE cDelay, BYTE cTypematicRate) {
    __asm {

    }
}

/**
 * 
 */
BYTE         KbdSimulate(BYTE cScancode, BYTE cAscii) {
    __asm {

    }
}

/**
 * 
 */
BYTE         KbdGetId() {
    __asm {
        mov ah, 0Ah
        int 16h
    }
}
