/**
 *      File: DOSXPLOD.H
 *      Function prototypes and C call interface for C4 system export API
 *      Copyright (c) 2025 by Will Klees
 */

#ifndef __DOSXPLOD_H_
#define __DOSXPLOD_H_

#include "TYPES.H"
#include <stdarg.h>

/* Handy macros */
#define LOBYTE(word)        ((word) & 0xFF)
#define HIBYTE(word)        (((word) >> 8) & 0xFF)
#define LOWORD(dword)       ((dword) & 0xFFFF)
#define HIWORD(dword)       (((dword) >> 16) & 0xFFFF)

/* Typedefs */
typedef DWORD SYSRESULT;

/* Error codes */
#define SYSERR_SUCCESS                          0
#define SYSERR_INSUFFICIENT_MEMORY              1
#define SYSERR_IO_ERROR                         2
#define SYSERR_IMG_FORMAT                       3
#define SYSERR_IMG_MACHINE_TYPE                 4
#define SYSERR_IMG_RELOCS                       5
#define SYSERR_IMG_ENTRY_FAILED                 6
#define SYSERR_IMG_MISSING_DEPENDENCY           7
#define SYSERR_IMG_MISSING_IMPORT               8
#define SYSERR_IMG_MISSING                      9
#define SYSERR_IMG_BAD_RELOC_TYPE               10
#define SYSERR_NOT_EXE                          11

/* Exception handling frame structure */
typedef struct _EXCEPT_CONTEXT {
    DWORD FS, GS, ES, DS; /* Segment registers */
    DWORD EDI, ESI, EBP, OldESP, EBX, EDX, ECX, EAX; /* PUSHAD frame */
    DWORD ExceptionNumber;
    DWORD ReturnEIP, ReturnCS, ErrorCode;
    DWORD EIP, CS, EFLAGS, ESP, SS;
} EXCEPT_CONTEXT, *PEXCEPT_CONTEXT;

typedef void (cdecl *PEXCEPTION_HANDLER)(PEXCEPT_CONTEXT pContext);

/**
 *  int03 handler
 *      push 3                  ; Push exception number
 *      jmp except_handler
 * 
 *  except_handler
 *      pushad
 *      push ds                 ; Push segment registers
 *      push es
 *      push gs
 *      push fs
 *      push esp                ; Push pointer to exception frame
 *      call [handler_chain]
 *      add esp, 4              ; Pop pointer to exception frame
 *      pop fs                  ; Pop segment registers
 *      pop gs
 *      pop es
 *      pop ds
 *      popad
 *      add esp, 4              ; Pop exception number
 *      retfd
 * 
 *  exception handler has signature: void cdecl handler(PEXCEPT_CONTEXT pContext)
 */

/* Memory manager */
PVOID     SysMemAlloc(DWORD dwLen);
PVOID     SysMemReAlloc(PVOID ptr, DWORD dwNewLen);
void      SysMemFree(PVOID ptr);

/* Image loader */
SYSRESULT SysLoadLibrary(CHAR* pszLibName, PVOID* ppvModule);
BOOL      SysFreeLibrary(PVOID pModule);
PVOID     SysGetModuleHandle(CHAR* pszModuleName);
PCHAR     SysGetModuleFileName(PVOID pModule);
PVOID     SysGetProcAddress(PVOID pModule, CHAR* pszProcName);

/* Misc */
void               SysExit(DWORD dwExitCode);
DWORD              SysGetVersion();
void               SysLogError(char* fmt, ...);
PCHAR              SysGetCommandLine();
PEXCEPTION_HANDLER SysSetExceptionHandler(PEXCEPTION_HANDLER pHandler);

/* Useful C string functions */
int memcmp(const void * ptr1, const void * ptr2, UINT num);
int strcmp(const char* str1, const char* str2);
int stricmp(const char* str1, const char* str2);
int strncmp(const char * string1, const char * string2, UINT count);
int strnicmp(const char * string1, const char * string2, UINT count);
char * strcpy(char * destination, const char * source);
char * strncpy(char * destination, const char * source, UINT num);
UINT strlen(const char * str);
int vsprintf(char * s, const char * format, va_list arg);

#define DLL_PROCESS_ATTACH 1
#define DLL_PROCESS_DETACH 2

#endif
