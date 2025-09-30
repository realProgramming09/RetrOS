bits 32
extern start
global sendByte
global recByte
global sendWord
global recWord


call start
jmp $

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
 

