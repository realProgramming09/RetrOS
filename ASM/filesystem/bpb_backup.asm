BPB:
    db 0xEB, 0x54, 0x90         ; Codice necessario
    .oemID: dd "MSWIN4.1"       ; Identificatore OEM
    .bytesPerSector: dw 512     ; Dimensione del settore, in byte
    .sectorsPerCluster: db 128   ; Settori per Cluster
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