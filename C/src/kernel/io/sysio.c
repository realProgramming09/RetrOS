#include "kernel/sysio.h"
#include "kernel/fixedMath.h"
#include "kernel/terminal.h"
#include "kernel/mmu.h"

void printFloat(float f);
void printc(char c);
void prints(const String* s);
void printn(int32_t n);
void printImmediate(const char* data);

static char keyboardCodes[] = {
    0x1B, -1, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', 'i', '\b', 
    0x09, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 'e', '+', '\n',
    -3, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'o', 'a', 'u', 
    -4, '<', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', -3, -1, -5, ' '
};
static uint8_t isShiftPressed;

void printFloat(float f){ 
    int integer = (int)f; //Isolare e stampare la parte intera
    printn(integer);
    

    float decimal = f-integer; //Isolare la parte decimale
    if(decimal != 0) printc('.');
    for(int i = 0; i < 4 && decimal != 0; i++){
        //Ricavare la prossima cifra e stamparla
        decimal *= 10; 
        printc('0' + (int)decimal % 10);
        decimal -= (int) decimal; //Levare la cifra
    }
}  
void printc(char c){
    char s[] = {c, '\0'};
    printToTerminal(s);
   
}
void prints(const String* s){ 
    char* ptr = strPointer(s);
    printToTerminal(ptr); //Qui è più diretta
    genericFree(ptr);
}
void printn(int32_t n){
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
    printToTerminal(str);
}
void printImmediate(const char* data){
    printToTerminal(data);
}
void print(DataType type, const void* data){
   
    switch(type){ //Chiamare la funzione giusta per ogni dataType
        case INT:
            printn(*(int*)data);
            break;
        case STRING:
            prints((String*)data);
            break;
        case CHAR:
            printc(*(char*)data);
            break;
        case FLOAT:
            printFloat(*(float*)data);
            break;
        case STRING_IMMEDIATE:
            printImmediate(data);
            break;
    }
}
void println(DataType type, const void* data){
    print(type, data);
    printToTerminal("\n");
}
char getChar(){
     
    char currentChar = 0;
    currentKeyPressed = 0; //Per sicurezza
    while(currentKeyPressed <= 0); //Aspettiamo un input (il multitasking non esiste, quindi è così)
        
    if(currentKeyPressed == 0x2A) isShiftPressed = 1;
    else if(currentKeyPressed == 0xAA) isShiftPressed = 0;
    
    if(currentKeyPressed >= 59 || currentKeyPressed == 0x2A || currentKeyPressed == 0xAA){
        currentKeyPressed = 0;
        return 0;
    }
    
    currentChar = keyboardCodes[currentKeyPressed]; //Ottenere il carattere mappato
    if(isShiftPressed){
        if(currentChar >= 'a' && currentChar <= 'z') currentChar -= 0x20; //Lettera maiuscola
        else if(currentChar >= '1' && currentChar <= '9') currentChar -= 0x10; //Simbolo sul numero
    }
    

    return currentChar;

}
 