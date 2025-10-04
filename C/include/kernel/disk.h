#include "sys.h"
 

void readSectorsIntoBuffer(uint32_t sectorStart, uint16_t count, uint16_t* buffer, size_t size); //Loads the specified sectors in buffer
uint16_t* readSectors(uint32_t sectorStart, uint16_t count); //Loads the specified folder in RAM
void writeSector(uint32_t sectorStart, uint16_t* data, size_t size); //Writes data starting from a sector
 

 