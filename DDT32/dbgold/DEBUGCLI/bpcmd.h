#pragma once
#include "dbgdef.h"

#define NUM_BPS 32

extern BREAKPOINT Breakpoints[NUM_BPS];

void BreakpointSetCommand();

void BreakpointAccessCommand();

void BreakpointClearCommand();

void BreakpointDisableCommand();

void BreakpointEnableCommand();

void BreakpointListCommand();
