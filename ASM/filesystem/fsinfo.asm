FSINFO:
    .leadSign: dd 0x41615252    ; Firma per validare questo settore come settore FSINFO
    .reserved: times 480 db 0   ; Quasi tutto il settore è riservato
    .sign2: dd 0x41615252       ; Firma centrale
    .freeClusters: dd 0xFFFFFFFF ; Quanti cluster sono liberi. 0xFFFFFFFF significa che dobbiamo rifare i conti
    .startLooking: dd 2        ; Da quale cluster cercare per cluster liberi. 
    .reserved2: dd 0            ; 12 byte riservati
                dw 0
    .trailSign: dd 0xAA550000   ; Firma finale