#include <stdio.h>
#include <string.h>
#include "tokenize.h"
#include "dbgdef.h"
#include "expr.h"
#include "memcmd.h"
#include "bpcmd.h"

#define TOKEN_STRVIEW(token, index)	(&((token)->tokens[index].str))
#define STRVIEW_STARTSWITH(str, str_view)		(memcmp(str, (str_view)->start, strlen(str)) == 0)
#define STRVIEW_EQUALS(str, str_view)			(((str_view)->len == strlen(str)) && STRVIEW_STARTSWITH(str, str_view))

void print_strview(STRING_VIEW* str_view) {
	int p;

	for (p = 0; p < str_view->len; p++) {
		printf("%c", str_view->start[p]);
	}
}

TOKENIZED* token_list;

void get_line(char* str, int len) {
	fgets(str, len, stdin);
}

int GetAddress(int starting_token, int *end_token, ADDRESS* Address) {
	TOKEN_VIEW view;
	STRING_VIEW* start = TOKEN_STRVIEW(token_list, starting_token);
	uint32_t value;
	int end, ret = 0;
		
	Address->Type = UNDEFINED;

	view.len = token_list->nTokens - starting_token;
	view.start = starting_token;
	view.toks = token_list;

	if (STRVIEW_EQUALS("@", start)) { /* Linear address */
		Address->Type = LINEAR;
		view.start++;
		view.len--;
	}
	else if (STRVIEW_EQUALS("%", start)) { /* Protected-mode segmented address */
		Address->Type = PMODE;
		view.start++;
		view.len--;
	}
	else if (STRVIEW_EQUALS("&", start)) { /* Real/V86-mode segmented address */
		Address->Type = RMODE;
		view.start++;
		view.len--;
	}

	if (Address->Type == UNDEFINED) { /* No prefix was given, guess based on current mode */
		Address->Type = RMODE;
	}

	end = find_expr(&view);
	if (end == 0xFFFFFFFF) {
		value = 1;
		ret = 3;
		goto OffsetOnly;
	}
	view.len = end - view.start;

	if (parse_expr(&view, &value, 0, NULL)) return 2;

	if (Address->Type == LINEAR) {
		Address->Offset = value;
		Address->Linear = value;
		*end_token = end;
		return 0;
	}
	else {
		if (STRVIEW_EQUALS(":", TOKEN_STRVIEW(token_list, end))) {
			Address->Selector = value;
			
			view.start = end + 1;
			view.len = token_list->nTokens - view.start;

			end = find_expr(&view);
			view.len = end - view.start;

			if (end == 0xFFFFFFFF) return 2;

			if (parse_expr(&view, &value, 0, NULL)) return 2;

			Address->Offset = value;
		}
		else { /* Only an offset was specified, use CS */
			OffsetOnly:

			Address->Selector = 0; /* CS */
			Address->Offset = value;
		}

		/* convert to linear */
		if (Address->Type == RMODE) {
			Address->Linear = (Address->Selector << 4) + Address->Offset;
		}
		else if (Address->Type == PMODE) {
			Address->Linear = Address->Offset;
		}

		*end_token = end;

		return ret;
	}
}

int GetRange(int starting_token, int* end_token, MEM_RANGE* Range) {
	int end, end2;
	int status;

	/* Try getting the first address */
	status = GetAddress(starting_token, &end, &(Range->Start));

	if (status == 3) { /* CS:IP */
		goto NoEnd;
	}
	else if (status) return 1;

	if (STRVIEW_EQUALS("l", TOKEN_STRVIEW(token_list, end))) {  /* Base L Count */
		uint32_t Count;

		if (GetExpr(end + 1, &end2, &Count)) return 1;

		Range->End = Range->Start;
		Range->End.Linear += Count;
		Range->End.Offset += Count;

		return 0;
	}

	status = GetAddress(end, &end2, &(Range->End));

	/* Now get the end */
	if (status == 2) /* Parsing failed */ {
		return 1;
	}
	else if (status) { /* There is no end, so it's start+16 */
		NoEnd:
		Range->End = Range->Start;
		Range->End.Linear += 16;
		Range->End.Offset += 16;
		*end_token = end;
		return 3;
	}
	else {
		*end_token = end2;
	}

	return 0;
}

int GetExpr(int starting_token, int* end_token, uint32_t* value) {
	TOKEN_VIEW view;
	
	view.len = token_list->nTokens - starting_token;
	view.start = starting_token;
	view.toks = token_list;

	if (starting_token >= token_list->nTokens) return 1;

	*end_token = find_expr(&view);

	if (*end_token == 0xFFFFFFFF) return 1;

	view.len = *end_token - view.start;

	return parse_expr(&view, value, 0, NULL);
}

void StackTraceCommand() {

}

typedef struct _X86_FLAG {
	int mask;
	char clear[3];
	char set[3];
} X86_FLAG;

X86_FLAG flag_names[] = { {1, "NC", "CY"}, {4, "PO", "PE"}, {0x10, "NA", "NC"}, {0x40, "NZ", "ZR"}, {0x80, "PL", "NG"}, {0x200, "DI", "EI"}, {0x400, "UP", "DN"}, {0x800, "NV", "OV"} };

void print_flags(uint32_t flags) {
	int i;

	for (i = sizeof(flag_names) / sizeof(flag_names[0]) - 1; i >= 0; i--) {
		if (flags & flag_names[i].mask) {
			printf("%s ", flag_names[i].set);
		}
		else {
			printf("%s ", flag_names[i].clear);
		}
	}
}

char* mips_reg[32] = {
	" $0",  "$at",  "$v0",  "$v1",  "$a0",  "$a1",  "$a2",  "$a3",
	"$t0",  "$t1",  "$t2",  "$t3",  "$t4",  "$t5",  "$t6",  "$t7",
	"$s0",  "$s1",  "$s2",  "$s3",  "$s4",  "$s5",  "$s6",  "$s7",
	"$t8",  "$t9",  "$k0",  "$k1",  "$gp",  "$sp",  "$fp",  "$ra"
};

void dump_regs_mips() {
	int i, p;

	printf("$pc: %08X  ", 0);

	for (i = 1; i < 32; i++) {

		if (i % 4 == 0) {
			printf("\n");
		}

		printf("%s: %08X  ", mips_reg[i], 0);
	}

	printf("\n");
}

void dump_regs_386() {
	printf("AX=%08X  BX=%08X  CX=%08X  DX=%08X  SI=%08X  DI=%08X\n", 0, 0, 0, 0, 0, 0);
	printf("IP=%08X  SP=%08X  BP=%08X  IOPL=%d        ", 0, 0, 0, 0);

	print_flags(0);

	printf("\nCS=%04X  SS=%04X  DS=%04X  ES=%04X  FS=%04X  GS=%04X  CR3=%05X   EFL=%06X\n", 0, 0, 0, 0, 0, 0, 0, 0);
}

void DisplayStatus() {
	dump_regs_386();

	/* print current symbol */
	/* disassemble current instruction */

	printf("ntdll!LdrpDoDebuggerBreak+0x4e:\n");
	printf("0028:8029CCF7    DIV    ECX\n");
}

void RegCommand() {
	if (0) { /* Second token is a register */
		if (0) { /* Third token is an equal sign, evaluate the expression! */

		}
		else { /* Just display the processor register */

		}
	}
	else { /* Display register file / processor status */
		DisplayStatus();
	}
}

void HelpCommand() {
	int end;
	ADDRESS Address;
	char c[2];

	int status = GetAddress(1, &end, &Address);

	if (status == 2) {
		printf("Syntax error\n");
	}
	else if (status == 0) {
		printf("Evaluate expression: %u = %08X\n", Address.Offset, Address.Offset);
	}
	else {
		printf("A [<address>] - assemble\n");
		printf("B[C|D|E] [<bps>] - clear/enable/disable breakpoint(s)\n");
		printf("BL - list breakpoints\n");
		printf("BA[id] <access> <size> [Options] <addr> - set processor breakpoint\n");
		printf("BP[id] [Options] <address> - set soft breakpoint\n");
		printf("C <range> <address> - compare memory\n");
		printf("D[type] [<range>] - display memory\n");
		printf("E[type] <address> [<values>] - enter memory values\n");
		printf("F <range> <list> - fill memory with pattern\n");
		printf("G [<address>] - go\n");
		printf("K - stacktrace\n");
		printf("LM - list loaded modules\n");
		printf("LN <expr> - list nearest symbols\n");
		printf("P [=<addr>] [<count=1>] - program step out\n");
		printf("Q - quit\n");
		printf("R [<reg> [=<expr>]] - view or set registers\n");
		printf("S <range> <list> - search memory for pattern\n");
		printf("T [=<addr>] [<count=1>] - trace / step in\n");
		printf("U [<range>] - unassemble\n");
		printf("X Wildcard - view symbols\n");
		printf("? <expr> - display expression\n");

		printf("\nHit enter...");
		get_line(c, 2);

		printf("\n");

		printf("<expr> unary ops: - ~\n");
		printf("       binary ops: + - * << >> & ^ |\n");
		printf("       comparisons: == < > !=\n");
		printf("       operands: number in hexadecimal, public symbol, <reg>\n");
		printf("<type> : b (byte), w (word), d (doubleword)\n");
		printf("<range>: <address> <address>\n");
		printf("         <address> L <count>\n");

		/* x86 options */
		printf("\n");
		printf("x86 options:\n");
		printf("DG [<list>] - dump selectors\n");
		printf("DIDT [<list>] - dump interrupt descriptors\n");
		printf("DIVT [<list>] - dump interrupt vectors\n");
		printf("<reg> : [e]ax, [e]bx, [e]cx, [e]dx, [e]si, [e]di, [e]bp, [e]sp, [e]ip, [e]fl,\n");
		printf("        al, ah, bl, bh, cl, ch, dl, dh, cs, ds, es, fs, gs, ss\n");
		printf("<flag>: iopl, of, df, if, tf, sf, zf, af, pf, cf\n");
		printf("<addr>: %%<protect-mode [seg:]address>\n");
		printf("        &<real/V86-mode [seg:]address>\n");
		printf("        @<linear address>\n");

		///* If not, try the Win32 command parser (load symbol, dump teb, peb, heap, kernel, gdi, user objects, crash dump, threads) */
			/* and write/read memory to/from file */

		/* Dump Win32 commands */

	}
}

int DebugLoop() {
	char buf[512];
	char old_buf[512];

	buf[0] = 0;
	old_buf[0] = 0;

	while (1) {
		printf("#");
		get_line(buf, 512);

		if (strlen(buf) > 1) {
			strcpy(old_buf, buf);
		}

		token_list->nTokens = 0;

		if (tokenize(old_buf, token_list)) {
			printf("Too many tokens!\n");
			continue;
		}

		if (token_list->nTokens >= 1) {
			STRING_VIEW* cmd = TOKEN_STRVIEW(token_list, 0);

			if (0) {

			}

			/* Display Memory Commands */
			else if (STRVIEW_EQUALS("db", cmd)) {
				last_disp_sz = 0;
				DispMemCommand();
			}
			else if (STRVIEW_EQUALS("dw", cmd)) {
				last_disp_sz = 1;
				DispMemCommand();
			}
			else if (STRVIEW_EQUALS("dd", cmd)) {
				last_disp_sz = 2;
				DispMemCommand();
			}
			else if (STRVIEW_EQUALS("d", cmd)) {
				DispMemCommand();
			}

			/* Fill Memory Commands */
			else if (STRVIEW_EQUALS("fb", cmd)) {
				last_fill_sz = 0;
				FillMemCommand();
			}
			else if (STRVIEW_EQUALS("fw", cmd)) {
				last_fill_sz = 1;
				FillMemCommand();
			}
			else if (STRVIEW_EQUALS("fd", cmd)) {
				last_fill_sz = 2;
				FillMemCommand();
			}
			else if (STRVIEW_EQUALS("f", cmd)) {
				FillMemCommand();
			}

			/* Enter Memory Commands */
			else if (STRVIEW_EQUALS("eb", cmd)) {
				last_enter_sz = 0;
				EnterMemCommand();
			}
			else if (STRVIEW_EQUALS("ew", cmd)) {
				last_enter_sz = 1;
				EnterMemCommand();
			}
			else if (STRVIEW_EQUALS("ed", cmd)) {
				last_enter_sz = 2;
				EnterMemCommand();
			}
			else if (STRVIEW_EQUALS("e", cmd)) {
				EnterMemCommand();
			}

			/* Compare Memory Command */
			else if (STRVIEW_EQUALS("c", cmd)) {
				CompareMemCommand();
			}

			/* Search Memory Command */
			else if (STRVIEW_EQUALS("s", cmd)) {
				SearchMemCommand();
			}

			/* Stack Trace */
			else if (STRVIEW_EQUALS("k", cmd)) {
				StackTraceCommand();
			}

			/* Breakpoint Commands */
			else if (STRVIEW_STARTSWITH("bp", cmd)) {
				BreakpointSetCommand();
			}
			else if (STRVIEW_EQUALS("ba", cmd)) {
				BreakpointAccessCommand();
			}
			else if (STRVIEW_EQUALS("bc", cmd)) {
				BreakpointClearCommand();
			}
			else if (STRVIEW_EQUALS("bd", cmd)) {
				BreakpointDisableCommand();
			}
			else if (STRVIEW_EQUALS("be", cmd)) {
				BreakpointEnableCommand();
			}
			else if (STRVIEW_EQUALS("bl", cmd)) {
				BreakpointListCommand();
			}

			/* Control Flow Commands */
			else if (STRVIEW_EQUALS("g", cmd)) {
				printf("Go not yet implemented!\n");
			}
			else if (STRVIEW_EQUALS("gu", cmd)) {
				printf("Step out not yet implemented!\n");
			}
			else if (STRVIEW_EQUALS("t", cmd)) {
				printf("Trace not yet implemented!\n");
			}
			else if (STRVIEW_EQUALS("p", cmd)) {
				printf("Program step not yet implemented!\n");
			}

			/* Register Commands */
			else if (STRVIEW_EQUALS("r", cmd)) {
				RegCommand();
			}

			/* Dump Descriptor Tables */
			else if (STRVIEW_EQUALS("dg", cmd)) {
				printf("Dump selectors\n");
			}
			else if (STRVIEW_EQUALS("didt", cmd)) {
				printf("Dump interrupt descriptor entries\n");
			}
			else if (STRVIEW_EQUALS("divt", cmd)) {
				printf("Dump interrupt vectors\n");
			}

			/* Examine Symbol Table */
			else if (STRVIEW_EQUALS("x", cmd)) {
				printf("Get a string to examine a symbol\n");
			}

			/* List Loaded Modules */
			else if (STRVIEW_EQUALS("lm", cmd)) {
				printf("List loaded modules\n");
			}

			/* List nearest symbols */
			else if (STRVIEW_EQUALS("ln", cmd)) {
				printf("List nearest symbols to address\n");
			}

			/* Unassemble */
			else if (STRVIEW_EQUALS("u", cmd)) {
				printf("Unassemble\n");
			}

			/* Assemble */
			else if (STRVIEW_EQUALS("a", cmd)) {
				printf("Assemble\n");
			}

			/* Help command */
			else if (STRVIEW_EQUALS("?", cmd)) {
				HelpCommand();
			}

			/* Quit command */
			else if (STRVIEW_EQUALS("q", cmd)) {
				printf("Quit\n");
			}

			/* If not, try the Win32 command parser (load symbol, dump teb, peb, heap, kernel, gdi, user objects, crash dump, threads) */
			/* and write/read memory to/from file */
			else {
				printf("Unknown\n");
			}

			print_tokenized(token_list);
		}
	}
}

int main() {
	token_list = alloc_tokenized(256);

	DebugLoop();
}
