#include "kernel/sys.h"

//Macro per le syscall
#define SYSCALL_MALLOC 0
#define SYSCALL_FREE 1
#define SYSCALL_REALLOC 2

#define SYSCALL_PRINT 3
#define SYSCALL_GETCHAR 4
#define SYSCALL_SCAN 5 

#define SYSCALL_OPEN 6
#define SYSCALL_WRITE 7

#define SYSCALL_WRITECOM 8
#define SYSCALL_EXIT 9

typedef struct FILE{
    char* name; //Nome file
    void* contents; //Contenuti del file
    size_t size; //Dimensione del file
}FILE;

uint32_t syscall(uint32_t syscallNumber, uint32_t param1, uint32_t param2, uint32_t param3); //Imposta i registri e lancia l'interrupt

void* malloc(size_t size); //Alloca RAM
void free(void* ptr); //Libera RAM
void* realloc(void* ptr, size_t size); //Aggiusta lo spazio di un puntatore in RAM

void printf(const char* format, ...); //Stampa a schermo
void scan(void* buffer, size_t size); //Riceve dei dati da tastiera
char getChar(); //Ritorna un singolo carattere da tastiera

FILE* open(const char* path); //Apre un file
void close(FILE* file); //Chiude un file
void write(FILE* file, void* contents, size_t size); //Aggiorna i contenuti del file
void flushFile(FILE* file); //Scrive il file sul disco
void readNextCluster(FILE* file); //Legge il prossimo cluster del file

void sendSerial(uint32_t ID, void* buffer, size_t size);

char* itoa(int n); //Porta un numero in stringa

void exit(int code); //Esce prematuramente