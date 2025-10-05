#include "kernel/print.h"
#include "kernel/fixedMath.h"
#include "kernel/terminal.h"
#include "kernel/memory.h"

void printFloat(float f, int isStatic);
void printc(char c, int isStatic);
void prints(const String*, int isStatic);
void printn(int32_t n, int isStatic);
void printImmediate(const char* data, int isStatic);


void printFloat(float f, int isStatic){ 
    int integer = (int)f; //Isolare e stampare la parte intera
    printn(integer, isStatic);
    

    float decimal = f-integer; //Isolare la parte decimale
    if(decimal != 0) printc('.', isStatic);
    for(int i = 0; i < 4 && decimal != 0; i++){
        //Ricavare la prossima cifra e stamparla
        decimal *= 10; 
        printc('0' + (int)decimal % 10, isStatic);
        decimal -= (int) decimal; //Levare la cifra
    }
}  
void printc(char c, int isStatic){
    char s[] = {c, '\0'};
    printToTerminal(s, isStatic);
   
}
void prints(const String* s, int isStatic){ 
    char* ptr = strPointer(s);
    printToTerminal(ptr, isStatic); //Qui è più diretta
     
}
void printn(int32_t n, int isStatic){
    char invertedStr[12] = ""; //Buffer che conterrà n...al contrario
    char str[12] = ""; //Buffer che conterrà n veramente

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
    printToTerminal(str, isStatic);
}
void printImmediate(const char* data, int isStatic){
    printToTerminal(data, isStatic);
}
void print(DataType type, const void* data){
   
    switch(type){ //Chiamare la funzione giusta per ogni dataType
        case INT:
            printn(*(int*)data, 0);
            break;
        case STRING:
            prints((String*)data, 0);
            break;
        case CHAR:
            printc(*(char*)data, 0);
            break;
        case FLOAT:
            printFloat(*(float*)data, 0);
            break;
        case STRING_IMMEDIATE:
            printImmediate(data, 0);
            break;
    }
}
void println(DataType type, const void* data){
    print(type, data);
    printToTerminal("\n", 0);
}

void printStatic(DataType type, const void* data){
    switch(type){ //Chiamare la funzione giusta per ogni dataType
        case INT:
            printn(*(int*)data, 1);
            break;
        case STRING:
            prints((String*)data, 1);
            break;
        case CHAR:
            printc(*(char*)data, 1);
            break;
        case FLOAT:
            printFloat(*(float*)data, 1);
            break;
        case STRING_IMMEDIATE:
            printImmediate(data, 1);
            break;
    }
}
 