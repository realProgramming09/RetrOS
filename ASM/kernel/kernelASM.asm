bits 32
global loadIDT
global sendByte
global recByte
global sendWord
global recWord
extern isrHandler
extern idtInit
extern main
extern stepTimer
extern reserveKeyboard

;Macro per l'ISR senza errori
%macro ISR_NOERR 1 
global isr%1

isr%1:
    push dword %1
    push dword 0 ;Nessun codice di errore, padding per avere un codice solo e non 300
   
    jmp isr_main
%endmacro

;Macro per l'ISR con errori
%macro ISR_ERROR 1
global isr%1

isr%1:
    push dword %1
    jmp isr_main
%endmacro
 
 
call idtInit
call main
jmp $

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERROR 8
ISR_NOERR 9
ISR_ERROR 10
ISR_ERROR 11
ISR_ERROR 12
ISR_ERROR 13
ISR_ERROR 14
ISR_NOERR 16
ISR_ERROR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERROR 30
ISR_NOERR 31
global isr32

isr32:
    pushad ; Save registers before calling the function
    call stepTimer
    popad

    push dword 32
    push dword 0 ;Nessun codice di errore, padding per avere un codice solo e non 300
    
    jmp isr_main
ISR_NOERR 33
ISR_NOERR 34
ISR_NOERR 35
ISR_NOERR 36
ISR_NOERR 37
ISR_NOERR 38
ISR_NOERR 39
ISR_NOERR 40
ISR_NOERR 41
ISR_NOERR 42
ISR_NOERR 43
ISR_NOERR 44
ISR_NOERR 45
ISR_NOERR 46
ISR_NOERR 47
ISR_NOERR 48
ISR_NOERR 49
 
isr_main: 
    cli
    pushad
    mov ax, 0x10
    mov es, ax
    mov ds, ax
    push esp
    call isrHandler
    add esp, 4
    popad
    
    add esp, 8
    sti
    iret

loadIDT:
    mov eax, [esp + 4]
    lidt[eax]
    ret

 
sendByte:
    mov dx, [esp+4]
    mov al, [esp+8]
    out dx, al
    ret

 
recByte:
    mov dx, [esp+4]   ; porta
    in  al, dx
    movzx eax, al     ; ritorna in EAX
    ret
 
sendWord:
    mov dx, [esp+4]
    mov ax, [esp+8]
    out dx, ax
    ret


recWord:
    mov dx, [esp+4]   ; porta
    in  ax, dx
    movzx eax, ax     ; ritorna in EAX
    ret
 
