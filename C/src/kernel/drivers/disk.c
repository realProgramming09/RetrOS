#include "kernel/mmu.h"
#include "kernel/disk.h"
#include "kernel/sysio.h"
#include "kernel/bsod.h"

//Macro che definiscono i registri dell'IDE
#define LBA_LOW 0x1F3
#define LBA_MID 0x1F4
#define LBA_HIGH 0xF5
#define STATUS 0x1F7
#define DATA 0x1F0
#define ERRORS 0x1F1
#define SECTOR_COUNT 0x1F2
#define DRIVE_REG 0x1F6
 
//Macro che definiscono i codici per disco primario o secondario
#define MASTER 0xE0

//Macro che definiscono i codice per leggere o scrivere
#define READ 0x20
#define WRITE 0x30

void diskSetup(uint32_t lba, uint32_t count, uint32_t driveNumber, uint32_t command); //Funzione che imposta il disco in base a ciò che vogliamo fare

static inline void waitForDisk(){ //Funzione che aspetta che il disco sia pronto
    uint8_t status;
    do{
        status = recByte(STATUS); //Leggiamo il registro status
    } while(status & 0x80 && !(status & 0x08)); //Finché i bit 7 e 3 sono settati, tocca aspettare
}
void diskSetup(uint32_t lba, uint32_t count, uint32_t driveNumber, uint32_t command){ 
    waitForDisk(); //Aspettiamo il disco...
    uint8_t selector = driveNumber | ((lba >> 24) & 0xF); //Prima, selezionare il disco
    sendByte(DRIVE_REG, selector);  

    //Riempire i registri con l'indirizzo LBA
    sendByte(LBA_LOW, lba & 0xFF);
    sendByte(LBA_MID, (lba >> 8) & 0xFF);
    sendByte(LBA_HIGH, (lba >> 16) & 0xFF);

    //Dire al disco quanti settori vogliamo leggere/scrivere
    sendByte(SECTOR_COUNT, count);
    sendByte(STATUS, command); //Leggere/scrivere
}
uint16_t* readSectors(uint32_t sectorStart, uint8_t count){
    
    diskSetup(sectorStart, count, MASTER, READ); //Impostare il disco principale per leggere 

    uint8_t errors = 0;
    uint16_t* sectorBuffer = genericAlloc(sizeof(uint16_t)*256 * count);
    for(uint8_t i = 0; i < count; i++){
        waitForDisk(); //Aspettiamo...
        for(uint16_t j = 0; j < 256; j++){
            sectorBuffer[256*i + j] = recWord(DATA);
            errors = recByte(ERRORS);
            if(errors){
                panic(DISK_ERROR);
            } 
            waitForDisk();
        }
    }
    
    return sectorBuffer;
}
void writeSector(uint32_t sectorStart, uint16_t* data, size_t size){
    
    //Quanti settori occupa, per eccesso
    uint16_t count;
    if(size % 512 == 0) count = size / 512;  
    else count = size / 512 +1;
    
    diskSetup(sectorStart, count, MASTER, WRITE); //Impostare il disco per scrivere
     
    for(uint16_t i = 0; i < count; i++){
        waitForDisk(); //Aspettiamo il disco...
        for(uint16_t j = 0; j < 256; j++){
            uint16_t index = i*256+j; //Assicuriamoci di non mandare il buffer in overflow
            if(index >= size) sendWord(DATA, 0);
            else sendWord(DATA, data[index]);
            if(recByte(ERRORS)){
                 panic(DISK_ERROR);
            }
            waitForDisk();
        }
    }
}
