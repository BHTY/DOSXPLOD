/**
 *      File: TYPES.H
 *      Type definitions for DOS extender
 *      Copyright (c) 2025 by Will Klees
 */

#ifndef __TYPES_H_
#define __TYPES_H_

typedef unsigned long   ULONG, *PULONG;
typedef unsigned long   DWORD, *PDWORD;
typedef signed long     LONG, *PLONG;
typedef signed short    SHORT, *PSHORT;
typedef unsigned short  WORD, *PWORD;
typedef unsigned int    UINT, *PUINT;
typedef signed int      INT, *PINT;
typedef signed char     CHAR, *PCHAR;
typedef unsigned char   BYTE, *PBYTE;
typedef void*           PVOID;
typedef BYTE            BOOL;

#define TRUE  1
#define FALSE 0

#ifndef NULL
#define NULL  ((PVOID)0)
#endif

#endif
