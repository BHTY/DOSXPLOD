/**
 *      File: MAIN.C
 *      Main driver for DDT32 debugger.
 *      Copyright (c) 2025 by Will Klees
 */

#include "../DOSCALLS.H"
#include "../DOSXPLOD.H"
#include "../I386INS.H"

DWORD mystrlen(char* str) {
    DWORD cnt = 0;

    while (*(str++)) cnt++;

    return cnt;
}

void puts(char* str) {
    ULONG ulWritten;
    DosWrite(1, str, mystrlen(str), &ulWritten);
}

int gets(char* str, int max) {
    ULONG ulRead;
    DosRead(0, str, max, &ulRead);
    str[ulRead] = 0;
    return ulRead;
}


typedef LONG int32_t;
typedef DWORD uint32_t;
typedef BYTE uint8_t;
typedef WORD uint16_t;

#define isdigit(d)      ((d) >= '0' && (d) <= '9')

uint32_t ilog(uint32_t num, uint32_t base){
    uint32_t t = 0;

    while(num >= 1){
        num /= base;
        t++;
    }

    return t;
}

int32_t ipow(uint32_t base, uint32_t power){
    int32_t t = 1;

    while(power > 0){
        power--;
        t *= base;
    }

    return t;
}

char hexch(uint8_t num){ //for a number between 0 and 15, get the hex digit 0-f
    if(num < 10){
        return (char)(num + 48);
    }
    return (char)(num + 55);
}

int32_t numtostr(uint8_t *str, int num, int base, int sign, int digits){ //0=unsigned, 1=signed
	int i;
    int places;
	
	if (sign && (num < 0)){
		str[0] = '-';
		i = 1;
		num = ~num + 1;
	}
	else{
		i = 0;
	}

	places = ilog(num, base);
    places = (places == 0) ? 1 : places;

    for(; i < (digits - places); i++){
            str[i] = '0';
    }

	while (places > 1){
		uint32_t temp = ipow(base, places - 1);
		uint32_t tmp = (num - (num % temp));
		str[i] = hexch(tmp / temp);
		num -= tmp;

		places--;
		i++;
	}

	str[i] = hexch(num);
	str[i + 1] = 0;

	return i + 1;
}

int skip_atoi(const char **s)
{
    int i=0;

    while (isdigit(**s))
        i = i*10 + *((*s)++) - '0';
    return i;
}


typedef void** VA_LIST;
#define VA_START(val, lastarg)      val = &(lastarg) + 1
#define VA_ARG(val, type)           ((type)(*(val++)))
//#define VA_ARG(val, type)           ((type)(readssdword(val++)))
#define VA_END(val)

#undef va_start
#undef va_list
#undef va_arg
#undef va_end

#define va_list VA_LIST
#define va_start VA_START
#define va_arg VA_ARG
#define va_end VA_END


int avsprintf(char* buf, const char* fmt, va_list args){
    char *str;
	uint16_t i;
	uint16_t len;
	char *s;
    int digits = 0;

	for (str = buf; *fmt; ++fmt){

		if (*fmt != '%'){
			*str++ = *fmt;
			continue;
		}

		fmt++;
        digits = 0;

        if(isdigit(*fmt)) digits = skip_atoi(&fmt);

		switch (*fmt){
			case 'c':
				*str++ = (char)va_arg(args, int);
				break;
			case 's':
				s = va_arg(args, char*);
				len = mystrlen(s);
				for (i = 0; i < len; ++i) *str++ = *s++;
				break;
			case 'o':
				break;
			case 'x':
				str += numtostr(str, va_arg(args, unsigned int), 16, 0, digits);
				break;
			case 'X':
				str += numtostr(str, va_arg(args, int), 16, 0, digits);
				break;
			case 'p':
				str += numtostr(str, (uint32_t)(va_arg(args, void*)), 16, 0, 8);
				break;
			case 'd':
			case 'i':
				str += numtostr(str, va_arg(args, int), 10, 1, digits);
				break;
			case 'u':
				str += numtostr(str, va_arg(args, unsigned int), 10, 0, digits);
				break;
			case 'b':
				str += numtostr(str, va_arg(args, unsigned int), 2, 0, digits);
				break;
			case 'B':
				str += numtostr(str, va_arg(args, unsigned int), 2, 1, digits);
				break;
			default:
                *str++ = '%';
				break;
		}

	}

	*str = 0;

	return str - buf;
}

/**
 *  SysLogError procedure - 
 */
void      printf(char* fmt, ...) {
    CHAR Buffer[1024];
    ULONG ulWritten;
    va_list args;
    DWORD dwBase;

    va_start(args, fmt);

    avsprintf(Buffer, fmt, args);
    va_end(args);

    DosWrite(1, Buffer, mystrlen(Buffer), &ulWritten);
}

void PrintReason(PEXCEPT_CONTEXT pContext) {
    switch (pContext->ExceptionNumber) {
        case 0:
            printf("Divide Fault\n\r");
            break;
        case 1:
            printf("Unhandled Debug Exception\n\r");
            break;
        case 2:
            printf("Unhandled NMI\n\r");
            break;
        case 3:
            printf("Unhandled Breakpoint Trap\n\r");
            break;
        case 4:
            printf("Overflow Trap\n\r");
            break;
        case 5:
            printf("Bound Fault\n\r");
            break;
        case 6:
            printf("Invalid Opcode Fault\n\r");
            break;
        case 7:
            printf("Device Not Available Fault\n\r");
            break;
        case 8:
            printf("Double Fault\n\r");
            break;
        case 0xA:
            printf("Invalid TSS Fault: Sel=%04Xh\n\r", pContext->ErrorCode);
            break;
        case 0xB:
            printf("Segment Not Present Fault: Sel=%04Xh\n\r", pContext->ErrorCode);
            break;
        case 0xC:
            printf("Stack Segment Fault: Sel=%04Xh\n\r", pContext->ErrorCode);
            break;
        case 0xD:
            printf("General Protection Fault: Sel=%04Xh\n\r", pContext->ErrorCode);
            break;
        case 0xE:
            printf("Page Fault %s %08Xh %s\n\r", (pContext->ErrorCode & 2) ? "Writing" : "Reading",
                getCR2(), (pContext->ErrorCode & 1) ? "" : "(NP)");
            break;
        default:
            break;
    }
}

void DumpRegisters(PEXCEPT_CONTEXT pContext) {
    printf("AX=%08X  BX=%08X  CX=%08X  DX=%08X  SI=%08X  DI=%08X\n\r", 
        pContext->EAX, pContext->EBX, pContext->ECX, pContext->EDX, pContext->ESI, pContext->EDI);
#define FLAG(mask,set,clear)    ((pContext->EFLAGS & (1 << mask)) ? set : clear)
    printf("IP=%08X  SP=%08X  BP=%08X  IOPL=%d        %s %s %s %s %s %s %s %s\n\r",
        pContext->EIP, pContext->ESP, pContext->EBP, (pContext->EFLAGS & 0x3000) >> 12,
        FLAG(I386_FLAG_OV,"OV","NV"), FLAG(I386_FLAG_DIR ,"DN","UP"), 
        FLAG(I386_FLAG_IE,"EI","DI"), FLAG(I386_FLAG_SIGN,"NG","PL"), 
        FLAG(I386_FLAG_ZR,"ZR","NZ"), FLAG(I386_FLAG_AC  ,"AC","NA"), 
        FLAG(I386_FLAG_PF,"PE","PO"), FLAG(I386_FLAG_CY  ,"CY","NC"));
#undef FLAG
    printf("CS=%04X  SS=%04X  DS=%04X  ES=%04X  FS=%04X  GS=%04X  CR3=%05X   EFL=%06X\n\r",
        pContext->CS, pContext->SS, pContext->DS, pContext->ES, 
        pContext->FS, pContext->GS, getCR3() >> 12, pContext->EFLAGS);

    /* Print near symbol */
    /* Disassemble instruction */
}

BYTE Region[4096] = {0xCC, 0xCC, 0xCC, 0xCC};

int mainCRTStartup() {
    DWORD (cdecl *pfnRegion)(void);

    EXCEPT_CONTEXT ctx;
    char buf[512];
    puts("DDT32 Symbolic Debugger for Intel 80386\n\r\n");
    stosb(&ctx, 0, sizeof(EXCEPT_CONTEXT));
    ctx.ExceptionNumber = 0x3;
    PrintReason(&ctx);
    DumpRegisters(&ctx);
    puts("DDT32!DebugRegion:\n\r");
    puts("01A7:00453000    DIV    ECX\n\r");
    puts("# ");
    gets(buf, 512);
    puts(buf);

    __asm {
        mov eax, offset Region
        call eax
    }

//    pfnRegion = &Region;
//    pfnRegion();

    return 0;
}

/**
 *  Say that CS:EIP points at a soft breakpoint (non-hardcoded INT03)
 *      actually this applies to hard breakpoints too
 *  If you trace
 *      The breakpoint is temporarily removed
 *      A flag is set, as is the trace flag (that would have been set anyway)
 *      When the processor returns into the trace handler, the breakpoint will be replaced and the flag unset
 *      Of course, now we're back in the debugger
 *  If you go
 *      The breakpoint is temporarily removed
 *      A flag is set, as is the trace flag
 *      When the processor returns into the trace handler, the breakpoint will be replaced and the flag unset
 *      Clear the trace flag
 *      Continue execution
 * 
 *  The trace handler needs a little bit of intelligence
 *      Is the instruction an INT01? 
 *      Was the trace flag set?
 *          If so, work out why
 *      Was it a hard breakpoint?
 */

/**
 *  Preallocate a small memory region to store user-generated code/data
 *  Initially populate that region with
 *      Region+0000: INT03
 *      Region+0001: CALL ModuleEntry   (only if a module was provided)
 *      Region+0006: RET                ()
 * 
 *  Then call Region() to break into the debugger
 *  The return address on the stack points back into the loop which will
 *  print "hey!" and then loop back into the debugger again
 * 
 *  Also, add something like IsBadReadPtr() even though it's bad form
 */

/**
 *  Fault vs Trap: A fault happens before the instruction executes; CS:EIP points
 *  to the faulting instruction. Traps happen after.
 * 
 *  INT03 (breakpoint), INT04 (overflow), and some debug exceptions are traps
 *      Data read/write breakpoints are traps
 *      I/O read/write breakpoints are traps
 *      Single-step exceptions are traps
 *      (Notably, an INT01 instruction fetch breakpoint is a fault)
 *      (I don't know about a hardcoded INT01 trace)
 */

/**
 *  Fix the INT03 handler to set up a proper interrupt stack frame
 *      SS      |
 *      ESP     <-- We don't yet push this stuff
 *      EFLAGS  |
 *      CS      |
 *      EIP     <-- Points here
 */

