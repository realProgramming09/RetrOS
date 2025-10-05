#include "sys.h"
#include "terminal.h"
#include "files.h"
#pragma once


//ASM functions: send and receive a byte from a port
extern void sendByte(uint16_t port, uint8_t value); 
extern uint8_t recByte(uint16_t port);  

//ASM functions: send and receive 2 bytes from a port
extern void sendWord(uint16_t port, uint16_t value); //Manda 2 byte alla porta specificata
extern uint16_t recWord(uint16_t port); //Riceve 2 byte dalla porta specificata