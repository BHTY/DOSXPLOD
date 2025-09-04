#include "bpcmd.h"

BREAKPOINT Breakpoints[NUM_BPS];

void PrintBreakpoint(int id) {
	BREAKPOINT* bp = &Breakpoints[id];

	if (1 || bp->Type) {
		if (bp->Enabled) {
			printf("*");
		}
		else {
			printf(" ");
		}

		printf(" %2d: ", id);

		/* Print Address */
		printf("0000:00000000");

		if (bp->Type == 2) { /* Software breakpoint */
		}
		else {
			printf("     ");
		}

		/* Print symbol */
		printf("Sym");

		printf("\n");
	}
}

void BreakpointSetCommand() {

}

void BreakpointAccessCommand() {

}

void BreakpointClearCommand() {

}

void BreakpointDisableCommand() {

}

void BreakpointEnableCommand() {

}

void BreakpointListCommand() {

}