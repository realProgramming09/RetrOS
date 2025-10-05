#include "kernel/keyboard.h"
#include "kernel/memory.h"

static char keyboardCodes[] = {
    0x1B, -1, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', 'i', '\b', 
    0x09, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 'e', '+', '\n',
    -3, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'o', 'a', 'u', 
    -4, '<', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', -3, -1, -5, ' '
};
static uint8_t isShiftPressed;

void initKeyboard(){
    reserve(keyboardCodes, 59);
    reserve(&keyboardCodes, sizeof(keyboardCodes));
}
char getChar(){
     
    char currentChar = 0;
    currentKeyPressed = 0; //Per sicurezza
    while(!currentKeyPressed){
        asm volatile("hlt");//Aspettiamo un input (il multitasking non esiste, quindi è così)
    }
        
    if(currentKeyPressed == 0x2A){ 
        isShiftPressed = 1;
        return 0;
    }
    else if(currentKeyPressed == 0xAA){
        isShiftPressed = 0;
        return 0;
    } 
    else if(currentKeyPressed >= 59){
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