#include "kernel/memory.h"
#include "kernel/sys.h"
#include "kernel/sysio.h" 
#include "kernel/terminal.h"
#include "kernel/bsod.h"


#define E820_ADDR 0x4000
#define E820_COUNT_ADDR 0x1FF0
#define SEG_SIZE 0x80 //128B
#define SEG_OFFSET_SIZE 0x10000
#define RAM_START_ADDR 0x100000

/* THIS FILE IS THE EXACT SAME AS mmu.c, BUT IT IMPLEMENTS ONLY THE NECESSARY FUNCTIONS TO LOAD THE KERNEL INTO RAM */
/* I WON'T TRANSLATE THIS COMMENTS INTO ENGLISH */

struct E820Entry{
    uint64_t baseAddr; //Indirizzo del segmento
    uint64_t length; //Lunghezza del segmento
    uint32_t type; //Tipo di segmento (utile, riservato, rotto etc)
    uint32_t additional; //Info extra non sempre date
}__attribute__((packed));


typedef struct SegmentHeader{
    uint8_t* base; //Indirizzo iniziale dell'area allocata
    uint32_t segmentCount; //Quanti segmenti l'area copre
}__attribute__((packed)) SegmentHeader_t;

MMU_t miniMMU;
static MMU_t* state;
uint32_t segmentBitmap[SEG_OFFSET_SIZE];

 
uint32_t detectRam(void){
    uint32_t totalCapacity = 0;
    struct E820Entry* table = (struct E820Entry*)E820_ADDR; //Ottenere la tavola della RAM dall'indirizzo del bootloader
    uint8_t counter = *(uint8_t*)E820_COUNT_ADDR; 

    //Traversarla e segnare ogni segmento utile 
    for(uint8_t i = 0; i < counter; i++){
        if(table[i].length == 0 || table[i].type != 1 || table[i].baseAddr == 0x0) continue; //Se il tipo non è 1, è riservata. Riserviamo NOI i primi 640KB
        totalCapacity += table[i].length;
    }

     

    return totalCapacity; 
    

}
MMU_t* mmuInit(){    
    miniMMU = (MMU_t){
        .capacity = 0,
        .freeAmount = 0, 
        .segmentBitmap = segmentBitmap
    };
    
    miniMMU.capacity = detectRam();
    miniMMU.freeAmount = miniMMU.capacity;//All'inizio tutta la RAM è libera

    //Inizializziamo la bitmap a 0
    for(uint16_t i = 0; i < 1024; i++){
        miniMMU.segmentBitmap[i] = 0;
    } 
    state = &miniMMU;
    return state;
}
void* genericAlloc(size_t size){
    if(size < 1) return NULL; //Dimensione invalida
     

    //Ottenere quanti segmenti occupa la dimensione richiesta (per eccesso)
    uint16_t segCount = size;
    uint16_t  effectiveSize = size + sizeof(SegmentHeader_t); //Bisogna tener conto dell'header
    if(effectiveSize % SEG_SIZE == 0) segCount = effectiveSize / SEG_SIZE;
    else segCount = effectiveSize / SEG_SIZE +1;

    //Cercare per abbastanza segmenti liberi di fila
    uint16_t actualCapacity = state->capacity / SEG_SIZE / 32; //La bitmap supporta fino a 256MB di RAM, realisticamente ne hai 4  
    uint16_t remaining =  segCount;
    uint8_t* base = 0;
    for(uint16_t i = 0; i < actualCapacity; i++){
        uint8_t found = 0; //Se abbiamo trovato quel che cerchiamo
        for(uint8_t j = 0; j < 32; j++){
            if(!remaining){ //Abbiamo trovato i segmenti che ci servono
                found = 1;
                break;
            }
            if(!(state->segmentBitmap[i] & ((uint32_t)1 << j))){
                if(!base){ //Il primo segmento libero che incontriamo, impostiamone l'indirizzo assoluto in un puntatore
                    uint16_t segNumber = i*32+j;
                    base = (uint8_t*)(RAM_START_ADDR + segNumber * SEG_SIZE);
                }
                remaining--;
            }
            else{ //La sequenza non è più contigua, ricominciare a cercare
                base = 0; 
                remaining = segCount;
            }

        }
        if(found) break; //Uscire dal ciclo
    }
    if(remaining) return NULL; //Non c'è spazio

    //Creare l'header
    SegmentHeader_t header = {
        .base = base + sizeof(header),
        .segmentCount = segCount
    };
    
    //Marcare i segmenti del caso come occupati
    uint32_t segNumber = (uint32_t)(base - RAM_START_ADDR) / SEG_SIZE; //Riottenere il numero del segmento dall'indirizzo assoluto
    uint16_t pageNumber = segNumber / 32; //Il numero della pagina
    uint16_t segOffset = segNumber % 32; //Il numero del segmento relativo alla pagina
    
    for(int i = 0; i < segCount; i++){
        if(segOffset == 32){ //Abbiamo raggiunto un confine di pagina
            segOffset = 0;
            pageNumber++;
        }
        
        state->segmentBitmap[pageNumber] |= ((uint32_t)1 << segOffset++); //Impostare il prossimo bit a 0
        state->freeAmount -= SEG_SIZE;
    }

    //Piazzare l'header all'inizio del primo segmento
    *(SegmentHeader_t*)base = header;

     
    return header.base;

}   
 