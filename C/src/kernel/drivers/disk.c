#include "kernel/memory.h"
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

void diskSetup(uint32_t lba, uint32_t count, uint32_t driveNumber, uint32_t command); //Function that sets up the disk depending on what we want to do

static inline void waitForDisk(){ //Function that waits for disk
    uint8_t status;
    do{
        status = recByte(STATUS); //Read the status registers
    } while(status & 0x80 && !(status & 0x08)); //While bits 7 is set and bit 3 isn't, we wait
}
static inline void checkForErrors(){
    if(recByte(ERRORS) && (recByte(STATUS) & 1)){  //If there is an error, launch a BSOD
        panic(DISK_ERROR);
    } 
     
}
void diskSetup(uint32_t lba, uint32_t count, uint32_t driveNumber, uint32_t command){ 
    waitForDisk();  
    uint8_t selector = driveNumber | ((lba >> 24) & 0xF); //Tell which disk to command
    sendByte(DRIVE_REG, selector);  

    //Write LBA into registers
    sendByte(LBA_LOW, lba & 0xFF);
    sendByte(LBA_MID, (lba >> 8) & 0xFF);
    sendByte(LBA_HIGH, (lba >> 16) & 0xFF);

    //Tell the disk how many sectors we wanna read/write
    sendByte(SECTOR_COUNT, count);
    sendByte(STATUS, command); //Read/write
}
uint16_t* readSectors(uint32_t sectorStart, uint16_t count){
    
    diskSetup(sectorStart, count, MASTER, READ); //Set the master drive for reading
     
    uint16_t* sectorBuffer = genericAlloc(sizeof(uint16_t)*256 * count); //Allocate enough space on RAM
    for(uint8_t i = 0; i < count; i++){
        waitForDisk(); //Aspettiamo...
        for(uint16_t j = 0; j < 256; j++){
            sectorBuffer[256*i + j] = recWord(DATA);
            checkForErrors();
            waitForDisk();
        }
    }
    
    return sectorBuffer;
}
void readSectorsIntoBuffer(uint32_t sectorStart, uint16_t count, uint16_t* buffer, size_t size){
    
    diskSetup(sectorStart, count, MASTER, READ); //Set the master drive for reading

     
    for(uint8_t i = 0; i < count; i++){
        waitForDisk(); //Aspettiamo...
        for(uint16_t j = 0; j < 256; j++){
            int index = i*256 + j;
            if(index >= size){
                return;//We stop if we are overflowing
            } 

            buffer[256*i + j] = recWord(DATA);
            checkForErrors();
            waitForDisk();
        }
    }
 
}
void writeSector(uint32_t sectorStart, uint16_t* data, size_t size){
    
    //How many sectors the buffer occupies, approximating by excess
    uint16_t count;
    if(size % 512 == 0) count = size / 512;  
    else count = size / 512 +1;
    
    diskSetup(sectorStart, count, MASTER, WRITE); //Set the master slave for writing
     
    for(uint16_t i = 0; i < count; i++){
        waitForDisk(); //Aspettiamo il disco...
        for(uint16_t j = 0; j < 256; j++){
            uint16_t index = i*256+j; //No overflow
            if(index >= size) return;

            sendWord(DATA, data[index]);

            checkForErrors();
            waitForDisk();
        }
    }
}
