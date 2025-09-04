#include <stdio.h>
#include "tokenize.h"
#include "dbgdef.h"
#include "expr.h"

int last_disp_sz = 0;
int last_enter_sz = 0;
int last_fill_sz = 0;

void DispMemCommand() {
	MEM_RANGE Range;
	ADDRESS Address;
	int end_tok;
	int status = GetRange(1, &end_tok, &Range);

	if (status != 0 && status != 3) {
		printf("Syntax error\n");
	}
	else {
		if (last_disp_sz == 0) {
			printf(" BYTE: ");
		}
		else if (last_disp_sz == 1) {
			printf(" WORD: ");
		}
		else {
			printf("DWORD: ");
		}
		printf("Dumping memory from (%d)%04X:%08X to (%d)%04X:%08X\n", Range.Start.Type, Range.Start.Selector, Range.Start.Offset, Range.End.Type, Range.End.Selector, Range.End.Offset);
	}
}

void FillMemCommand() {
	MEM_RANGE Range;
	int end_tok;
	int status = GetRange(1, &end_tok, &Range);

	if (status) {
		printf("Range error\n");
	}
	else {

	}
}

void EnterMemCommand() {
	ADDRESS Start;
	int end;
	uint32_t val;

	int status = GetAddress(1, &end, &Start);

	if (status) {
		printf("Syntax error\n");
		return;
	}

	printf("Writing to (%d)%04X:%08X: ", Start.Type, Start.Selector, Start.Offset);

	while (!GetExpr(end, &end, &val)) {
		printf("%08X ", val);
	}

	printf("\n");
}

void CompareMemCommand() {

}

void SearchMemCommand() {

}