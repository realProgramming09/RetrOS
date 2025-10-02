#include "sys.h"
#pragma once

typedef struct String String;
typedef struct {
    String** data;
    int length;
} StringArray;

String* new(const char* str); //Inizializza una stringa in RAM
String* newBuffer(int capacity); //Inizializza un buffer in RAM

StringArray* split(String* s, char c); //Spezza la stringa in un array di stringhe basandosi sul carattere
StringArray* splitQuotes(String* s, char c); //Spezza la stringa in un array di stringhe basandosi sul carattere, ma trattando i pezzi tra virgolette come unici a prescindere

void setChar(String* s, char c, int index); //Imposta il carattere all'indice specificato
char charAt(String* s, int index); //Ritorna il carattere all'indice specificato

int compareStrings(String* s1, String* s2); //Compara le stringhe
int compareImmediate(String* s1, const char* s2, size_t size); //Compara una stringa e un char*
String* concatAndCopyStrings(String* s1, String* s2); //Concatena le stringhe, caricando il risultato in RAM
String* copyString(String* s); //Copia la stringa in RAM
void copyImmediate(String*s, char* str, size_t size); //Copia la stringa nel char* 
void concatStrings(String* s1, String* s2); //Concatena le stringhe
void append(String* s, char c); //Appende il carattere alla stringa

int strLength(const String* s); //Ritorna la lunghezza della stringa
int strCapacity(const String* s); //Ritorna la capacità della stringa (utile nei buffer)
char* strPointer(const String* s); //Ritorna una copia del char* della stringa null terminato in RAM

void clearString(String* s); //Pulisce la stringa
void unloadString(String* s); //Scarica ls stringa dalla RAM
void unloadArray(StringArray* a); //Scarica l'array di stringhe dalla RAM

void strcpy(const char* s1, char* s2); //Copia una stringa in un altra
int strcmp(const char* s1, const char* s2); //Compara due stringhe
int strlen(const char* s1); //Ritorna la lunghezza della stringa
void strncpy(const char* s1, char* s2, size_t size); //Copia una stringa in un altra evitando l'overflow
int strncmp(const char* s1, const char* s2, size_t size);  //Compara due stringhe evitando l'overflow