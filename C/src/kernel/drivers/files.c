#include "kernel/files.h"
#include "kernel/disk.h"
#include "kernel/memory.h"
#include "kernel/str.h"
 
//Macro che definiscono gli attributi dei file
#define DIRECTORY 0x10
#define READ_ONLY 1
#define VOLUME_ID 8
#define ARCHIVE 0x20
#define SYSTEM 4

//Macro che definiscono modalità del path-parser
#define BAD_CLUSTER 0xFFFFFFF7
#define FREE_CLUSTER 0
#define EOF 0xFFFFFFF8



struct BPB{
    uint8_t jmp[3]; //Il codice necessario
    char oemID[8]; //Identificatore OEM
    uint16_t sectorSize; //Byte per settore
    uint8_t clusterSize; //Settori per cluster
    uint16_t reservedSectors; //Settori riservati
    uint8_t FATs; //Quante FAT ci sono
    uint16_t rootEntries; //Quante entry ha la root
    uint16_t totalSectors; //Numero di settori
    uint8_t mediaType; //Tipo media
    uint16_t sectorsPerFat; //Settori per FAT
    uint16_t trackSize; //Settori per track
    uint16_t heads; //Teste
    uint32_t hiddenSectors; //Quanti settori nascosti
    uint32_t largeTotalSectors; //Numero di settori se > 32MB o FAT32
}__attribute__((packed));

struct FAT32_EBPB{
    uint32_t sectorsPerfat; //Settori per FAT
    uint16_t flags; //Flags.
    uint16_t revision; //Revisione
    uint32_t rootCluster; //Cluster della root
    uint16_t FSInfo; //Posizione della FSInfo
    uint16_t backup; //Posizione del backup
    uint32_t reserved1;
    uint16_t reserved2;
    uint16_t driveNumber; //Numero disco
    uint8_t reserved3;
    uint8_t signature;
    uint32_t serialNumber; //Numero seriale
    char label[12]; //Nome disco
    char fileSys[8]; //FileSystem
}__attribute__((packed));

typedef struct{
    Folder_t* parentDirectory; //Directory genitore
    DirectoryEntry_t* currentEntry; //Entry nella parent che rispecchia l'elemento al quale punta il path
} PathInfo;

struct FSinfo{
    uint32_t leadSignature; //Firma iniziale
    char reserved[480]; //Riservato
    uint32_t midSignature; //Firma centrale
    uint32_t freeClusters; //Quanti settori sono liberi
    uint32_t firstCluster; //Primo cluster buono
    char reserved2[12]; //Riservato
    uint32_t trailSignature; //Firma finale
}__attribute__((packed));

static struct BPB* bpb;
static struct FAT32_EBPB* ebpb;
static struct FSinfo* fs;
static uint32_t* FAT;
uint32_t FATsize = 0;
uint32_t entriesSize;
uint32_t rootCluster = 2;


/* FUNZIONI INTERNE, PER ASTRARRE TASK RIPETITIVE */
DirectoryEntry_t* findEntry(Folder_t* folder, String* fullName, uint8_t attributes); //Scorre una cartella per cercare un'entry specifica
void removeEntry(DirectoryEntry_t* entry); //Marca un'entry come libera
PathInfo parsePath(String* path, int attribute); //Scorre il percorso ritornando la cartella genitore e l'entry che rappresenta ciò che il path descrive accertandosi che esiste
 
/* FUNZIONI PRINCIPALI CHE FANNO IL LAVORO SPORCO */
int delete(String* path, int attribute); //Cancella file/cartella
int create(String* path, int attribute); //Crea  file/cartella
void* open(String* path, int attributes);//Apre file/cartella
int rename(String* path, String* newName, int attributes); //Rinomina file/cartella

static inline uint32_t getSectorNumber(uint32_t clusterNumber){
    int firstSector = bpb->reservedSectors + ebpb->sectorsPerfat;
    return firstSector+(clusterNumber-2)*bpb->clusterSize;
}
static inline void* readCluster(uint32_t clusterNumber){
    uint32_t sectorNumber = getSectorNumber(clusterNumber);
    void* cluster = readSectors(sectorNumber, bpb->clusterSize);
    return cluster;
}
static inline void writeCluster(uint32_t clusterNumber, void* data){
    uint32_t sectorNumber = getSectorNumber(clusterNumber);
    writeSector(sectorNumber, (uint16_t*)data, bpb->clusterSize * bpb->sectorSize);
}
static inline void updateFilesystem(){
    writeSector(bpb->reservedSectors, (uint16_t*)FAT, ebpb->sectorsPerfat*bpb->sectorSize); //Riscrive la FAT nel settore
    writeSector(7, (uint16_t*)fs, bpb->sectorSize);
}

Folder_t openRoot(){
    //Creare il folder_t
    Folder_t folder = {
        .creationDate = 0,
        .creationTime = 0,
        .elements = 0,
        .clusterNumber = rootCluster,
        .path = newBuffer(64),
        .name = new("/")
    };
    
    folder.entries = readCluster(folder.clusterNumber);
    
    
    for(int i = 0; i < entriesSize && folder.entries[i].name[0] != 0; i++){
        if((uint8_t)folder.entries[i].name[0] != FREE_ENTRY){
            folder.elements++;
            folder.totalSize += folder.entries[i].size;
            
        }
    }
    return folder;
}
DirectoryEntry_t* findEntry(Folder_t* folder, String* fullName, uint8_t attributes){
    if(!folder->entries || !folder || !fullName || !attributes) return (DirectoryEntry_t*)NOT_FOUND;

    StringArray* tokens = split(fullName, '.'); //Spezzare il nome completo in nome ed estensione
    DirectoryEntry_t* entry = (DirectoryEntry_t*)NOT_FOUND;

    //Scorrere la cartella alla ricerca della entry giusta
    for(int i = 0; folder->entries[i].name[0] != 0; i++){
        if((uint8_t)folder->entries[i].name[0] == FREE_ENTRY) continue; //Entry libera
        if(folder->entries[i].attributes != attributes) continue; //L'attributo deve coincidere

        if(!compareImmediate(tokens->data[0], (char*)folder->entries[i].name, NAME_SIZE)){
            if(attributes == 0x20){ //Se è un file, bisogna controllare anche l'estensione
                if(!strLength(tokens->data[1]) && folder->entries[i].extension[0] == 0){ //Se manca a uno, deve mancare all'altro
                    entry = &folder->entries[i];
                    break;
                }
                else if(!compareImmediate(tokens->data[1], folder->entries[i].extension, EXTENSION_SIZE)){
                    entry = &folder->entries[i];
                    break;
                }
            }
            else{
                entry = &folder->entries[i];
                break;
            }
        }

    }

    unloadArray(tokens);
    return entry;
}
void removeEntry(DirectoryEntry_t* entry){  
    if(!entry) return;

    //Segnare l'elemento come libero nella cartella, nella FAT e nella FSinfo
    entry->name[0] = FREE_ENTRY;

    int cluster = (entry->clusterNumberHigh << 16) | entry->clusterNumberLow; 
    FAT[cluster] = 0;
    fs->freeClusters++;
    updateFilesystem();
     
}
PathInfo parsePath(String* path, int attribute){
    PathInfo dummy = {.currentEntry = NULL}; //Pathinfo da ritornare in caso di errore
 
    if(!path || (attribute != DIRECTORY && attribute != ARCHIVE)) return dummy;

    String* name = newBuffer(12); //ALlocare una stringa per contenere il nome
    String* parentName = NULL; //Allocare (dopo, nel codice) una stringa contenente il nome della cartella genitore
    String* parentPath = newBuffer(64); //Allocare una stringa per contenere il percorso della cartella genitore
    uint8_t pathLength = strLength(path); //La lunghezza del percorso

   

    //Scorrere il percorso alla ricerca del nostro file
    Folder_t* parent = genericAlloc(sizeof(Folder_t)); //Allocare la parent directory
    parent->clusterNumber = rootCluster;
    
    uint8_t extension = 0;
    for(int i = 0, j = 0; i < pathLength; i++){
        if(j >= NAME_SIZE && !extension){
            genericFree(parent);
            unloadString(parentName);
            unloadString(parentPath);
            unloadString(name);
            return (PathInfo){.currentEntry = (DirectoryEntry_t*)NAME_TOO_LONG};
        }
        if(!extension && charAt(path, i) == '.') extension = 1;
        if(charAt(path, i) == '/'){ //Siamo a un nome di cartella

            //Leggere gli elementi della cartella e vedere se la sottocartella esiste
            DirectoryEntry_t* entries = readCluster(parent->clusterNumber);
            uint8_t found = 0;
            for(int j = 0; entries[j].name[0] != 0; j++){
                if(entries[j].name[0] == FREE_ENTRY) continue;
                if(!compareImmediate(name, (char*)entries[j].name, NAME_SIZE) && entries[j].attributes == DIRECTORY){
                    parent->clusterNumber = (entries[j].clusterNumberHigh << 16) | entries[j].clusterNumberLow;
                    found = 1;
                    break;
                }
            }
            genericFree(entries);


            if(!found){ //IL percorso non esiste
                unloadString(name);
                unloadString(parentName);
                unloadString(parentPath);
                return (PathInfo){ .currentEntry = (DirectoryEntry_t*)INVALID_PATH};
            }
            
            //Preparare per la prossima iterazione
            if(strLength(parentPath)) append(parentPath, '/'); //Continuare a costruire il percorso della cartella genitore
            concatStrings(parentPath, name);

            //Salvare il nome corrente
            unloadString(parentName); //La MMU è immune a liberare un NULL ovviamente
            parentName = copyString(name); 
            clearString(name);
            j = 0;
        }
        else{ 
            append(name, charAt(path, i));
            j++;
        }
    }

    if(!parentName || !strLength(parentName)) *parent = openRoot(); //Se abbiamo non almeno un nesting (es a/b), la cartella genitore è la root
    else { //Altrimenti è da impostare
        parent->creationTime = 0;
        parent->creationDate = 0;
        parent->entries = readCluster(parent->clusterNumber);
        parent->name = copyString(parentName);
        parent->path = copyString(parentPath);
        for(int i = 0;parent->entries[i].name[0] != 0; i++){
            if((uint8_t)parent->entries[i].name[0] != FREE_ENTRY){
               parent->totalSize +=parent->entries[i].size;
               parent->elements++;
            }
        }
    }
    //Cercare l'entry nella cartella parent e impostare il pathinfo
    PathInfo info = {
        .parentDirectory = parent,
        .currentEntry = findEntry(parent, name, attribute)
    };
 

    //Scaricare la RAM
    unloadString(name);
    unloadString(parentName);
    unloadString(parentPath);

    return info;
}

int delete(String* path, int attribute){
    if(!path) return NOT_FOUND;

    StringArray* tokens = split(path, '/'); //Dividere il path in nomi del filesystem

    PathInfo info = parsePath(path, attribute);
    Folder_t* parent = info.parentDirectory;
    DirectoryEntry_t* entry = info.currentEntry;

    //Gestione errori
    if(!entry) return INVALID_PATH;
    if(entry == (DirectoryEntry_t*)INVALID_PATH) return NOT_FOUND;
    if(entry == (DirectoryEntry_t*)NAME_TOO_LONG) return NAME_TOO_LONG;
    if(entry == (DirectoryEntry_t*)NOT_FOUND){
        closeFolder(parent);
        return NOT_FOUND;
    }

    //Azzerare il cluster a cui la entry punta (a.k.a cancellare effettivamente il file/directory)
    int zero = 0, cluster = (entry->clusterNumberHigh >> 16) | entry->clusterNumberLow;
    writeCluster(cluster, &zero); 

    //Segnare la entry come libera, aggiornare la cartella sul disco  e scaricare la RAM
    removeEntry(entry);
    writeCluster(parent->clusterNumber, parent->entries);
    unloadArray(tokens);
    closeFolder(parent);

    return 0; //Tutto in ordine
}
int create(String* path, int attribute){
    PathInfo element = parsePath(path, attribute); //Parsare il path per ottenere il nome della cartella genitore
    
    //Gestione errori
    if(!element.currentEntry) return INVALID_PATH;
    else if(element.currentEntry == (DirectoryEntry_t*)INVALID_PATH) return NOT_FOUND;
    else if(element.currentEntry != (DirectoryEntry_t*)NOT_FOUND){
        closeFolder(element.parentDirectory);
        return ALREADY_EXISTS;
    } 
    if(element.currentEntry == (DirectoryEntry_t*)NAME_TOO_LONG) return NAME_TOO_LONG;
     

    //Estrapolare il nome del file/cartella dal path
    StringArray* name = split(path, '/');
    StringArray* dividedName = split(name->data[name->length-1], '.'); //Dividere in nome ed estensione
    Folder_t* parent = element.parentDirectory; //Per leggibilità

    //Cercare nella FAT il prossimo cluster libero
    int freeCluster = -1;
    for(int i = 3; i < FATsize; i++){
        if(!FAT[(freeCluster = i)]) break;
    }
    if(freeCluster < 0){ //Abbiamo finito i cluster
        unloadArray(name);
        unloadArray(dividedName);
        closeFolder(parent);
        return NO_FREE_CLUSTERS;
    }

    //Creare una directoryEntry che contiene i dati del nostro file/cartella (molti campi sono 0, non ho ancora l'RTC)
    DirectoryEntry_t newEntry = {
        .attributes = attribute,
        .creationDate = 0,
        .creationTime = 0,
        .msCreationTime = 0,
        .lastAccessDate = 0,
        .lastModificationDate = 0,
        .lastModificationTime = 0,
        .size = 0,
        .clusterNumberHigh = (freeCluster >> 16) & 0xFFFF,
        .clusterNumberLow = freeCluster & 0xFFFF
    };
    copyImmediate(dividedName->data[0], (char*)newEntry.name, NAME_SIZE);
    if(attribute != DIRECTORY && dividedName->length > 1) copyImmediate(dividedName->data[1], newEntry.extension, EXTENSION_SIZE); //Copiare l'estensione solo se è un file e solo se provvista

    //Mettere la entry nella cartella parent
    for(int i = 0; i < entriesSize; i++){
        if(parent->entries[i].name[0] == 0 || parent->entries[i].name[0] == FREE_ENTRY){
            parent->entries[i] = newEntry;
            break;
        }
    }

    //Aggiornare tutto sul disco: FAT, FSinfo e cartella genitore
    FAT[freeCluster] = EOF;
    fs->freeClusters--;
    updateFilesystem();
    writeCluster(parent->clusterNumber, parent->entries);

    //Scaricare la RAM
    unloadArray(name);
    unloadArray(dividedName);
    closeFolder(parent);

    return 0;
}
int rename(String* path, String* newName, int attributes){
    StringArray* dividedName = split(newName, '.'); //Dividere il nome dall'estensione
    PathInfo info = parsePath(path, attributes);

    //Leggibilità
    Folder_t* parent = info.parentDirectory;
    DirectoryEntry_t* entry = info.currentEntry;

    //Gestione errori
    if(!entry) return INVALID_PATH;
    if(entry == (DirectoryEntry_t*)INVALID_PATH) return NOT_FOUND;
    if(entry == (DirectoryEntry_t*)NAME_TOO_LONG) return NAME_TOO_LONG;
    if(entry == (DirectoryEntry_t*)NOT_FOUND){
        closeFolder(parent);
        unloadArray(dividedName);
        return NOT_FOUND;
    }

    //Copiare il nome (e l'estensione se è un file ed è provvista)
    copyImmediate(dividedName->data[0],(char*)entry->name, NAME_SIZE);
    if(attributes == ARCHIVE){
        if(dividedName->length > 1) copyImmediate(dividedName->data[1], entry->extension, EXTENSION_SIZE);
        else for(int i = 0; i < EXTENSION_SIZE; i++) entry->extension[i] = 0;
    }

    //Aggiornare la cartella genitore sul disco
    writeCluster(parent->clusterNumber, parent->entries);

    //Scaricare la RAM
    closeFolder(parent);
    unloadArray(dividedName);

    return 0;

}
void* open(String* path, int attributes){
    if(!path) return NULL;
    if(!strLength(path)) return getRoot(); 

    PathInfo info = parsePath(path, attributes); //Scorrere il percorso alla ricerca del nostro file/cartella

    //Leggibilità
    DirectoryEntry_t* entry = info.currentEntry;
    Folder_t* parent = info.parentDirectory;
    StringArray* tokens = split(path, '/'); //Estrapolare il nome del file

    //Gestione errori
    if(!entry) return (DirectoryEntry_t*)INVALID_PATH;
    if(entry == (DirectoryEntry_t*)INVALID_PATH) return (DirectoryEntry_t*)NOT_FOUND;
    if(entry == (DirectoryEntry_t*)NAME_TOO_LONG) return (DirectoryEntry_t*)NAME_TOO_LONG;
    if(entry == (DirectoryEntry_t*)NOT_FOUND){
        closeFolder(parent);
        unloadArray(tokens);
        return(DirectoryEntry_t*) NOT_FOUND;
    }

    //Ricavare il numero del cluster
    int cluster = (entry->clusterNumberHigh << 16) | entry->clusterNumberLow;
    void* handler = NULL; //Handler generico

    //In base agli attributi, creare e ritornare un file_t o un folder_t
    if(attributes == ARCHIVE){
        File_t* file = genericAlloc(sizeof(File_t)); //Allocare il file in RAM
        *file = (File_t){
            .clusterNumber = cluster,
            .creationDate = 0,
            .creationTime = 0,
            .name = copyString(tokens->data[tokens->length-1]),
            .path = copyString(path),
            .size = entry->size,
        };
        file->contents = readCluster(file->clusterNumber);
        handler = file;
    }
    else if(attributes == DIRECTORY){
        Folder_t* folder = genericAlloc(sizeof(Folder_t));
        *folder = (Folder_t){
            .clusterNumber = cluster,
            .creationDate = 0,
            .creationTime = 0,
            .name = copyString(tokens->data[tokens->length-1]),
            .path = copyString(path)
        };

        //Scorrere la cartella e contare le entry
        folder->entries = readCluster(folder->clusterNumber);
        for(int i = 0; folder->entries[i].name[0] != 0; i++){
            if(folder->entries[i].name[0] != FREE_ENTRY){
                folder->elements++;
                folder->totalSize += folder->entries[i].size;
            }
        }

        handler = folder;
    }

    //Scaricare la RAM
    unloadArray(tokens);
    closeFolder(parent);

    return handler;
}
 
Folder_t* getRoot(){
    Folder_t* root = genericAlloc(sizeof(Folder_t));
    *root = openRoot();
    return root;
}
void rootDirInit(){
    uint8_t* bootSector = (uint8_t*)readSectors(0, 1); //Leggere il boot sector
    uint8_t* FSsector = (uint8_t*)readSectors(7, 1); //Leggere il settore della FSinfo

    //Leggere BPB, EBPB e FSinfo
    bpb = (struct BPB*)bootSector;
    ebpb = (struct FAT32_EBPB*)(bootSector + sizeof(struct BPB));
    fs = (struct FSinfo*)FSsector;
    entriesSize =  bpb->clusterSize * bpb->sectorSize / sizeof(DirectoryEntry_t);

    //Leggere la FAT
    FAT = (uint32_t*)readSectors(bpb->reservedSectors, ebpb->sectorsPerfat);
    FATsize = bpb->largeTotalSectors - bpb->clusterSize; //Impostare una dimensione per lavorarci

    //Impostare la FSinfo
    if(fs->freeClusters == 0xFFFFFFFF || fs->freeClusters > bpb->largeTotalSectors / bpb->clusterSize) fs->freeClusters = (bpb->largeTotalSectors - bpb->reservedSectors - bpb->sectorsPerFat) / bpb->clusterSize; //I primi 2 sono sempre occupati

    //Impostare la entry 0 e 1 e impostare il 2 cluster per la ROOT (per sicurezza)
    FAT[0] = bpb->mediaType;
    FAT[1] = 0xFFFFFFFF;
    FAT[2] = 0xFFFFFFF8; //La root occupa un solo cluster
 
 
    updateFilesystem();    
    genericFree(bootSector);
    genericFree(FSsector);
}
int newFolder(String* path){
    return create(path, DIRECTORY);
}
int newFile(String* path){
    return create(path, ARCHIVE);
}   
void writeToFile(File_t* file, void* contents, size_t size){
    if(!file || !contents || size < 1) return;; 

    uint8_t* bytes = (uint8_t*)contents; //Castare il buffer a un uint8_t per copiare
    
    int i = 0; //Copiare il file 
    for(; i < size; i++) file->contents[i] = bytes[i];
    for(; i < file->size; i++) file->contents[i] = 0;

    //Aggiornare le entry della parent, il file ha una dimensione diversa
    PathInfo info = parsePath(file->path, ARCHIVE);
    DirectoryEntry_t* entry = info.currentEntry;
    Folder_t* parent = info.parentDirectory;

    file->size = size;
    entry->size = size;
    
    writeCluster(parent->clusterNumber, parent->entries); //Aggiorna la cartella genitore sul disco
    writeCluster(file->clusterNumber, file->contents); //Aggiorna il file sul disco

    closeFolder(parent); //Scarica la RAM
}
void closeFile(File_t* file){
    if(!file || !file->contents) return;

    writeCluster(file->clusterNumber, file->contents); //Aggiornare il file sul disco
    unloadString(file->path);
    unloadString(file->name);
    genericFree(file->contents);
    genericFree(file);
}
void closeFolder(Folder_t* folder){
    if(!folder || !folder->entries) return;
    
    writeCluster(folder->clusterNumber, folder->entries); //Scrivere la cartella sul disco
    unloadString(folder->name);
    unloadString(folder->path);
    genericFree(folder->entries);
    genericFree(folder);
}
uint32_t getDiskCapacity(){
     return (bpb->largeTotalSectors - bpb->reservedSectors - bpb->sectorsPerFat)* bpb->sectorSize; //Ritorna la capacità del disco in bytes
}
uint32_t getDiskFreeSpace(){
    return fs->freeClusters * bpb->clusterSize * bpb->sectorSize; //Ritorna lo spazio libero in bytes
}
Folder_t* openFolder(String* path){
   return open(path, DIRECTORY);
}
Folder_t* getParent(String* path){
     PathInfo info = parsePath(path, ARCHIVE); //Gli attributi in realtà non servono, a noi ci interessa la cartella genitore
     return info.parentDirectory;
}
int deleteFile(String* path){
    return delete(path, ARCHIVE); //Possiamo affidarci interamente alla funzione interna
}
int deleteDirectory(String* path){
    Folder_t* folder = openFolder(path); //Dobbiamo fare dei controlli extra: la cartella deve essere vuota
    int code = 0;

    if(!folder->elements) code = delete(path, DIRECTORY);
    else code = NOT_EMPTY;

    closeFolder(folder);
    return code;
}
File_t* openFile(String* path){
     return open(path, ARCHIVE);
}
int renameFile(String* path, String* newName){
    return rename(path, newName, ARCHIVE);
}
int renameDirectory(String* path, String* newName){
    return rename(path, newName, DIRECTORY);
}