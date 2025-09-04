/**
 *      File: MOUCALLS.H
 *      Function prototypes and C call interface for Microsoft Mouse Int33 API
 *      Copyright (c) 2025 by Will Klees
 */

#ifndef __MOUCALLS_H_
#define __MOUCALLS_H_

#include "TYPES.H"

/* Mouse status structure */
typedef struct _MOUSTATUS {
    WORD X, Y, Buttons;
} MOUSTATUS;

/* Mouse driver version structure */
typedef struct _MOUVERSION {
    BYTE MajorVersion, MinorVersion, MouseType, IRQNumber;
} MOUVERSION;

/* Mouse sensitivity structure */
typedef struct _MOUSENS {
    WORD Horiz, Vert, dblSpdThreshold;
} MOUSENS;

/* Mouse button press/release info structure */
typedef struct _MOUBUTINFO {
    WORD wCount, wHoriz, wVert, wStatus;
} MOUBUTINFO;

/* Mouse motion counter structure */
typedef struct _MOUMOTCNTR {
    SHORT iHoriz, iVert;
} MOUMOTCNTR;

/* Mouse entry points */
DWORD       MouReset();
void        MouShow();
void        MouHide();
void        MouGetPos(MOUSTATUS* pStatus);
void        MouSetPos(WORD x, WORD y);
void        MouGetButtonPressInfo(WORD wButton, MOUBUTINFO* pInfo);
void        MouGetButtonReleaseInfo(WORD wButton, MOUBUTINFO* pInfo);
void        MouSetHorizExtents(WORD wMin, WORD wMax);
void        MouSetVertExtents(WORD wMin, WORD wMax);
void        MouSetGraphicCursor(WORD wHotX, WORD wHotY, BYTE* pMasks);
void        MouSetTextCursor(WORD wCurType, WORD wStartMask, WORD wEndMask);
MOUMOTCNTR  MouReadMickeys();

/**
 *      INT 33,C - Set Mouse User Defined Subroutine and Input Mask
 *      ES:DX = far pointer to user interrupt
 *      X = user interrupt mask:

	   |F-5|4|3|2|1|0| user interrupt mask in CX
	     |	| | | | `--- cursor position changed
	     |	| | | `---- left button pressed
	     |	| | `----- left button released
	     |	| `------ right button pressed
	     |	`------- right button released
	     `--------- unused

	returns nothing


	- routine at ES:DX is called if an event occurs and the
	  corresponding bit specified in user mask is set
	- routine at ES:DX receives parameters in the following
	  registers:

	  AX = condition mask causing call
	  CX = horizontal cursor position
	  DX = vertical cursor position
	  DI = horizontal counts
	  SI = vertical counts
	  DS = mouse driver data segment
	  BX = button state:

	     |F-2|1|0|
	       |  | `--- left button (1 = pressed)
	       |  `---- right button (1 = pressed)
	       `------ unused

	- initial call mask and user routine should be restore on exit
	  from user program
	- user program may need to set DS to it's own segment
 */

void        MouEnableLightPenEmulation();
void        MouDisableLightPenEmulation();
void        MouSetMickeyPixelRatio(WORD wHoriz, WORD wVert);
void        MouConditionalOff(WORD wUpperX, WORD upperY, WORD lowerX, WORD lowerY);
void        MouSetDblSpeedThreshold(WORD wThreshold);
/**
 * INT 33,14 - Swap Interrupt Subroutines


	AX = 14h
	ES:DX = far pointer to user routine
	CX = user interrupt mask:

		|F-8|7|6|5|4|3|2|1|0| user interrupt mask in CX
		  |  | | | | | | | `--- cursor position changed
		  |  | | | | | | `---- left button pressed
		  |  | | | | | `----- left button released
		  |  | | | | `------ right button pressed
		  |  | | | `------- right button released
		  `--------------- unused

	on return:
	CX = previous user interrupt mask
	ES:DX = far pointer to previous user interrupt

	- routine at ES:DX is called if an event occurs and the
	  corresponding bit specified in user mask is set
	- routine at ES:DX receives parameters in the following
	  registers:

	  AX = condition mask causing call
	  CX = horizontal cursor position
	  DX = vertical cursor position
	  DI = horizontal counts
	  SI = vertical counts
	  DS = mouse driver data segment
	  BX = button state:

	     |F-2|1|0|
	       |  | `--- left button (1 = pressed)
	       |  `---- right button (1 = pressed)
	       `------ unused

	- initial call mask and user routine should be restore on exit
	  from user program
	- user program may need to set DS to it's own segment
	- see	INT 33,C
 */
WORD        MouGetMemoryNeeded();
void        MouSaveState(PBYTE pState);
void        MouRestoreState(PBYTE pState);
/**
 * INT 33,18 - Set alternate subroutine call mask and address


	AX = 18h
	DX = offset to function
	CX = user interrupt mask:

		|F-8|7|6|5|4|3|2|1|0| user interrupt mask in CX
		  |  | | | | | | | `--- alt key pressed during event
		  |  | | | | | | `---- ctrl key pressed during event
		  |  | | | | | `----- shift key pressed during event
		  |  | | | | `------ right button up event
		  |  | | | `------- right button down event
		  |  | | `-------- left button up event
		  |  | `--------- left button down event
		  |  `---------- cursor moved
		  `------------ unused

	- entire mask is set to zero when INT 33,0 is called
	- up to three handlers may be defined with this call
	- mask should be cleared before program exit
	- when handler is called it receives parameters in the following
	  registers:

	  AX = condition mask causing call
	  CX = horizontal cursor position
	  DX = vertical cursor position
	  DI = horizontal counts
	  SI = vertical counts
	  DS = mouse driver data segment
	  BX = button state:

	     |F-2|1|0|
	       |  | `--- left button (1 = pressed)
	       |  `---- right button (1 = pressed)
	       `------ unused


 */

/**
 * INT 33,19 - Get User Alternate Interrupt Address


	AX = 19h
	CX = user interrupt call mask (see below)


	on return:
	BX:DX = user interrupt vector
	CX = user interrupt call mask or zero if not found

		|F-8|7|6|5|4|3|2|1|0| user interrupt mask in CX
		  |  | | | | | | | `--- alt key pressed during event
		  |  | | | | | | `---- ctrl key pressed during event
		  |  | | | | | `----- shift key pressed during event
		  |  | | | | `------ right button up event
		  |  | | | `------- right button down event
		  |  | | `-------- left button up event
		  |  | `--------- left button down event
		  |  `---------- cursor moved
		  `------------ unused

	- returns vector to function defined by INT 33,18
	- searches the event handlers defined by INT 33,18 for a routine
	  with a call mask matching CX
 */

void        MouGetSensitivity(MOUSENS* pSensitivity);
void        MouSetSensitivity(WORD Horiz, WORD Vert, WORD dblSpdThreshold);
void        MouDisable();
void        MouEnable();
DWORD       MouResetSoftware();
void        MouSetLang(WORD wLang);
WORD        MouGetLang();
MOUVERSION  MouGetVersion();

#endif
