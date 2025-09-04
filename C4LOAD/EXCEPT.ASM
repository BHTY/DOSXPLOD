	.386p
	.MODEL flat

PUBLIC SetHandlers_
EXTERN _ExceptionPrint:PROC

.CODE

datasel dw 0
stacksel dw 0
flatstack dd 0
stackbase dd 0
handler dd offset _ExceptionPrint

int00:
    push 0
    jmp except_handler
int01:
    push 1
    jmp except_handler
int03:
    push 0
    push cs
    call invoke_except_handler
    iretd
int0d:
    push 0dh
    jmp except_handler
int0e:
    push 0eh
    jmp except_handler

thing:
    ret

invoke_except_handler:
    push 3
    jmp except_handler

except_handler:
    pushad              ; Push general purpose registers
    push ds             ; Push segment registers
    push es
    push gs
    push fs
    mov esi, esp        ; SS:ESI = pointer to exception frame
    mov stacksel, ss    ; Save exception handler SS
    mov ax, ds          ; Set flat SS
    mov ss, ax
    mov esp, flatstack  ; And flat stack
    mov bx, stacksel    ; Calculate base of exception handler SS
    mov ax, 6           ; DPMI call: Get Segment Base Address
    int 31h
    mov eax, ecx        ; EAX = base address
    shl eax, 16
    mov ax, dx
    mov stackbase, eax
    add esi, eax        ; ESI = linear address of exception handler stack frame
    push esi
    call [handler]
    pop esp             ; ESP = linear address of exception handler stack frame
    sub esp, stackbase  ; Readjust ESP to be relative to exception handler SS
    mov bx, stacksel    ; Restore exception handler SS
    mov ss, bx          
    pop fs              ; Pop segment registers
    pop gs
    pop es
    pop ds
    popad               ; Pop general-purpose registers
    add esp, 4          ; Remove exception code
    retfd

SetHandlers_:
    mov datasel, ds
    mov ax, 203h
    mov bl, 0
    mov cx, cs
    mov edx, offset int00
    int 31h

    mov datasel, ds
    mov ax, 203h
    mov bl, 0dh
    mov cx, cs
    mov edx, offset int0d
    int 31h

    mov datasel, ds
    mov ax, 205h
    mov bl, 03h
    mov cx, cs
    mov edx, offset int03
    int 31h

    mov datasel, ds
    mov ax, 203h
    mov bl, 01h
    mov cx, cs
    mov edx, offset int01
    int 31h

    mov datasel, ds
    mov ax, 203h
    mov bl, 0eh
    mov cx, cs
    mov edx, offset int0e
    int 31h

    mov ax, 501h
    mov bx, 0
    mov cx, 1000h
    int 31h
    mov word ptr [flatstack], cx
    mov word ptr [flatstack+2], bx
    add [flatstack], 1000h

    ret

END

; pushad ... push fs
; mov esi, esp
; mov stacksel, ss
; mov ax, ds
; mov ss, ax
; mov esp, flatstack
; mov bx, stacksel
; mov ax, 6
; int 31h
; mov eax, ecx
; shl eax, 16
; mov ax, dx
; add esi, eax
; push esi
; call _ExceptionPrint
; pop esp
; mov bx, stacksel
; mov ss, bx
; pop fs ... popad