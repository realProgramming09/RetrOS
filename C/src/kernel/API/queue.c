#include "kernel/queue.h"
#include "kernel/memory.h"


#define DEFAULT_SIZE 128
#define MIN_SIZE 64

typedef struct Queue{
    uint8_t* base; //The array that stores data
    size_t elementSize; //Size of each element
    int size; //How many elements
    int capacity; //Max size
    int head; //Index of first element
    int tail; //Index of last element
} Queue_t;

Queue_t* createQueue(size_t elementSize, size_t size){
    if(size < MIN_SIZE) size = DEFAULT_SIZE; //Queue size needs to be > 64 bytes to be worth it

    //Allocate queue on RAM and set it up
    Queue_t* queue = (Queue_t*)genericAlloc(sizeof(Queue_t));
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->elementSize = elementSize;
    queue->capacity = elementSize * size;
    
    //Allocate array on RAM
    queue->base = (uint8_t*)genericAlloc(queue->capacity);

    return queue;
}
int queueAdd(Queue_t* queue, void* data){
    if(!queue || !data) return -1;
    if(queue->size + queue->elementSize > queue->capacity) return 1;

    uint8_t* dataBytes = data; //Cast to uint8_t[] for copying
    for(int i = 0; i < queue->elementSize; i++){
        queue->base[queue->tail++ % queue->capacity] = dataBytes[i]; //Copy the element at the end, byte by byte
    }
    queue->size++;

    return 0;
}
void queueGet(Queue_t* queue, void* buffer, size_t size){
    if(!queue || queue->head == queue->tail) return;

    uint8_t* bufferBytes = (uint8_t*) buffer; //Cast to uint8_t[] for copying
    for(int i = 0; i < queue->elementSize; i++){
        if(i < size) bufferBytes[i] = queue->base[queue->head % queue->capacity]; //Copy wihout overflow
        queue->head++;
    }
}
int queueSize(Queue_t* queue){
    return queue ? queue->size : -1;
}
int queueCapacity(Queue_t* queue){
    return queue ? queue->capacity : -1;
}
int queueElementSize(Queue_t* queue){
    return queue ? queue->elementSize : -1;
}