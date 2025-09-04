#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ctx386.h"

char mode_prompts[] = { '>', '#', '-' };

char command[512];
char last_command[512] = { 0 };

breakpoint bps[32] = { 0 };
symbol* symbols;

uint32_t trapped_at = 0; /* Virtual Address where we trapped */
int which_bp = -1;

void DbgAddSym(int has_seg, uint16_t segment, uint32_t address, char* name) {
	
}

uint32_t FarToFlatPm(uint16_t seg, uint32_t offset) {
	// FIXME
	return offset;
}

uint32_t FarToFlatRm(uint16_t seg, uint32_t offset) {
	return (seg << 4) + offset;
}

int find_matching_bp(uint16_t cs, uint32_t eip) {
	return -1;
}

int correct_error(ctx386* ctx, error386* error) {
	return 0;
}

void display_segment(int error_code) {
	printf(" (");

	if (error_code & 0x2) { // IDT selector
		printf("IDT");
	}
	else {
		if (error_code & 0x4) { // LDT selector
			printf("LDT");
		}
		else { // GDT selector
			printf("GDT");
		}
	}

	printf("+%04X)\n", error_code & 0xFFF8);
}

void print_flags(ctx386* ctx) {
	PRINT_FLAG(OVERFLOW, "OV", "NV");
	PRINT_FLAG(DIRECTION, "DN", "UP");
	PRINT_FLAG(INTERRUPT, "EI", "DI");
	PRINT_FLAG(SIGN, "PL", "NG");
	PRINT_FLAG(ZERO, "ZR", "NZ");
	PRINT_FLAG(AC, "AC", "NA");
	PRINT_FLAG(PARITY, "PO", "PE");
	PRINT_FLAG(CARRY, "CY", "NC");
}

extern void* hProcess;
#include <windows.h>
#include "dis386.h"

void dump_state(ctx386* ctx, int cur_sym) {
	char buf[128];
	char str[128];

	printf("AX=%08X  BX=%08X  CX=%08X  DX=%08X  SI=%08X  DI=%08X\n", ctx->eax, ctx->ebx, ctx->ecx, ctx->edx, ctx->esi, ctx->edi);
	printf("IP=%08X  SP=%08X  BP=%08X  IOPL=%d        ", ctx->eip, ctx->esp, ctx->ebp, IOPL(ctx->eflags));

	// print eflags
	print_flags(ctx);
	printf("\n");

	printf("CS=%04X  SS=%04X  DS=%04X  ES=%04X  FS=%04X  GS=%04X  CR3=%05X   EFL=%06X\n", ctx->cs, ctx->ss, ctx->ds, ctx->es, ctx->fs, ctx->gs, GetCr3(), ctx->eflags);

	// print current symbol if possible
	if (cur_sym) {
		printf("ntdll!LdrpDoDebuggerBreak+2b:\n");
		printf("%04X:%08X    ", ctx->cs, ctx->eip);
	}

	ReadProcessMemory(hProcess, ctx->eip, buf, 16, NULL);
	disasm_opcode(str, buf, ctx->eip, 1, 1, 0, 0, 0);
	printf("%s\n", str);

	// disassemble current instruction
}

void DbgMain(ctx386* ctx, error386* error) {
	int running = 1;
	int mode = GET_FLAG(V8086) + PE(GetCr0());

	if (correct_error(ctx, error)) return;

	switch (error->error_type) {


		case BREAKPOINT_TRAP: {
			int bp_index = find_matching_bp(ctx->cs, ctx->eip-1);
			ctx->eip--;

			if (bp_index == -1) {
				printf("Break instruction exception\n");
				break;
			}

			printf("Breakpoint %d hit\n", bp_index);

			// Temporarily disable the breakpoint
			which_bp = bp_index;

			break;
		}
		case DIVIDE_FAULT:
			printf("Divide Fault\n");
			break;
		case NMI:
			printf("Hotkey struck\n");
			break;
		case OVERFLOW_TRAP:
			printf("Overflow Trap\n");
			break;
		case BOUND_FAULT:
			printf("Bound Fault\n");
			break;
		case UD_FAULT:
			printf("Undefined Opcode Fault\n");
			break;
		case NM_FAULT:
			printf("Device Not Available Fault\n");
			break;
		case DOUBLE_FAULT:
			printf("Double Fault\n");
			break;
		case TSS_FAULT:
			printf("TSS Fault");
			display_segment(error->error_code);
			break;
		case NP_FAULT:
			printf("Segment Not Present Fault");
			display_segment(error->error_code);
			break;
		case SS_FAULT:
			printf("Stack Segment Fault");
			display_segment(error->error_code);
			break;
		case GP_FAULT:
			printf("General Protection Fault");
			display_segment(error->error_code);
			break;
		case DEBUG_EXCEPTION:
			printf("fuck\n");
			break;
		case PAGE_FAULT:
			printf("Page fault ");

			if (error->error_code & 0x10) {
				printf("execut");
			}
			else if (error->error_code & 0x1) {
				printf("writ");
			} else {
				printf("read");
			}

			printf("ing linear address %08X", error->error_subcode);
			break;
	}

	dump_state(ctx, 1);

	while (running) {
		printf("%c", mode_prompts[mode]);
		fgets(command, 512, stdin);

		ctx->eflags |= 0x100;

		if (command[0] != '\n') strcpy(last_command, command);

		running = 0;
	}
}
