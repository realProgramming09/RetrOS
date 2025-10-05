#include "kernel/memory.h"
#include "kernel/sys.h"
#include "kernel/sysio.h" 
#include "kernel/terminal.h"
#include "kernel/bsod.h"
#include "kernel/queue.h"

#define E820_ADDR 0x4000
#define E820_COUNT_ADDR 0x1FF0
#define SEG_SIZE 0x40 //128B
#define SEG_OFFSET_SIZE 0x20000
#define RAM_START_ADDR 0x100000
#define BITMAP_ENTRY_SIZE 32

#define INT_SIZE 4
#define SMALL_STRING_SIZE 8
#define STRING_SIZE 16
#define STRUCT_SIZE 32
#define LARGE_STRUCT_SIZE 64

//Struct of an E820 entry: base address, entry size, entry tipe (reserved, useful, broken etc.) and optional info
struct E820Entry{
    uint64_t baseAddr; 
    uint64_t length; 
    uint32_t type;  
    uint32_t additional;  
}__attribute__((packed));


//Struct of an allocation header: the pointer's address and the allocation size in segments
typedef struct SegmentHeader{
    void* base;
    uint32_t segmentCount; 
}__attribute__((packed)) SegmentHeader_t;


static int firstFreePage = 0;
static MMU_t* state;
static uint32_t initialAddress = NULL;
static uint32_t segmentBitmap[SEG_OFFSET_SIZE];

 

static inline void* MMUAlloc(MMU_t* local, size_t size){
     state = local;
     state->segmentBitmap = segmentBitmap;
     return genericAlloc(size);
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
    
    
    for(uint32_t i = 0; i < SEG_OFFSET_SIZE; i++){ 
        segmentBitmap[i] = (i < 1024) ? m->segmentBitmap[i] : 0;
    }
     
    //Allocate state on RAM and set it up
    state = MMUAlloc(m, sizeof(MMU_t));
    *state = (MMU_t){
        .capacity = m->capacity,
        .freeAmount = m->freeAmount,
        .segmentBitmap = segmentBitmap
    };
    

    //Mark &state as occupied so it doesn't get corrupted
    initialAddress = (uint32_t)&state;
    int segAbsNumber = (initialAddress - RAM_START_ADDR) / SEG_SIZE;
    state->segmentBitmap[segAbsNumber / BITMAP_ENTRY_SIZE] |= ((uint32_t)1 << segAbsNumber % BITMAP_ENTRY_SIZE); //Impostare il segmento dove si trova a 1 
     
 
}
void* genericAlloc(size_t size){
    if(size < 1) return NULL; //Dimensione invalida
    if(!state->capacity){
        panic(MMU_OVERWRITE);
    }
    else if(initialAddress != (uint32_t)&state && initialAddress > 0){
        panic(MMU_CORRUPTION);
    }
    

    //Calculate how many segments the allocation occupies. Approx. by excess. 
    uint32_t segCount = size;
    uint32_t  effectiveSize = size + sizeof(SegmentHeader_t); //Accounting for header
    if(effectiveSize % SEG_SIZE == 0) segCount = effectiveSize / SEG_SIZE;
    else segCount = effectiveSize / SEG_SIZE +1;

    //Search for enough contigous bits
    uint32_t actualCapacity = state->capacity / SEG_SIZE / BITMAP_ENTRY_SIZE; //The RAM size in entries, to not iterate on RAM that you don't have 
    uint16_t remaining =  segCount;
    uint32_t segmentNumber = 0;
    for(uint32_t i = firstFreePage; i < actualCapacity && remaining; i++){
        if(state->segmentBitmap[i] == 0xFFFFFFFF){ //Entry completely full
            firstFreePage = i;
            continue;
        } 
         
        for(uint8_t j = 0; j < 32; j++){
            if(!(state->segmentBitmap[i] & ((uint32_t)1 << j))){
                if(!segmentNumber){ //The first 0 we found: store the absolute address elsewhere
                    segmentNumber = i * BITMAP_ENTRY_SIZE + j;
                     
                }
                if(!--remaining) break;
            }
            else{ //Sequence too short, continue searching
                segmentNumber = 0; 
                remaining = segCount;
            }

        }
        
    }
    if(remaining){//Out of memory
        panic(MALLOC_FAILED);
    }; 

    //Create the header
    SegmentHeader_t header = {
        .base = (void*)(segmentNumber * SEG_SIZE + RAM_START_ADDR + sizeof(header)), //Calculate the base from the segment number accounting for header
        .segmentCount = segCount
    };
    
    //Mark the segments as occupied 
    uint32_t pageNumber = segmentNumber / BITMAP_ENTRY_SIZE;  //Page number
    uint16_t segOffset = segmentNumber % BITMAP_ENTRY_SIZE;  //Segment number relative to page
    
    if(pageNumber < firstFreePage) firstFreePage = pageNumber;
    for(int i = 0; i < segCount; i++){
        if(segOffset ==  BITMAP_ENTRY_SIZE ) { //We reached a page end
            segOffset = 0;
            pageNumber++;
        }
        
        state->segmentBitmap[pageNumber] |= ((uint32_t)1 << segOffset++); //Set the current bit to 1
        state->freeAmount -= SEG_SIZE;
        
    }

    

    //Place header just before pointer
    *(SegmentHeader_t*)(header.base - sizeof(header)) = header; 
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

    //Get back the header from the pointer
    SegmentHeader_t header = *(SegmentHeader_t*)(ptr - sizeof(header));  
    if(header.base != ptr) return; //Pointer isn't allocated or something went wrong

    uint32_t segNumber = (uint32_t)(ptr - RAM_START_ADDR - sizeof(header)) / SEG_SIZE; //Get the starting segment's number back from absolute address
   
    //Calculate which part to free
    uint32_t pageNumber = segNumber / BITMAP_ENTRY_SIZE; //Il numero della pagina
    uint16_t segOffset = segNumber % BITMAP_ENTRY_SIZE; //Il numero del segmento relativo alla pagina
    
    for(int i = 0; i < header.segmentCount; i++){
        if(segOffset == BITMAP_ENTRY_SIZE){ //We reached a page end
            segOffset = 0;
            pageNumber++;
        }
        if(state->segmentBitmap[pageNumber] & ((uint32_t)1 << segOffset)){//Set the next bit to 0 if it isn't
            state->segmentBitmap[pageNumber] &= ~((uint32_t)1 << segOffset); 
            state->freeAmount += SEG_SIZE;
        }
        segOffset++;
        
        
    }

    


}
void* genericRealloc(void* ptr, size_t size){
    if(!state->capacity){
        panic(MMU_OVERWRITE);
    }
    else if(initialAddress != (uint32_t)&state){
        panic(MMU_CORRUPTION);
    }
    void* newPtr = genericAlloc(size); //Allocate new pointer
    if(!newPtr){
        panic(MALLOC_FAILED);
    }; 
    
    //Obtain the headers of both allocations
    SegmentHeader_t prevHeader = *(SegmentHeader_t*)(ptr - sizeof(SegmentHeader_t));
    SegmentHeader_t newHeader = *(SegmentHeader_t*)(newPtr - sizeof(SegmentHeader_t));

    //Calculate the smallest pointers's size to avoid overflow
    uint16_t actualSegCount = (prevHeader.segmentCount > newHeader.segmentCount) ? newHeader.segmentCount : prevHeader.segmentCount;
    uint16_t actualSize = actualSegCount*SEG_SIZE - sizeof(SegmentHeader_t);

    //Copy data
    uint8_t* a = ptr;
    uint8_t* b = newPtr;
    for(uint16_t i = 0; i < actualSize; i++){
        b[i] = a[i];
    }

    genericFree(ptr); //Free old pointer
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