#include "sys.h"
#include "terminal.h"
#include "files.h"
#pragma once


 
//Enum to pass in datatypes
typedef enum{
    INT,
    CHAR,
    STRING, //String*
    FLOAT,
    STRING_IMMEDIATE, //char*
} DataType;

extern uint8_t currentKeyPressed;

void print(DataType type, const void* data); //Prints something, taking the type as a parameter
void println(DataType type, const void* data); //print() that does '\n'
void printStatic(DataType type, const void* data); //print() that does not step forward. Useful for printing static text
char getChar(); //Gets a single char from keyboard

//ASM functions: send and receive a byte from a port
extern void sendByte(uint16_t port, uint8_t value); 
extern uint8_t recByte(uint16_t port);  

//ASM functions: send and receive 2 bytes from a port
extern void sendWord(uint16_t port, uint16_t value); //Manda 2 byte alla porta specificata
extern uint16_t recWord(uint16_t port); //Riceve 2 byte dalla porta specificata