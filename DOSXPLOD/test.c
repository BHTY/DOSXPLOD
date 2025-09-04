/**
 * 
 */

#include "../DOSCALLS.H"
#include "../DPMI.H"
#include "../BIOCALLS.H"
#include "../I386INS.H"

void VgaSetPalette(BYTE cIdx, BYTE cRed, BYTE cGreen, BYTE cBlue) {
	outb(0x3c8, cIdx);
	outb(0x3c9, cRed >> 2);
	outb(0x3c9, cGreen >> 2);
	outb(0x3c9, cBlue >> 2);
}

void VgaWaitBlank() {
    
}

void kill() {
	*(char*)(0xDEADBEEF) = 0;
}

int mainCRTStartup() {
	int i;
	ULONG ulWritten;
	DWORD dwBase;
	WORD wSel;
	char fillcolor = 0;

	*(unsigned char*)(0xB8000) = 0x41;

	VioSetMode(0x13);

	for (i = 0; i < 64000; i++) {
		*(unsigned char*)(0xA0000 + i) = i;
	}

	KbdRead();
	stosb(0xA0000, 15, 64000);

	KbdRead();

	while (!KbdHit()) {
		
	stosb(0xA0000, fillcolor++, 64000);
	}

	//KbdRead();
	VioSetMode(0x03);
	DosWrite(1, "Hello, world!\n\r", 15, &ulWritten);


	DpmiMapLinear(0xA0000, 0x10000, &dwBase);

	DpmiMapSegmentSelector(0xB800, &wSel);

	__asm {
		mov ax, wSel
		mov fs, ax
		mov ecx, dwBase
	}

	kill();

	return 0;
}