#include "kernel/sys.h"
#pragma once

#define MALLOC_FAILED 1
#define MMU_CORRUPTION 2
#define SUICIDE 3
#define FILESYSTEM_CORRUPTION 4
#define DISK_ERROR 5
#define MMU_OVERWRITE 6
#define SERIAL_ERROR 7

#define panic(x) panicCode = x; \
int z =3 / 0;\

typedef struct{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t isrNumber;
} RegisterFrame;


extern int panicCode;
void launchBSOD(RegisterFrame frame, int code); //Lancia il BSOD, chiedendo dei registri (dall'ISR)