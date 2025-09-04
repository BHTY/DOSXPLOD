/**
 *      File: BIOCALLS.H
 *      Function prototypes and C call interface for BIOS flat mode API
 *      Copyright (c) 2025 by Will Klees
 */

#ifndef __BIOCALLS_H_
#define __BIOCALLS_H_

#include "TYPES.H"

/* Video BIOS */
typedef struct _CUR_POS_SHAPE {
    BYTE cStartScanLine, cEndScanLine, cRow, cCol;
} CUR_POS_SHAPE, *PCUR_POS_SHAPE;

typedef struct _CUR_VID_MODE {
    BYTE cVidMode, cNumCols, cActivePg;
} CUR_VID_MODE, *PCUR_VID_MODE;

typedef struct _LIGHT_PEN_STATUS {
    BYTE cStatus;
} LIGHT_PEN_STATUS, *PLIGHT_PEN_STATUS;

BYTE          VioSetMode(BYTE cVideoMode);
void          VioSetCursorShape(BYTE cScanRowStart, BYTE cScanRowEnd);
void          VioSetCursorPosition(BYTE cPgNum, BYTE cRow, BYTE cCol);
CUR_POS_SHAPE VioGetCursorPositionAndShape(BYTE cPgNum);
void          VioReadLightPen(PLIGHT_PEN_STATUS pPenStatus);
void          VioSetActivePage(BYTE cPgNum);
void          VioScrollUp  (BYTE cScroll, BYTE cAttr, BYTE cUpperRowNum, BYTE cLeftColNum, BYTE cLowerRowNum, BYTE cRightColNum);
void          VioScrollDown(BYTE cScroll, BYTE cAttr, BYTE cUpperRowNum, BYTE cLeftColNum, BYTE cLowerRowNum, BYTE cRightColNum);
WORD          VioReadChar(BYTE cPgNum);
void          VioWriteCharAttr(BYTE cChar, BYTE cPgNum, BYTE cAttr, WORD wRep);
void          VioWriteChar(BYTE cChar, BYTE cPgNum, WORD wRep);
void          VioSetBgColor(BYTE cColor);
void          VioSetPalette(BYTE cID);
void          VioWritePixel(BYTE cColor, BYTE cPgNum, WORD x, WORD y);
BYTE          VioReadPixel(BYTE cPgNum, WORD x, WORD y);
void          VioTtyOutput(BYTE cChar, BYTE cPgNum, BYTE cColor);
void          VioGetMode(PCUR_VID_MODE* pMode);
void          VioSetCharSet(BYTE cBytesPerChar, WORD wNumChars, WORD wFirstChar, BYTE* pData);
void          VioWriteString(BYTE cWriteMode, BYTE cPgNum, BYTE cColor, WORD wLen, BYTE cRow, BYTE cCol, BYTE* pStr);

/* Disk BIOS */
typedef struct _DRIVE_PARAMS {
    BYTE cNumHardDisks, cLastHeadIndex;
    WORD cLastCylinderIndex;
    BYTE cSectorsPerTrack;
    BYTE cDriveType;
    BYTE* pDPT;
} DRIVE_PARAMS, *PDRIVE_PARAMS;

BYTE  BioResetDisks(BYTE cDrive);
BYTE  BioGetStatus(BYTE cDrive);
WORD  BioReadSectors  (BYTE cSectors, BYTE cCylinder, BYTE cSector, BYTE cHead, BYTE cDrive, BYTE* pBuffer);
WORD  BioWriteSectors (BYTE cSectors, BYTE cCylinder, BYTE cSector, BYTE cHead, BYTE cDrive, BYTE* pBuffer);
WORD  BioVerifySectors(BYTE cSectors, BYTE cCylinder, BYTE cSector, BYTE cHead, BYTE cDrive, BYTE* pBuffer);
BYTE  BioFormatTrack  (BYTE cSectors, BYTE cCylinder, BYTE cSector, BYTE cHead, BYTE cDrive, BYTE* pBuffer);
BYTE  BioFormatTrackSetBadSectorFlags(BYTE cInterleave, BYTE cTrack, BYTE cSector, BYTE cHead, BYTE cDrive);
BYTE  BioFormatDriveFromTrack(BYTE cInterleave, BYTE cTrack, BYTE cSector, BYTE cHead, BYTE cDrive);
BYTE  BioReadDriveParams(BYTE cDrive, PDRIVE_PARAMS pParams);

WORD  DskReadSectors (BYTE cDrive, PDRIVE_PARAMS pParams, DWORD dwLBA, DWORD dwSectors, BYTE* pBuffer);
WORD  DskWriteSectors(BYTE cDrive, PDRIVE_PARAMS pParams, DWORD dwLBA, DWORD dwSectors, BYTE* pBuffer);
void  DskLBAtoCHS(PDRIVE_PARAMS pParams, DWORD dwLBA, BYTE* pcCylinder, BYTE* pcSector, BYTE* pcHead);
DWORD DskCHStoLBA(PDRIVE_PARAMS pParams, BYTE cCylinder, BYTE cSector, BYTE cHead);

/* Keyboard BIOS */
#define KBD_ASCII(w)        ((w) & 0xFF)
#define KBD_SCANCODE(w)     ((w) >> 8)
WORD         KbdRead();
WORD         KbdHit();
BYTE         KbdGetState();
void         KbdSetRepeat(BYTE cMode, BYTE cDelay, BYTE cTypematicRate);
BYTE         KbdSimulate(BYTE cScancode, BYTE cAscii);
BYTE         KbdGetId();

/* Printer BIOS */
BYTE         PrnPrint(BYTE cChar, WORD wPort);
BYTE         PrnSetup(WORD wPort);
BYTE         PrnQuery(WORD wPort);

#endif
