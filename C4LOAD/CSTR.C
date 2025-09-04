/**
 *      File: CSTR.C
 *      C string functions subset
 *      Copyright (c) 2025 by Will Klees
 */

#include <DOSXPLOD.H>

int memcmp(const void * ptr1, const void * ptr2, size_t num);
int strcmp(const char* str1, const char* str2);
int stricmp(const char* str1, const char* str2);
int strncmp(const char * string1, const char * string2, size_t count);
int strnicmp(const char * string1, const char * string2, size_t count);
char * strcpy(char * destination, const char * source);
char * strncpy(char * destination, const char * source, size_t num);
size_t strlen(const char * str);
int vsprintf(char * s, const char * format, va_list arg);