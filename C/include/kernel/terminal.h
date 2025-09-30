#include "sys.h"
#include "str.h"
#pragma once

//Macro per semplificare la stampa a colori
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define PURPLE 5
#define BROWN 6
#define GRAY 7
#define DARK_GRAY 8
#define LIGHT_BLUE 9
#define LIGHT_GREEN 0xA
#define LIGHT_CYAN 0xB
#define LIGHT_RED 0xC
#define LIGHT_PURPLE 0xD
#define YELLOW 0xE
#define WHITE 0xF

void terminalInit(); //Inizializza il terminale 

void setTerminalColor(const uint8_t color); //Imposta il colore
uint8_t getTerminalColor(); //Ritorna il colore

void printToTerminal(const char* s); //Stampa la stringa sul terminale
void scanTerminal(uint8_t* buffer, size_t size); //Mette l'input da tastiera nel buffer
void moveCursor(int x, int y); //Muove il cursore
void clearTerminal(); //Pulisce il terminale

void scrollTerminal(); //Scorre il terminale verso il basso
