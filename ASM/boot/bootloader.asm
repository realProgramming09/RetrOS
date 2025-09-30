bits 16

BPB:
    db 0xEB, 0x54, 0x90         ; Codice necessario
    .oemID: dd "MSWIN4.1"       ; Identificatore OEM
    .bytesPerSector: dw 512     ; Dimensione del settore, in byte
    .sectorsPerCluster: db 64   ; Settori per Cluster
    .reservedSector: dw 96      ; Settori riservati (questo incluso)
    .FATs: db 1                 ; Numero di FAT. 2 di solito (1 attiva, una di backup)
    .rootEntries: dw 0          ; Numero di entries per la root directory. Non usato in FAT32
    .totalSectors: dw 0         ; NUmero di settori totali. Non usato in FAT32 
    .mediaType: db 0xf8         ; Tipo di media. 0xF8 = HDD, 0xF0 = Floppy standard
    .sectorsPerFat: dw 0        ; Numero di settori per FAT. Non utilizzato in FAT32
    .sectorsPerTrack: dw 0      ; Settori per traccia. Da chiedere al BIOS
    .headsNumber: dw 0          ; Numero di teste. Da chiedere al BIOS
    .hiddenSectorsNumber: dd 0  ; Numero di settori nascosti. Non ci sono necessari
    .largeTotalSectors: dd 204800 ; Numero di settori totali. Usare per FAT32 o se hai più di 65535 settori
EBPB:
    .sectorsPerFat: dd 64        ; Settori per FAT.
    .flags: dw 0                ; Flags. Queste specificano che tutte le FAT sono aggiornate a runtime
    .FATrevision: dw 0          ; Numero revisione. Si, la più recente è la 0.0 
    .rootCluster: dd 2          ; Primo cluster della root directory.  
    .FSinfo:  dw 7              ; Settore della struttura FSINFO
    .backupBPB: dw 6            ; Settore dove si trova il backup del BPB se si rompe il disco
    .reserved:  dd 0            ; 12byte riservati
                dw 0
    .driveNumber: dw 0x80       ; Numero disco, 0x80 per HDD e 0x00 per Floppy
    .reserved2: db 1            ; Riservato
    .signature: db 0x29         ; Firma
    .serialNumber: dd 0x12345678   ; Numero seriale arbitrario
    .label: db "ILMIODISCO  "   ; Nome del disco. Arbitrario
    .fileSys: db "FAT32   "     ; Descrive il formato. Inaffidabile, ma necessario


boot:

    mov [EBPB.driveNumber], dl 

    ; Impostare i registri dei segmenti
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov bp, 0x7B00
    mov sp, 0x7B00

    ;Leggere il numero di teste e i settori per track dal bios
    mov ah, 8
    int 0x13
    movzx eax, dh
    inc eax
    mov dword [BPB.headsNumber], eax
    movzx eax, cl
    and eax, 0x3F
    mov dword [BPB.sectorsPerTrack], eax

    ; Leggere il secondo file del bootloader dal disco
    mov ah, 2        ; Funzione di lettura
    mov al, 2       ; Settori da leggere
    mov cx, 3       ; Testina 0 e settore di partenza 2
    mov dh, 0        ; Testina 0
    mov bx, 0x1000   ; Carica a ES:BX = 0x0000:0x1000
    mov dl, [EBPB.driveNumber]
    int 0x13
    jc error

    ;Leggere il kernel dal disco
    mov ah, 2
    mov al, 40 ;Leggiamo 20KB 
    mov cx, 10
    mov dh, 0
    mov bx, 0x2000
    int 0x13
    jc error

    

    call readRam

    jmp 0x0000:0x1000  ; Salta al secondo file del bootloader

readRam:
    mov di, 0x4000 ; Indirizzo dove si troverà la mappa
    xor ebx, ebx
    mov edx, 0x534D4150 ; Numero magico
    mov eax, 0xe820 ;Codice interrupt
    mov ecx, 24 ;Chiediamo per 24byte
    mov bp, 0   ;Qui salveremo quante entry
    int 0x15
    jc error
.loop:
    add di, 24 ;Prepariamoci per la prossima entry
    cmp eax, 0x534D4150 
    jnz error
    jc .exit
    cmp ebx, 0
    jz .exit
    mov eax, 0xe820
    mov ecx, 24
    inc bp
    int 0x15
    jmp .loop
.exit:
    mov word [0x1FF0], bp
    ret
error:
    mov si, diskErrorMsg
    call print
    jmp $

print:
    push ax
    mov ah, 0x0e
.loop:
    lodsb
    or al, al
    jz .exit
    int 0x10
    jmp .loop
.exit:
    pop ax
    ret

diskErrorMsg: db "Disk read failed...", 0
times 510 - ($ - $$) db 0
dw 0xAA55



