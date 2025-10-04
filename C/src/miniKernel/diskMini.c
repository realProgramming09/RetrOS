#include "kernel/memory.h"
#include "kernel/disk.h"
#include "kernel/sysio.h"
#include "kernel/bsod.h"

#define LBA_LOW 0x1F3
#define LBA_MID 0x1F4
#define LBA_HIGH 0xF5
#define STATUS 0x1F7
#define DATA 0x1F0
#define ERRORS 0x1F1
#define SECTOR_COUNT 0x1F2
#define DRIVE_REG 0x1F6
 
/* THIS FILE IS IDENTICAL TO disk.c BUT IMPLEMENTS ONLY THE NECESSARY FUNCTIONS TO READ THE KERNEL FROM DISK IN 32bit*/
/* SO I WON'T TRANSLATE THE COMMENTS INTO ENGLISH */

void waitForDisk(){
    uint8_t status;
    do{
        status = recByte(STATUS); //Read the status register
    } while(status & 0x80 && !(status & 0x08)); //While bits 7 and 3 ar
}

uint16_t* readSectors(uint32_t sectorStart, uint8_t count){
    waitForDisk(); //Aspettiamo il disco...
    uint8_t selector = 0xE0 | ((sectorStart >> 24) & 0xF); //Prima, selezionare il disco
    sendByte(DRIVE_REG, selector);  

    //Riempire i registri con l'indirizzo LBA
    sendByte(LBA_LOW, sectorStart & 0xFF);
    sendByte(LBA_MID, (sectorStart >> 8) & 0xFF);
    sendByte(LBA_HIGH, (sectorStart >> 16) & 0xFF);

    //Dire al disco quanti settori vogliamo leggere
    sendByte(SECTOR_COUNT, count);
    sendByte(STATUS, 0x20); //Leggere

   
    uint16_t* sectorBuffer = genericAlloc(sizeof(uint16_t)*256 * count);
    for(uint8_t i = 0; i < count; i++){
        waitForDisk(); //Aspettiamo...
        for(uint16_t j = 0; j < 256; j ++){
            sectorBuffer[256*i + j] = recWord(DATA);
        }
    }
    
    return sectorBuffer;
}
 
