#include "kernel/mmu.h"
#include "kernel/sys.h"
#include "kernel/sysio.h" 
#include "kernel/terminal.h"
#include "kernel/bsod.h"


#define E820_ADDR 0x4000
#define E820_COUNT_ADDR 0x1FF0
#define SEG_SIZE 0x80 //128B
#define SEG_OFFSET_SIZE 0x10000
#define RAM_START_ADDR 0x100000
#define BITMAP_ENTRY_SIZE 32

#define BSOD(x) \

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

MMU_t initialMMU;
static MMU_t* state;
static uint32_t initialAddress = NULL;
uint32_t segmentBitmap[SEG_OFFSET_SIZE];

void* MMUAlloc(MMU_t* local, size_t size){
     if(size < 1) return NULL; //Dimensione invalida
     

    //Ottenere quanti segmenti occupa la dimensione richiesta (per eccesso)
    uint16_t segCount = size;
    uint16_t  effectiveSize = size + sizeof(SegmentHeader_t); //Bisogna tener conto dell'header
    if(effectiveSize % SEG_SIZE == 0) segCount = effectiveSize / SEG_SIZE;
    else segCount = effectiveSize / SEG_SIZE +1;

    //Cercare per abbastanza segmenti liberi di fila
    uint16_t actualCapacity = local->capacity / SEG_SIZE / 32; //La bitmap supporta fino a 256MB di RAM, realisticamente ne hai 4  
    uint16_t remaining =  segCount;
    uint8_t* base = 0;
    for(uint16_t i = 0; i < actualCapacity; i++){
        if(local->segmentBitmap[i] == 0xFFFFFFFF) continue; //Entry del tutto piena
        uint8_t found = 0; //Se abbiamo trovato quel che cerchiamo
        for(uint8_t j = 0; j < BITMAP_ENTRY_SIZE; j++){
            if(!remaining){ //Abbiamo trovato i segmenti che ci servono
                found = 1;
                break;
            }
            if(!(local->segmentBitmap[i] & ((uint32_t)1 << j))){
                if(!base){ //Il primo segmento libero che incontriamo, impostiamone l'indirizzo assoluto in un puntatore
                    uint16_t segNumber = i * BITMAP_ENTRY_SIZE + j;
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
    if(remaining){ //Non c'è spazio
        panic(MALLOC_FAILED);
    } 

    //Creare l'header
    SegmentHeader_t header = {
        .base = base + sizeof(header),
        .segmentCount = segCount
    };
    
    //Marcare i segmenti del caso come occupati
    uint32_t segNumber = (uint32_t)(base - RAM_START_ADDR) / SEG_SIZE; //Riottenere il numero del segmento dall'indirizzo assoluto
    uint16_t pageNumber = segNumber / BITMAP_ENTRY_SIZE; //Il numero della pagina
    uint16_t segOffset = segNumber % BITMAP_ENTRY_SIZE; //Il numero del segmento relativo alla pagina
    
    for(int i = 0; i < segCount; i++){
        if(segOffset == BITMAP_ENTRY_SIZE){ //Abbiamo raggiunto un confine di pagina
            segOffset = 0;
            pageNumber++;
        }
        
        local->segmentBitmap[pageNumber] |= ((uint32_t)1 << segOffset++); //Impostare il prossimo bit a 0
        local->freeAmount -= SEG_SIZE;
    }

    //Piazzare l'header all'inizio del primo segmento
    *(SegmentHeader_t*)base = header;

     
    return header.base;

}
uint32_t detectRam(void){
    
    
    uint32_t totalCapacity = 0;
    struct E820Entry* table = (struct E820Entry*)E820_ADDR; //Ottenere la tavola della RAM dall'indirizzo del bootloader
    uint8_t counter = *(uint8_t*)E820_COUNT_ADDR; 

    //Traversarla e segnare ogni segmento utile 
    for(uint8_t i = 0; i < counter; i++){
        if(table[i].length == 0 || table[i].type != 1 || table[i].baseAddr == 0x0) continue; //Se il tipo non è 1, è riservata. Non tocchiamo RAM < 1MB
        totalCapacity += table[i].length;
    }

     

    return totalCapacity; 
    

}
 
void loadMMU(MMU_t* m){
    initialMMU = (MMU_t){
        .capacity = m->capacity,
        .freeAmount = m->freeAmount,
        .segmentBitmap = segmentBitmap,
    };
    for(uint32_t i = 0; i < SEG_OFFSET_SIZE; i++){ 
        initialMMU.segmentBitmap[i] = (i < 1024) ? m->segmentBitmap[i] : 0;
    }

    //Allocare la MMU in RAM e configurarla
    state = NULL;
    state = MMUAlloc(&initialMMU, sizeof(MMU_t));
    state->capacity = initialMMU.capacity;
    state->freeAmount = initialMMU.freeAmount;
    state->segmentBitmap = segmentBitmap;
    initialAddress = (uint32_t)&state;

    //Impostare la regione dove si trova state come occupata sennò si sovrascrive
    uint32_t stateAddr = initialAddress;
    int segAbsNumber = (stateAddr - RAM_START_ADDR) / SEG_SIZE;
    state->segmentBitmap[segAbsNumber / BITMAP_ENTRY_SIZE] |= ((uint32_t)1 << segAbsNumber % BITMAP_ENTRY_SIZE); //Impostare il segmento dove si trova a 1 
     
}
void* genericAlloc(size_t size){
    if(size < 1) return NULL; //Dimensione invalida
    if(!state->capacity){
        panic(MMU_OVERWRITE);
    }
    else if(initialAddress != (uint32_t)&state){
        panic(MMU_CORRUPTION);
    }
    

    //Ottenere quanti segmenti occupa la dimensione richiesta (per eccesso)
    uint16_t segCount = size;
    uint16_t  effectiveSize = size + sizeof(SegmentHeader_t); //Bisogna tener conto dell'header
    if(effectiveSize % SEG_SIZE == 0) segCount = effectiveSize / SEG_SIZE;
    else segCount = effectiveSize / SEG_SIZE +1;

    //Cercare per abbastanza segmenti liberi di fila
    uint16_t actualCapacity = state->capacity / SEG_SIZE / BITMAP_ENTRY_SIZE;  //La bitmap supporta fino a 256MB di RAM, realisticamente ne hai 4  
    uint16_t remaining =  segCount;
    uint8_t* base = 0;
    for(uint16_t i = 0; i < actualCapacity; i++){
        if(state->segmentBitmap[i] == 0xFFFFFFFF) continue; //Entry del tutto piena
        uint8_t found = 0; //Se abbiamo trovato quel che cerchiamo
        for(uint8_t j = 0; j < 32; j++){
            if(!remaining){ //Abbiamo trovato i segmenti che ci servono
                found = 1;
                break;
            }
            if(!(state->segmentBitmap[i] & ((uint32_t)1 << j))){
                if(!base){ //Il primo segmento libero che incontriamo, impostiamone l'indirizzo assoluto in un puntatore
                    uint16_t segNumber = i * BITMAP_ENTRY_SIZE + j;
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
    if(remaining){//Non c'è spazio
        panic(MALLOC_FAILED);
    }; 

    //Creare l'header
    SegmentHeader_t header = {
        .base = base + sizeof(header),
        .segmentCount = segCount
    };
    
    //Marcare i segmenti del caso come occupati
    uint32_t segNumber = (uint32_t)(base - RAM_START_ADDR) / SEG_SIZE; //Riottenere il numero del segmento dall'indirizzo assoluto
    uint16_t pageNumber = segNumber / BITMAP_ENTRY_SIZE;  //Il numero della pagina
    uint16_t segOffset = segNumber % BITMAP_ENTRY_SIZE;  //Il numero del segmento relativo alla pagina
    
    for(int i = 0; i < segCount; i++){
        if(segOffset ==   BITMAP_ENTRY_SIZE ) { //Abbiamo raggiunto un confine di pagina
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

void genericFree(void* ptr){
    if(!state->capacity){
        panic(MMU_OVERWRITE);
    }
    else if(initialAddress != (uint32_t)&state){
        panic(MMU_CORRUPTION);
    }
    if(!ptr) return; //Il puntatore è nullo
    SegmentHeader_t header = *(SegmentHeader_t*)(ptr - sizeof(header)); //Ricavare l'header dall'area allocata
    if(header.base != ptr) return; //Hai passato un puntatore a cazzo oppure qualcosa è andato storto

    uint32_t segNumber = (uint32_t)(ptr - RAM_START_ADDR) / SEG_SIZE; //Riottenere il numero del segmento dall'indirizzo assoluto
    uint16_t segCount = header.segmentCount; //Quanti segmenti da liberare
    
    uint16_t pageNumber = segNumber / 32; //Il numero della pagina
    uint16_t segOffset = segNumber % 32; //Il numero del segmento relativo alla pagina
    
    for(int i = 0; i < segCount; i++){
        if(segOffset == 32){ //Abbiamo raggiunto un confine di pagina
            segOffset = 0;
            pageNumber++;
        }
        if(state->segmentBitmap[pageNumber] & ((uint32_t)1 << segOffset)){
            state->segmentBitmap[pageNumber] &= ~((uint32_t)1 << segOffset); //Impostare il prossimo bit a 0
            state->freeAmount += SEG_SIZE;
        }
        segOffset++;
        
        
    }

    ptr = NULL;


}
void* genericRealloc(void* ptr, size_t size){
    if(!state->capacity){
        panic(MMU_OVERWRITE);
    }
    else if(initialAddress != (uint32_t)&state){
        panic(MMU_CORRUPTION);
    }
    void* newPtr = genericAlloc(size);
    if(!newPtr){
        panic(MALLOC_FAILED);
    }; 
    
    //Ottenere gli header delle due allocazioni
    SegmentHeader_t prevHeader = *(SegmentHeader_t*)(ptr - sizeof(SegmentHeader_t));
    SegmentHeader_t newHeader = *(SegmentHeader_t*)(newPtr - sizeof(SegmentHeader_t));

    //Ottenere la dimensione in byte dell'allocazione più piccola per evitare overflow
    uint16_t actualSegCount = (prevHeader.segmentCount > newHeader.segmentCount) ? newHeader.segmentCount : prevHeader.segmentCount;
    uint16_t actualSize = actualSegCount*SEG_SIZE - sizeof(SegmentHeader_t);

    //Copiare un puntatore nell'altro
    uint8_t* a = ptr;
    uint8_t* b = newPtr;
    for(uint16_t i = 0; i < actualSize; i++){
        b[i] = a[i];
    }

    genericFree(ptr);
    return newPtr;
}
uint32_t getTotalRam(){
    if(!state->capacity){
        panic(MMU_OVERWRITE);
    }
    else if(initialAddress != (uint32_t)&state){
        panic(MMU_CORRUPTION);
    }
    return state->capacity;
}
uint32_t getFreeRam(){
    if(!state->capacity){
        panic(MMU_OVERWRITE);
    }
    else if(initialAddress != (uint32_t)&state){
        panic(MMU_CORRUPTION);
    }
    return state->freeAmount;
}
Page_t requestPage(){
    if(!state->capacity){
        panic(MMU_OVERWRITE);
    }
    else if(initialAddress != (uint32_t)&state){
        panic(MMU_CORRUPTION);
    }
    int number = 0;
    for(; number < SEG_OFFSET_SIZE; number++){
        if(!state->segmentBitmap[number]){
            Page_t page = {
                .pageNumber = number,
                .base = (void*)(number * BITMAP_ENTRY_SIZE * SEG_SIZE + RAM_START_ADDR), //L'indirizzo assoluto della pagina
            };
            state->segmentBitmap[number] = 0xFFFFFFFF;
            return page;
        }
    }
    panic(MALLOC_FAILED);
    return (Page_t){0};
}
void freePage(Page_t* page){
    if(!state->capacity){
        panic(MMU_OVERWRITE);
    }
    else if(initialAddress != (uint32_t)&state){
        panic(MMU_CORRUPTION);
    }
     
    state->segmentBitmap[page->pageNumber] = 0;
    page->base = NULL;
}