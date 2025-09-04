#pragma once

#include <stdint.h>

typedef enum _ADDR_TYPE {
	UNDEFINED = -1,
	LINEAR,
	RMODE,
	PMODE
} ADDR_TYPE;

typedef struct _ADDRESS {
	ADDR_TYPE	 Type;
	uint16_t Selector;
	uint32_t Offset;
	uint32_t Linear;
} ADDRESS;

typedef struct _MEM_RANGE {
	ADDRESS Start;
	ADDRESS End;
} MEM_RANGE;

typedef struct _BREAKPOINT {
	int Type; /* 0 = None, 1 = Software Breakpoint, 2 = Hardware Breakpoint */
	int Enabled; /* 0 = Disabled, 1 = Enabled */
	ADDRESS Address;
	uint32_t OriginalValue; /* stores type, size, and debug register for HW BP */
	int Data; /* extra data */
} BREAKPOINT;

typedef struct _DEBUG_SYMBOL {
	char Name[256];
	int Type; /* 0=Offset, 1=Seg:Offs */
	ADDRESS Address;
} DEBUG_SYMBOL;
