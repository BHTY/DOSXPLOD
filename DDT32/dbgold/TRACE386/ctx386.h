#pragma once
#include <stdint.h>

typedef struct _breakpoint {
	int exists;

	int bp_type; // 0 = software bp, 1 = hardware bp

	// for software breakpoint
	int enabled;
	uint16_t cs;
	uint32_t ip;
	uint8_t original_byte;

	// for hardware breakpoint
	int drx;
} breakpoint;

typedef struct _symbol {
	int implied_segment; // 0 for flat symbols, 1 for segmented symbols
	uint16_t segment;
	uint32_t offset;
	char* name;

	struct _symbol* next;
} symbol;

typedef struct _ctx386 {
	uint32_t dr0, dr1, dr2, dr3, dr6, dr7;
	uint32_t gs, fs, es, ds, cs, ss;
	uint32_t edi, esi, ebx, edx, ecx, eax, ebp, esp;
	uint32_t eflags;
	uint32_t eip;
} ctx386;

#define CARRY 0
#define PARITY 2
#define AC 4
#define ZERO 6
#define SIGN 7
#define TRAP 8
#define INTERRUPT 9
#define DIRECTION 10
#define OVERFLOW 11
#define NESTED 14
#define RESUME 16
#define V8086 17

#define GET_FLAG(flag)     ((ctx->eflags & (1 << (flag))) >> (flag))
#define PRINT_FLAG(flag, yes, no)	printf("%s ", GET_FLAG(flag) ? yes : no);

#define IOPL(flags) (((flags) & 0x3000) >> 12)

#define PE(cr0)		(cr0 & 0x1)

typedef struct _error386 {
	int error_type;
	int error_code;
	int error_subcode;
} error386;

// FAULT = IP points to the instruction causing an exception
// TRAP = IP points to following instruction
// In other words, a TRAP completes and then causes an exception while a FAULT fails first.

/* Division FAULT */
/* CS:IP points to instruction causing exception */
#define DIVIDE_FAULT 0		/* Divide Fault */

/* Debug Traps 
DR6[0..3] indicates which breakpoint condition caused the exception
DR7[16..17] = DR0 condition
DR7[20..21] = DR1 condition
DR7[24..25] = DR2 condition
DR7[28..29] = DR3 condition

Conditions: 00 = instruction fetch, 01 = write, 10 = I/O, 11 = R/W

Instruction Fetch => Instruction fetch FAULT (CS:IP points to the instruction trying to be fetched)
I/O Read/Write => I/O TRAP (CS:IP points to instruction after one causing exception)
Data Read/Write => Data TRAP (CS:IP points to instruction after one causing exception)
TF=1 => Single-step TRAP (CS:IP points to instruction after instruction causing exception)
	DR6[14] does the same
Software-triggered => Unexpected Single-step TRAP (CS:IP points to instruction after INT 01H)

Note that for instruction fetch faults, Windows XP and earlier versions do not set the Resume Flag
correctly, so it cannot be used to disable hardware breakpoints for one instruction. As a result,
the correct solution is similar to how software breakpoints are handled. The breakpoint is disabled,
the processor single-steps, breaks back into the debugger with a flag set, re-enables the breakpoint,
and keeps going.
*/
#define FETCH_FAULT 1			/* Breakpoint N hit */
#define IO_TRAP 2				/* Breakpoint N hit */
#define DATA_READ_TRAP 3		/* Breakpoint N hit */
#define DATA_WRITE_TRAP 4		/* Breakpoint N hit */
#define SINGLE_STEP_TRAP 5		/* Unexpected Trap 01 in code */
#define DEBUG_EXCEPTION 18		

/* NMI Interrupt */
/* CS:IP points to wherever it was when the NMI occured */
#define NMI 6					/* Hotkey struck */

/* Software Breakpoint TRAP */
/* CS:IP points to instruction after INT3 */
/* When the system breaks into the debugger, the INT3 is replaced by the original instruction and IP is decremented. */
/* Once you try to step past or go from that point, it will step once and then (using a flag) replace the breakpoint. */
#define BREAKPOINT_TRAP 7		/* Breakpoint N hit / Break instruction exception */

/* Overflow TRAP */
/* CS:IP points to instruction after INTO */
#define OVERFLOW_TRAP 8			/* Overflow Trap */

/* Bound FAULT */
/* CS:IP points to BOUND instruction */
#define BOUND_FAULT 9			/* Bound Fault */

/* Invalid Opcode FAULT */
/* CS:IP points to invalid instruction */
#define UD_FAULT 10				/* Undefined Opcode Fault */

/* Device Not Available FAULT */
/* CS:IP points to faulting instruction */
#define NM_FAULT 11				/* Device Not Available Fault */

/* Double FAULT */
/* CS:IP points to instruction causing double fault */
#define DOUBLE_FAULT 12			/* Double Fault */

/* Invalid TSS FAULT */
/* CS:IP points to either faulting instruction (if an exception occurs before loading segment selectors from TSS) or first instruction in new task */
/* Error code contains a selector error code */
#define TSS_FAULT 13			/* TSS Fault (selector) */

/* Segment Not Present FAULT */
/* Occurs when loading segment/gate with PRESENT bit=0 */
/* CS:IP points to instruction that tried to load the segment */
/* Error code contains segment selector index of segment descriptor causing exception */
#define NP_FAULT 14				/* Segment Not Present Fault (selector) */

/* Stack Segment FAULT */
/* CS:IP points to faulting instruction */
/* Occurs when stack limit check fails or you try to load an invalid stack segment */
/* Error code contains selector index if an invalid stack segment was tried to be loaded */
#define SS_FAULT 15				/* Stack Segment Fault (selector) */

/* General Protection FAULT */
/* CS:IP points to faulting instruction */
/* Usual Causes: */
/* - Executing privileged instruction with CPL>0 */
/* - Writing invalid data to CR0 */
/* - Segment error (privilege, read/write rights, limit, type) */
/* Error code contains segment selector index for segment-related exceptions */
#define GP_FAULT 16				/* General Protection Fault (selector) */

/* Page FAULT */
/* CS:IP points to faulting instruction */
/* Somehow, an address was accessed in a way that it's not allowed to be: */
/* Writing to read-only page */
/* Accessing not present page table/directory */
/* Accessing unmapped page */

/* Page Fault Error Code: */
/* Bit 0 = Present (set if page fault was caused by protection violation, unset for NP page) */
/* Bit 1 = Write (set for write access, unset for read access) */
/* Bit 2 = User (PF caused from CPL=3 if set -- doesn't necessarily mean this was a privilege violation) */
/* Bit 4 = Instruction Fetch (set if NX/XD is enabled and the page isn't valid code) */

/* CR2 = Virtual Address causing Page Fault */
#define PAGE_FAULT 17		/* Page fault OPERATing ADDRESS */

#define NEXT_THING 19

/* Selector Error Code */
/* Bit 0 is set if the exception originated externally to the CPU */
/* Bits 2-1 is 00 for GDT, 01 for IDT, 10 for LDT, 11 for IDT */
/* Bits 15-3 is the 13-bit selector index */

void DbgMain(ctx386* ctx, error386* error);
void DbgAddSym(int has_seg, uint16_t segment, uint32_t address, char* name);

uint32_t GetCr0();
uint32_t GetCr3();

// find next higher symbol
// find next lower symbol
