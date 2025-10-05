#include "user/kernelAPI.h"
#include "kernel/serial.h"
#include "kernel/files.h"
#include "kernel/print.h"
 
uint32_t syscall(uint32_t syscallNumber, uint32_t param1, uint32_t param2, uint32_t param3){
    uint32_t result;
    asm volatile(
        "int $0x30"
        : "=a"(result)                  // Output in EAX
        : "a"(syscallNumber),           // Input in EAX  
          "b"(param1),                  // EBX
          "c"(param2),                  // ECX
          "d"(param3)                   // EDX
        : "memory"                      // Clobber memory
    );
    return result;
}
void sendSerial(uint32_t ID, void* buffer, size_t size){
    syscall(SYSCALL_WRITECOM, ID, (uint32_t)buffer, (uint32_t)size);
}
void* malloc(size_t size){
    if(size < 1) return NULL;
    void* data = (void*)syscall(SYSCALL_MALLOC, (uint32_t)size, NULL, NULL);
    return data;
}
void free(void* ptr){
    if(!ptr) return;
    syscall(SYSCALL_FREE, (uint32_t)ptr, NULL, NULL);
}
void* realloc(void* ptr, size_t size){
    if(!ptr || size < 1) return NULL;
    void* newData = (void*)syscall(SYSCALL_REALLOC, (uint32_t)ptr, (uint32_t)size, NULL);
    return newData;
}
void printf(const char* format, ...){
    if(!format) return;


    //Creare un buffer per contenere cosa dobbiamo stampare
    uint16_t length = strlen(format); 
    char toPrint[length];

    //Puntatore allo stack dal quale prenderemo i dati
    uint32_t* stackPointer = (uint32_t*)(&format + 1);

     
    sendSerial(COM1, format, length);
    sendSerial(COM1, "\n\n", 1);
    

    //Scorrere la stringa
    int j = 0;
    for(int i = 0; i < length; i++){      
        if(format[i] == '%'){
            //Stampare ciò che abbiamo ricavato finora e pulire il buffer
            if(j > 0){
                toPrint[j] = 0;
                syscall(SYSCALL_PRINT, (uint32_t)toPrint, STRING_IMMEDIATE, NULL);
                j = 0;
            }

            i++;     
            if(format[i] == 's'){ //Ottenere un char* dallo stack e passarlo nella syscall
                char* data = (char*)(*stackPointer);
                syscall(SYSCALL_PRINT, (uint32_t)data, STRING_IMMEDIATE, NULL);
                stackPointer++;
            }
            else if(format[i] == 'd'){
                int data = (int)(*stackPointer);      
                syscall(SYSCALL_PRINT, (uint32_t)&data, INT, NULL);
                stackPointer++;
            }
            else if(format[i] == '%'){
                toPrint[j++] = '%';
            }
        }
        else toPrint[j++] = format[i];
    
        
    }
    if(j > 0) {
        toPrint[j] = 0;
        syscall(SYSCALL_PRINT, (uint32_t)toPrint, STRING_IMMEDIATE, NULL);
    }
}   
void scan(void* buffer, size_t size){
    if(!buffer) return;
    syscall(SYSCALL_SCAN, (uint32_t)buffer, size, NULL);
}
FILE* open(const char* path){
    if(!path) return NULL;

    //Allocare il file in RAM
    FILE* file = malloc(sizeof(FILE));
    file->name = malloc(64);
    strncpy(path, file->name, strlen(path));

    //Syscall ritorna dati del file
    file->contents = (void*)syscall(SYSCALL_OPEN, (uint32_t)&file->size, (uint32_t)file->name, NULL);
    if(file->contents == (void*)INVALID_PATH || file->contents == (void*)NOT_FOUND || file->contents == (void*)NAME_TOO_LONG){
        free(file->name);
        free(file);
        return file->contents;
    }
    return file;
}
void close(FILE* file){
    //if(!file) return;
    syscall(SYSCALL_WRITE, (uint32_t)file->name, (uint32_t)file->contents, file->size);
    free(file->name);
    free(file->contents);
    free(file);
}
void write(FILE* file, void* buffer, size_t size){
    if(!file || !buffer || size < 1) return;

    strncpy((char*)buffer, (char*)file->contents, size);
    file->size = size;
}
void flushFile(FILE* file){
    if(!file) return;
    syscall(SYSCALL_WRITE, (uint32_t)file->name, (uint32_t)file->contents, file->size);
}
void exit(int code){
    syscall(SYSCALL_EXIT, code, NULL, NULL);
}
void readNextCluster(FILE* file){
    if(!file) return;
    //QUANDO IL FILE MULTICLUSTER SARÀ UNA COSA
}
void strcpy(const char* s1, char* s2){
    if(!s1 || !s2) return;
    
    int l1 = strlen(s1), l2 = strlen(s2);
    
    int i = 0;
    for(; i < l1; i++) s2[i] = s1[i];
    for(; i < l2; i++) s2[i] = 0;
}
int strlen(const char* s){
    if(!s) return -1;
    int length;
    for(length = 0; s[length]; length++);
    return length;
}
int strcmp(const char* s1, const char* s2){
    if(!s1 || !s2) return -2;

    int length = strlen(s1);
    for(int i = 0; i < length; i++){
        if(s1[i] > s2[i]) return 1;
        else if(s1[i] < s2[i]) return -1;
    }
    return 0;
}
void strncpy(const char* s1, char* s2, size_t size){
    if(!s1 || !s2 || size < 1) return;

    int l1 = strlen(s1), l2 = strlen(s2);
    
    int i = 0;
    for(; i < l1 && i < size; i++) s2[i] = s1[i];
    for(; i < l2 && i < size; i++) s2[i] = 0;
}
int strncmp(const char* s1, const char* s2, size_t size){
    if(!s1 || !s2 || size < 1) return -2;

    int length = strlen(s1);
    for(int i = 0; i < length && i < size; i++){
        if(s1[i] > s2[i]) return 1;
        else if(s1[i] < s2[i]) return -1;
    }
    return 0;
}
char* itoa(int n){
    char invertedStr[12] = ""; //Buffer che conterrà n...al contrario
    static char str[12] = ""; //Buffer che conterrà n veramente

    uint8_t isNegative = 0; //Attenti al segno
    if(n < 0){
        str[isNegative++] = '-';
        n = -n;
    }
    
    uint8_t i = isNegative; //Traduciamo n in stringa
    do{
        invertedStr[i++] = n%10 + '0';
        n /= 10;
    } while(n > 0);
    

    //Ora bisogna invertire il nostro buffer
    for(uint8_t j = isNegative; j < i; j++){
        str[j] = invertedStr[i-j-1];
    }

    //Null-terminiamo il buffer
    str[i] = '\0';
    return str;
}