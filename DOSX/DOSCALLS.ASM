INT_FRAME STRUCT
    ifEIP       DWORD   ?
    ifCS        DWORD   ?
    ifEFLAGS    DWORD   ?
INT_FRAME ENDS

INT_PUSHAD_FRAME STRUCT

INT_PUSHAD_FRAME ENDS

;   
DPMIREGS STRUCT
    rmEDI       DWORD   ?
    rmESI       DWORD   ?
    rmEBP       DWORD   ?
    rmReserved  DWORD   ?
    rmEBX       DWORD   ?
    rmEDX       DWORD   ?
    rmECX       DWORD   ?
    rmEAX       DWORD   ?
    rmFLAGS     WORD    ?
    rmES        WORD    ?
    rmDS        WORD    ?
    rmFS        WORD    ?
    rmGS        WORD    ?
    rmCS        WORD    ?
    rmIP        WORD    ?
    rmSP        WORD    ?
    rmSS        WORD    ?
DPMIREGS ENDS


RMREGS DPMIREGS <?>
SelRMTA dw ?
SegRMTA dw ?            
PrevInt21 dd ?           


;   DOSCall_Write - Handles a protected-mode int 21h/40h, splitting writes 
;   above 64K into 64K chunks
;
;   Parameters (client registers):
;   - BX = File handle
;   - ECX = Number of bytes to be written
;   - DS:EDX = Segment:offset of buffer area
;   
;   Return values (client registers):
;   - If succcessful, CF is clear
;       EAX = Number of bytes written
;   - If failed, CF is set
;       EAX = Error code

DOSCall_Write proc far public
    xor edx, edx                            ; Number of bytes written so far
    mov esi, [esp.ipafECX]                  ; Number of bytes left to write

    mov ax, word ptr cs:[SelRMTA]           ; ES = Protected-mode selector to
    mov es, ax                              ; real-mode transfer buffer

    mov ebx, [esp.ipafEBX]                  ; Set real-mode EBX equal to client
    mov cs:[RMREGS.rmEBX], ebx              ; EBX (file handle)

    mov ax, word ptr cs:[SegRMTA]           ; Set real-mode DS equal to real-
    mov word ptr cs:[RMREGS.rmDS], ax       ; mode segment of transfer buffer

    xor ax, ax                              ; Set real-mode DX to 0
    mov word ptr cs:[RMREGS.rmEDX], ax

    mov cs:[RMREGS.rmSP], ax                ; Clear real-mode SS:SP to use
    mov cs:[RMREGS.rmSS], ax                ; real-mode stack provided by
                                            ; DPMI host

                                            ; Split into <=64K writes
@@loop:                                     ; ECX = Size of this sub-write
    cmp esi, 0FFFFh                         ; Is the transfer above 64K?
    ja @@clamp_size                         ;   If so, clamp sub-write to 64K
    mov ecx, esi                            ; Otherwise, this covers the whole
    jmp @@transfer                          ; write

@@clamp_size:
    mov ecx, 0FFFFh                         ; Clamp to 64K

@@transfer:
    mov cs:[RMREGS.rmECX], ecx              ; Set real-mode CX equal to sub-
                                            ; write size
    xor edi, edi
    mov esi, edx                            ; DS:ESI = Pointer to current chunk
    add esi, [esp.ipafEDX]                  
    rep movsb                               ; Copy chunk to transfer area

    ; Reflect the interrupt into real-mode to write this chunk
    push es                                 ; Save ES
    mov ax, 300h                            ; DPMI call: Simulate Real Mode Int
    mov bl, 21h                             ; Simulate int 21h
    xor bh, bh
    xor cx, cx                              ; Don't copy onto the realmode stack
    mov di, cs                              ; ES:EDI = Pointer to realmode regs
    mov es, di
    mov edi, offset RMREGS
    int 31h
    pop es                                  ; Restore ES

    test cs:[RMREGS.rmFLAGS], 1             ; Did the call fail?
    jnz @@error                             ;   If so, jump to the handler

    movzx eax, word ptr cs:[RMREGS.rmEAX]
    add edx, eax                            ; Add the number of bytes written
                                            ; to the total
    sub esi, eax                            ; Subtract the number of bytes
                                            ; written from the total left
    cmp eax, word ptr cs:[RMREGS.rmECX]     ; Did we write fewer than requested?
    jb @@done                               ;   If not, do another iteration

@@done:                                     ; Commit results back
    and [esp.ipafEFLAGS], 0FFFFFFFEh        ; Clear carry flag (no error)
    mov [esp.ipafEAX], edx                  ; Set client EAX to # of bytes written
    ; return

@@error:                                   
    test edx, edx                           ; Is this the first write? (0 written)
    jnz @@done                              ;   If not, return the number of bytes
                                            ;   we've already written
    or [esp.ipafEFLAGS], 1                  ; Indicate that an error occured
    mov eax, cs:[RMREGS.rmEAX]              ; Set client EAX to the error code
    mov [esp.ipafEAX], eax                  
    ; return
DOSCall_Write endp



;   RealMode_ReflectInt - The generic interrupt dispatcher that reflects an
;   interrupt back into real-mode, if no parameters need to be translated.
;   All protected-mode registers are copied into the real-mode register
;   image, the interrupt is simulated using DPMI int 31h/300h on the host's
;   real-mode stack, and the registers are copied back into the protected-
;   mode register image.
;   Note that DPMI's default behavior *is* to reflect interrupts into
;   real-mode, but since interrupts such as INT 21H are already hooked in
;   protected-mode for system calls that require parameter translation,
;   this stub is required for system calls that do not require translation.
;
;   Parameters:
;   - SS:ESP: Pointer to client protected-mode register image
;   - BL: Interrupt number

RealMode_ReflectInt proc far public
    ; Copy protected-mode registers -> real-mode
    xor eax, eax                        ; Clear real-mode SP:SS to use real-
    mov cs:[RMREGS.rmSP], ax            ; mode stack provided by DPMI host
    mov cs:[RMREGS.rmSS], ax
    
    ; Reflect the interrupt into real-mode
    push es                             ; Save ES
    mov ax, 300h                        ; DPMI call: Simulate Real Mode Int
    xor bh, bh
    xor cx, cx                          ; Don't copy onto the realmode stack
    mov di, cs                          ; ES:EDI = Pointer to realmode regs
    mov es, di
    mov edi, offset RMREGS
    int 31h
    pop es                              ; Restore ES

    ; Copy real-mode registers -> protected-mode


    ; Return
RealMode_ReflectInt endp


;   DOSCall_ReflectInt - The generic MS-DOS API dispatcher that reflects an
;   interrupt back into real-mode, if no parameters need to be translated.
;   Just a stub that calls RealMode_ReflectInt with BL set to 21H.

DOSCall_ReflectInt proc far public
    mov bl, 21h                     ; Reflect INT 21H
    jmp RealMode_ReflectInt
DOSCall_ReflectInt endp


;   
DOSCall_DispTbl
    dd offset DOSCall_ReflectInt        ; AH=00h
    dd offset DOSCall_ReflectInt        ; AH=01h
    dd offset DOSCall_ReflectInt        ; AH=02h
    dd offset DOSCall_ReflectInt        ; AH=03h
    dd offset DOSCall_ReflectInt        ; AH=04h
    dd offset DOSCall_ReflectInt        ; AH=05h
    dd offset DOSCall_ReflectInt        ; AH=06h
    dd offset DOSCall_ReflectInt        ; AH=07h
    dd offset DOSCall_ReflectInt        ; AH=08h
    dd offset DOSCall_PrintString       ; AH=09h
    dd offset DOSCall_BufferedInput     ; AH=0Ah
    dd offset DOSCall_ReflectInt        ; AH=0Bh
    dd offset DOSCall_ReflectInt        ; AH=0Ch
    dd offset DOSCall_ReflectInt        ; AH=0Dh
    dd offset DOSCall_ReflectInt        ; AH=0Eh
    dd offset DOSCall_OpenFCB           ; AH=0Fh
    dd offset DOSCall_CloseFCB          ; AH=10h
    dd offset DOSCall_FindFirstFCB      ; AH=11h
    dd offset DOSCall_FindNextFCB       ; AH=12h
    dd offset DOSCall_DeleteFCB         ; AH=13h
    dd offset DOSCall_ReadFCB           ; AH=14h
    dd offset DOSCall_WriteFCB          ; AH=15h
    dd offset DOSCall_CreateFCB         ; AH=16h
    dd offset DOSCall_RenameFCB         ; AH=17h
    dd offset DOSCall_ReflectInt        ; AH=18h
    



;   DOSCall - This is the main entry point for INT 21H interrupts issued from
;   protected-mode. If the desired call is valid, it will push all of the
;   registers onto the stack and invoke the appropriate handler to translate
;   parameters into real-mode, before popping them and returning back to the
;   client application. Otherwise, it sets the carry flag for an invalid call.
;
;   The desired MS-DOS API call is stored in the AH register. All other
;   parameters are dependent on the API call. The carry flag is cleared if
;   the call is successful and set if it fails. All other return values are
;   dependent on the API call.

DOSCall proc far public
    cmp ah, 6ch
    ja @@inv_fn
    pushad                          ; Push all registers
    push ds                         ; Push segment registers
    push ss
    push es
    push fs
    push gs
    movzx eax, ah
    jmp dword ptr [DOSCall_tbl + eax*4]
@@inv_fn:
    or dword ptr [esp.ifEFLAGS], 1  ; Set carry flag
    iretd
DOSCall endp
