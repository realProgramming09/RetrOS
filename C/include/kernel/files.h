#include "sys.h"
#include "str.h"
#pragma once
 
#define FREE_ENTRY 0xE5

//Struttura che contiene i dati del file
typedef struct File{
    String* name; //Il nome e l'estensione del file
    uint16_t creationTime; //Ora di creazione
    uint16_t creationDate; //Data di creazione
    uint32_t size; //Dimensione del file in byte
    String* path; //Percorso del file
    uint8_t* contents; //I contenuti del file
    uint32_t clusterNumber; //Numero del cluster dove inizia
}__attribute__((packed)) File_t ;

//Struttura che contiene i dati della cartella (più per leggibilità)
typedef struct Folder{
    String* name; //Il nome della cartella
    uint16_t creationTime; //Ora di creazione
    uint16_t creationDate; //Data di creazione
    String* path;//Il percorso della cartella
    uint32_t clusterNumber; //Numero del cluster dove inizia
    struct DirectoryEntry* entries; //I contenuti della cartella 
    uint32_t elements; //Quanti elementi contiene la cartella
    uint32_t totalSize; //Dimensione totale degli elementi nella cartella
}__attribute__((packed)) Folder_t;

//Struttura che contiene i dati di ogni entry (più per leggibilità)
typedef struct DirectoryEntry{
    uint8_t name[8]; //Il nome del file
    char extension[3]; //L'estensione del file
    uint8_t attributes; //Attributi del file
    uint8_t reserved; //Riservato
    uint8_t msCreationTime; //Tempo di creazione, in ms
    uint16_t creationTime; //Ora di creazione
    uint16_t creationDate; //Data di creazione
    uint16_t lastAccessDate; //Data di ultimo accesso
    uint16_t clusterNumberHigh; //Primi 2 byte del cluster
    uint16_t lastModificationTime; //Ora dell'ultima modifica
    uint16_t lastModificationDate; //Data dell'ultima modifica
    uint16_t clusterNumberLow; //Ultimi 2 byte del cluster
    uint32_t size; //Dimensione del file in byte
}__attribute__((packed)) DirectoryEntry_t ;

//Macro che definiscono valori comuni
#define NAME_SIZE 8
#define EXTENSION_SIZE 3

//Macro che definiscono codici errore
#define ALREADY_EXISTS 1 
#define NAME_TOO_LONG 2
#define NO_FREE_CLUSTERS 3
#define NOT_FOUND 4
#define NOT_EMPTY 5
#define INVALID_PATH 6


void rootDirInit(); //Inizializza la root directory

int newFile(String* path); //Crea un nuovo file al percorso specificato
File_t* openFile(String* path); //Apre un file esistente caricandolo in RAM
void writeToFile(File_t* file, void* contents, size_t size); //Scrive e aggiorna il file sul disco
void closeFile(File_t* file); //Scarica il file dalla RAM, scrivendolo in automatico sul disco


int newFolder(String* path); //Crea una cartella al percorso specificato o ne apre una esistente caricandola in RAM
Folder_t* openFolder(String* path); //Ritorna in modo leggibile i contenuti della cartella al percorso specificato caricando tutto in RAM
Folder_t* getParent(String* path); //Ritorna la cartella genitore 
Folder_t* getRoot(); //Ottiene la root directory
void closeFolder(Folder_t* folder); //Scarica la cartella dalla RAM

int renameFile(String* path, String* newName); //Rinomina un file al percorso specificato
int renameDirectory(String* path, String* newName); //Rinomina una directory al percorso specificato
int deleteFile(String* path); //Rimuove un file al percorso specificato
int deleteDirectory(String* path); //Rimuove una cartella al percorso specificato

uint32_t getDiskCapacity(); //Ottiene la capacità del disco
uint32_t getDiskFreeSpace(); //Ottiene lo spazio libero sul disco



