This directory contains the source code for DOSXPLOD.DLL, the main client
library for C4. It provides a C-callable interface for
- DOS Protected-Mode Interface (INT 31h)
- MS-DOS (INT 21h)
- BIOS (various)
- Microsoft Mouse (INT 33h)
- Image loader

Should the DOS extender loader hook a few interrupts as syscalls, including the image loader?