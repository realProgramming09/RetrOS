#include "sys.h"
#include "terminal.h"
#pragma once

typedef struct MMU{
    uint32_t capacity; //Total RAM capacity
    uint32_t freeAmount; //Total free RAM
    uint32_t* segmentBitmap; //BItmap that divides RAM into segments
} MMU_t;
 

MMU_t* mmuInit(); //Inizializza la MMU
uint32_t detectRam(void); //Ritorna la capacità della RAM
void loadMMU(MMU_t* m); //Carica una MMU. utile per comunicare

void* genericAlloc(size_t size); //Allocatore generico
void* genericRealloc(void* ptr, size_t size); //Ri-allocatore generico
void genericFree(void* ptr); //Libera un puntatore generico

uint32_t getTotalRam(); //Ritorna la capacità totale di RAM 
uint32_t getFreeRam(); //Ritorna la quantità di RAM libera