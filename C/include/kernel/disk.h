#include "sys.h"
 

uint16_t* readSectors(uint32_t sectorStart, uint8_t count); //Carica i settori specificati in RAM
void writeSector(uint32_t sectorStart, uint16_t* data, size_t size); //Scrive il settore specificato
 

 