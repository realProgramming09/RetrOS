#include "kernel/terminal.h"
#include "kernel/sysio.h"
#include "kernel/sys.h"
#include "kernel/serial.h"

//Macro per lo schermo
#define VGA_TEXT_ADDRESS 0xB8000
#define TEXT_MAX_WIDTH 80
#define TEXT_MAX_HEIGHT 25

//Macro per le porte del cursore
#define CURSOR_REGISTERS 0x3D4
#define CURSOR_DATA 0x3D5

typedef struct Terminal {
    uint16_t* frameBuffer; //Puntatore alla VRAM
    uint8_t color; //Colore del terminale
    uint8_t lineNumber; //Numero linea corrente
    uint16_t cursorPosition; //Posizione del cursore
} Terminal_t;
 
static Terminal_t terminal;
static uint8_t charX; //Posizione del carattere sulla linea

void scrollTerminal(){
    //Copiare ogni riga di terminale nella precedente
    for(int i = 0; i < TEXT_MAX_HEIGHT-1; i++){
        for(int j = 0; j < TEXT_MAX_WIDTH; j++){
            terminal.frameBuffer[j + i * TEXT_MAX_WIDTH] = terminal.frameBuffer[j + (i-1)*TEXT_MAX_WIDTH];
        }
    }

    //Svuotare l'ultima riga
    for(int i = 0; i < TEXT_MAX_WIDTH; i++){
        terminal.frameBuffer[i + (TEXT_MAX_HEIGHT-1)*TEXT_MAX_WIDTH] = (terminal.color << 8) | ' ';
    }
    terminal.lineNumber--;
}
static inline void updateCursor(uint16_t pos){
    sendByte(CURSOR_REGISTERS, 0x0F); sendByte(CURSOR_DATA, pos & 0xFF);
    sendByte(CURSOR_REGISTERS, 0x0E); sendByte(CURSOR_DATA, (pos >> 8) & 0xFF);
    terminal.cursorPosition = pos;
}
static inline void newLine(){
    charX = 0;
    if(++terminal.lineNumber >= TEXT_MAX_HEIGHT) scrollTerminal(); //Scrollare per necessario
}
static inline void deleteChar(){
    if(charX > 0) terminal.frameBuffer[--charX + TEXT_MAX_WIDTH * terminal.lineNumber] = (terminal.color << 8) | ' '; //Scrivere in memoria video uno spazio per cancellare
}
static inline void putChar(char c){
    terminal.frameBuffer[charX++ + TEXT_MAX_WIDTH * terminal.lineNumber] = (terminal.color << 8) | c; //Scrivere in memoria video
    if(charX > TEXT_MAX_WIDTH) newLine(); //Andare a capo se necessario
}
static inline void setCursorBlink(int isBlinking){
    sendByte(CURSOR_REGISTERS, 0x0A);
    uint8_t data = recByte(CURSOR_DATA);
    sendByte(CURSOR_DATA, isBlinking ? data & 0x1F : data | 0x20); //Abilitare o disabilitare il bit 5 del cursore
}
static inline void enableCursor(){
    // 1. ABILITA il cursore (bit 5 = 0) nel registro 0x0A
    sendByte(CURSOR_REGISTERS, 0x0A);
    uint8_t current = recByte(CURSOR_DATA);
    sendByte(CURSOR_DATA, (current & 0x1F));  // CLEAR bit 5 (0x1F = 00011111b)
    
    // 2. Imposta forma (opzionale, ma tu lo vuoi)
    sendByte(CURSOR_REGISTERS, 0x0A);
    sendByte(CURSOR_DATA, (recByte(CURSOR_DATA) & 0xE0) | 14);  // Start scanline 14
    
    sendByte(CURSOR_REGISTERS, 0x0B);
    sendByte(CURSOR_DATA, (recByte(CURSOR_DATA) & 0xE0) | 15);  // End scanline 15

    // 3. Aggiorna posizione
    updateCursor(0);
}
void terminalInit(){
    terminal = (Terminal_t){ //Impostare il terminale
        .color = 0x0F,
        .cursorPosition = 0,
        .frameBuffer = (uint16_t*)VGA_TEXT_ADDRESS,
        .lineNumber = 0,
    };
    clearTerminal(); //Pulire il terminale
    enableCursor();
    charX = 0;
}
void clearTerminal(){
    for(int i = 0; i < TEXT_MAX_HEIGHT; i++){
        for(int j = 0; j < TEXT_MAX_WIDTH; j++){
            terminal.frameBuffer[j + i * TEXT_MAX_WIDTH] = (terminal.color << 8) | ' ';
        }
    }
    terminal.lineNumber = 0;
    charX = 0;
}
void setTerminalColor(uint8_t color){
    terminal.color = color;
}
uint8_t getTerminalColor(){
    return terminal.color;
}
void moveCursor(int x, int y){
    updateCursor(x + y * TEXT_MAX_WIDTH);
}
void printToTerminal(const char* s, int isStepping){
    if(!s) return;

    //Scorrere la stringa
    int x = charX, y = terminal.lineNumber;
    for(int i = 0; s[i] != 0; i++){
        if(s[i] == '\b') deleteChar(); //Stiamo cancellando
        else if(s[i] == '\t'){ //Stiamo tabulando
            for(int j = 0; j < 4; j++) putChar(' ');
        }
        else if(s[i] == '\n') newLine();
        else putChar(s[i]);
    }
    if(isStepping){
        charX = x;
        terminal.lineNumber = y;
    }
}
void scanTerminal(uint8_t* buffer, size_t size){
    if(!buffer || size < 1) return;

    //Pulire il buffer
    for(size_t i = 0; i < size; i++) buffer[i] = 0;

   
    size_t lastChar = 0; //Indice dell'ultimo carattere
    int startX = charX; //Posizione iniziale dalla quale accettare input da tastiera
   
    moveCursor(charX, terminal.lineNumber);
    setCursorBlink(1);

    
    char c;
    do {
         
        c = getChar();
        if(!c) continue;
        if(c == '\n'){
            newLine();
            break;
        }

        if(c == '\b' && charX > startX){ //Cancellare
            updateCursor(terminal.cursorPosition-1);
            deleteChar();
            if(lastChar > 0) buffer[--lastChar] = 0; //Rimuovere il carattere dal buffer

        }
        else if(charX < TEXT_MAX_WIDTH -1 && c != '\b'){ //Inserire effettivamente
            if(lastChar < size-1) buffer[lastChar++] = c; //Appendere il carattere al buffer
            updateCursor(terminal.cursorPosition+1);
            putChar(c);
        }
    } while(1); //Finché non si preme INVIO
    buffer[lastChar] = 0; //Nullterminare il buffer

    setCursorBlink(0);
}
 