#include "sys.h"

typedef struct Queue Queue_t;

Queue_t* createQueue(size_t elementSize, size_t size); //Creates a queue as specified
void freeQueue(Queue_t* queue); //Frees a queue

int queueAdd(Queue_t* queue, void* data); //Adds a new element to the queue
void queueGet(Queue_t* queue, void* buffer, size_t size); //Pops an element from the queue and copies it into buffer

//Getters for the queue's variables
int queueSize(Queue_t* queue);
int queueElementSize(Queue_t* queue);
int queueCapacity(Queue_t* queue);