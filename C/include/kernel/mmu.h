#include "sys.h"
#include "terminal.h"
#pragma once

typedef struct MMU{
    uint32_t capacity; //Capacità totale
    uint32_t freeAmount; //Quanta RAM è libera
    uint32_t* segmentBitmap; //ARray di bitmap dei segmenti
} MMU_t;
typedef struct Page{
    uint16_t pageNumber; //Numero della pagina
    void* base; //Indirizzo della pagina
}__attribute__((packed)) Page_t;

MMU_t* mmuInit(); //Inizializza la MMU
uint32_t detectRam(void); //Ritorna la capacità della RAM
void loadMMU(MMU_t* m); //Carica una MMU. utile per comunicare. 

Page_t requestPage(); //Richiede una pagina (8KB), dove hai il pieno controllo
void freePage(Page_t* page); //Libera la pagina

void* genericAlloc(size_t size); //Allocatore generico
void* genericRealloc(void* ptr, size_t size); //Ri-allocatore generico
void genericFree(void* ptr); //Libera un puntatore generico

uint32_t getTotalRam(); //Ritorna la capacità totale di RAM 
uint32_t getFreeRam(); //Ritorna la quantità di RAM libera