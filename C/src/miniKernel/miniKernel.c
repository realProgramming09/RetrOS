
#include "kernel/sys.h"
#include "kernel/mmu.h"
 
 
void start(){
     MMU_t* state = mmuInit();
     
     uint16_t* mainKernel = readSectors(15, 81); //Legge il kernel dal disco e lo mette in RAM
     *(MMU_t*)0x5000 = *state; //Salva la MMU
     ((void (*)())mainKernel)(); //Saltare al kernel
}