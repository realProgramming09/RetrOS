bits 16
 

switch:
    cli
    lgdt[GDT]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm

 

start: 
        dq 0
    
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b10011010
        db 0xCF
        db 0x00
   
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b10010010
        db 0xCF
        db 0x00
 
end: 

GDT:
    dw end - start - 1
    dd start

bits 32
pm:

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x90000
    mov esp, ebp
    

     
    jmp 0x2000 ;Salta al kernel
    jmp $

