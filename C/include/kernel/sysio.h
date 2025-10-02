#include "sys.h"
#include "terminal.h"
#include "files.h"
#pragma once


 
//Enum per passare i datatype
typedef enum{
    INT,
    CHAR,
    STRING,
    FLOAT,
    STRING_IMMEDIATE,
} DataType;

extern uint8_t currentKeyPressed;

void print(DataType type, const void* data); //Stampa qualsiasi cosa a schermo, prendendo come parametro il tipo
void println(DataType type, const void* data); //Print che manda a capo
char getChar(); //Ottiene un carattere da tastiera

extern void sendByte(uint16_t port, uint8_t value); //Manda un byte alla porta specificata
extern uint8_t recByte(uint16_t port); //Riceve un byte dalla porta specificata

extern void sendWord(uint16_t port, uint16_t value); //Manda 2 byte alla porta specificata
extern uint16_t recWord(uint16_t port); //Riceve 2 byte dalla porta specificata