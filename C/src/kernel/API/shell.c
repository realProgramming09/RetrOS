#include "kernel/str.h"
#include "kernel/memory.h"
#include "kernel/sysio.h"
#include "kernel/files.h"
#include "kernel/slup.h"
#include "kernel/bsod.h"
#include "kernel/timer.h"

#define COMMANDS_NUMBER 21
 
static String* shellPath = NULL;
static char osName[23] = "RetrOS v0.0.1 - build 4";

//The following functions are the commands that the shell supports. Unique commands are commented
void send(String* input); //Sends data via serial using the SLUP protocol
void benchmark(String* input); //Runs benchmark 
void load(String* input); //Reads a file's contents via serial
void run(String* input); //Launches a .bin user
void cp(String* input); 
void writeCommand(String* input); //Writes something to file
void cat(String* input);  
void suicideNow(String* input); //Kills himself
void help(String* input);  
void shutDown(String* input); //Prepares the PC for manual shutdown
void rmdir(String* input);  
void rm(String* input);  
void rndir(String* input); //Renames a directory
void ren(String* input); //Renames a file
void mkfile(String* input); //Creates a file
void clear(String* input);  
void cd(String* input);  
void mkdir(String* input); 
void sysinfo(String* input); //Obtains info about the system
void ls(String* input); 
void echo(String*input);
void notFound(); //Prints an error message

 
static inline String* getFullPath(String* path){
    String* fullPath = copyString(shellPath);
    if(charAt(fullPath, 0) != 0) append(fullPath, '/');
    concatStrings(fullPath, path);
    return fullPath;
} 

void initShell(){
    shellPath = newBuffer(64); //Shell path is allocated on RAM
}
void launchShell(){
    if(!shellPath) return;

    //La lista di comandi riconosciuti
    char* commandList[] = {
        "echo\0",
        "ls\0",
        "sysinfo\0",
        "mkdir\0",
        "cd\0",
        "clear\0",
        "mkfile\0",
        "ren\0",
        "rndir\0",
        "rm\0",
        "rmdir\0",
        "shutdown\0",
        "help\0",
        "suicidenow\0",
        "cat\0",
        "write\0",
        "cp\0",
        "run\0",
        "load\0",
        "bench\0",
        "send\0"
    };

    //Lista di callback per ogni comando
    void (*callbackList[])(String*) = {
        echo,
        ls,
        sysinfo,
        mkdir,
        cd,
        clear,
        mkfile,
        ren,
        rndir,
        rm,
        rmdir,
        shutDown,
        help,
        suicideNow,
        cat,
        writeCommand,
        cp,
        run,
        load,
        benchmark,
        send
    };
    
    for(;;){
        
        //Print shell's path
        setTerminalColor(LIGHT_GREEN);
        print(STRING_IMMEDIATE, "C:/\0");
        print(STRING, shellPath);
        print(STRING_IMMEDIATE, ">\0");
        setTerminalColor(WHITE);

        //Get keyboard input into a buffer
        char input[64];
        scanTerminal((uint8_t*)input, 64);

        if(input[0] == '\0') continue;

         
        //Parse the command list and run the right command
        int found = 0;
        String* inputString = new(input);
        StringArray* tokens = split(inputString, ' ');
        for(int i = 0; i < COMMANDS_NUMBER;  i++){
            if((found = !compareImmediate(tokens->data[0], commandList[i], strlen(commandList[i])))){
                callbackList[i](inputString); 
                break;
            } 
        }
        if(!found) notFound(); //Se no, dare un messaggio di errore
        unloadArray(tokens);
        unloadString(inputString);
        
    }
}
 
void echo(String* input){
    StringArray* tokens = splitQuotes(input, ' ');
    println(STRING, tokens->data[1]);
    unloadArray(tokens);
}
void notFound(){;
    println(STRING_IMMEDIATE, "Command does not exist. Type in 'help' for a list of commands.\0");
    
}
void ls(String* input){
    Folder_t* folder = openFolder(shellPath);
    
    
    if(!folder->elements){//La cartella è vuota
        println(STRING_IMMEDIATE, "The current folder is empty.\0"); 
        closeFolder(folder);
    }
    else{
        
        for(int i = 0; folder->entries[i].name[0] != 0; i++){
            if((uint8_t)folder->entries[i].name[0] != FREE_ENTRY){
                DirectoryEntry_t current = folder->entries[i];
                //Stampare info sul file/sottocartella nella nostra cartella
                print(STRING_IMMEDIATE, current.name); //Nome
                if(current.attributes & 0x20){ //Estensione se si tratta di un file
                    print(STRING_IMMEDIATE, ".\0");
                    print(STRING_IMMEDIATE, current.extension);
                    
                }
                print(STRING_IMMEDIATE, "   \0");
                if(current.attributes & 0x10) print(STRING_IMMEDIATE, "<DIR> \0"); //Specificare se si tratta di una directory
                else print(STRING_IMMEDIATE, "      ");

                print(INT, &current.size); //La dimensione in bytes
                println(STRING_IMMEDIATE, "B\0");
            }
           
        }
    }
    //print(STRING_IMMEDIATE, "\n"); 
    closeFolder(folder);
     
}
void sysinfo(String* input){

    //Gather info about RAM and disk and divide it by 1M to use MB
    float totalDiskSpace = getDiskCapacity() / 1048576.0f;  
    float freeDiskSpace = getDiskFreeSpace() / 1048576.0f;  
    float totalRamSpace = getTotalRam() / 1048576.0f;  
    float freeRamSpace = getFreeRam() / 1048576.0f;  

    setTerminalColor(RED);
    println(STRING_IMMEDIATE, osName);

    #define PRINT_INFO(intro, total, free)\
        setTerminalColor(YELLOW);\
        print(STRING_IMMEDIATE, intro);\
        setTerminalColor(WHITE);\
        print(FLOAT, &total);\
        print(STRING_IMMEDIATE, "MB (");\
        print(FLOAT, &free);\
        println(STRING_IMMEDIATE, "MB free)");\
    

    PRINT_INFO("RAM: ", totalRamSpace, freeRamSpace);
    PRINT_INFO("Storage: ", totalDiskSpace, freeDiskSpace);

   
}
void mkdir(String* input){
      
    //Parsare il comando alla ricerca del nome della cartella
    StringArray* tokens = split(input, ' ');

    //Gestione degli errori
    if(tokens->length == 1){
        
        print(STRING_IMMEDIATE, "Directory name is missing.\n\0");
         
        unloadArray(tokens);
         
        return;
    }
     
    //Concatenare il nome della cartella al percorso del terminale per avere il path completo
    String* newPath = getFullPath(tokens->data[1]);

    //Creare una cartella al nuovo percorso
    int errorCode = newFolder(newPath);
    
 
    //La creazione della cartella è andata a buon fine? 
    if(errorCode == ALREADY_EXISTS) println(STRING_IMMEDIATE, "A directory with the specified name already exists.\0");
    else if(errorCode == INVALID_PATH || errorCode == NOT_FOUND)  println(STRING_IMMEDIATE, "The specified path is invalid\0");
    else if(errorCode == NAME_TOO_LONG) println(STRING_IMMEDIATE, "Directory name is too long. Max 8 chars + 3 chars.\0");
    

    //Liberare la RAM
    unloadString(newPath);  
    unloadArray(tokens);
    

}
void cd(String* input){ 
    
    //Parsare il comando alla ricerca del nome della cartella
    StringArray* tokens = split(input, ' ');
    if(tokens->length == 1){ //Vedere se ci sono almeno 2 campi
        
        println(STRING_IMMEDIATE, "Directory name is missing.\0");
      
        unloadArray(tokens);
        
        return;
    }
    
    String* name = tokens->data[1];
     
    
    
    if(!compareImmediate(name, ".\0", 1) && strLength(name) == 1){ //Siamo CDando nella cartella corrente (aka non stiamo facendo un cazzo)
        unloadArray(tokens);
        
        return;
    }
    if(!compareImmediate(name, "..\0", 2) && charAt(shellPath, 0) != 0){ //Tornare alla cartella genitore
        Folder_t* parent = getParent(shellPath);
        if(charAt(parent->path, 0) == '\0') for(int i = 0; i < 64; i++) clearString(shellPath); //Pulire il percorso se nella root
        else{
            unloadString(shellPath);
            shellPath = copyString(parent->path);
        }
        
        //Scaricare la RAM e liberare il terminale
        closeFolder(parent);
        unloadArray(tokens);         
        
        
        return;
    }
  

    //Aprire la cartella dove si trova il terminale 
    Folder_t* currentFolder = openFolder(shellPath);
    

    //Gestione errori
    if(currentFolder == (Folder_t*)INVALID_PATH) println(STRING_IMMEDIATE, "Invalid path.\0");
    else if(currentFolder == (Folder_t*)NOT_FOUND) println(STRING_IMMEDIATE, "Specified folder does not exist.\0");
    else if(currentFolder == (Folder_t*)NAME_TOO_LONG) println(STRING_IMMEDIATE, "Directory name is too long. Max 8 chars + 3 chars.\0");
    else {
        for(int i = 0; currentFolder->entries[i].name[0] != 0; i++){ 
            if((uint8_t)currentFolder->entries[i].name[0] == FREE_ENTRY) continue;
            if(!compareImmediate(name, (char*)currentFolder->entries[i].name, NAME_SIZE) && currentFolder->entries[i].attributes == 0x10){
                if(charAt(shellPath, 0) != '\0') append(shellPath, '/'); //Aggiornare il percorso del terminale: abbiamo trovato la cartella che cercavamo 
                concatStrings(shellPath, name);            
                
                break;
            }
        }
        closeFolder(currentFolder);
    }

    //Scaricare la RAM e liberare il terminale
    unloadArray(tokens);
 
    
}
void clear(String* input){
    clearTerminal();
     
}
void mkfile(String* input){
    
    StringArray* tokens = split(input, ' ');
    if(tokens->length == 1){ //Controllare che esistano almeno due campi
        print(STRING_IMMEDIATE, "File name is missing.\0");  
        unloadArray(tokens);   
        return;
    }
    
    //Ottenere il percorso completo del file
    String* fullPath = getFullPath(tokens->data[1]);
    
    //Tentare di creare un nuovo file
    int errorCode = newFile(fullPath); //Creare il file
  

    //Gestione errori
    if(errorCode == ALREADY_EXISTS) println(STRING_IMMEDIATE, "The file with the specified name already exists.\0");
    else if(errorCode == NO_FREE_CLUSTERS) println(STRING_IMMEDIATE, "Disk is full, could not create file.\0");
    else if(errorCode == INVALID_PATH) println(STRING_IMMEDIATE, "Invalid path.\0");
    else if(errorCode == NAME_TOO_LONG) println(STRING_IMMEDIATE, "File name is too long. Max 8 chars + 3 chars.\0");

    //Scaricare la RAM e liberare il terminale
    
    unloadString(fullPath);
    unloadArray(tokens);
  
    
}
void ren(String* input){
    StringArray* tokens = split(input, ' '); //Separare il comando in una lista di parole;
    if(tokens->length < 3){ //Verificare la presenza di almeno 3 campi
        
       println(STRING_IMMEDIATE, "Incomplete command. Type in 'help' to get a list of commands\0");
       
       unloadArray(tokens);
       return;
    }
     
     //Chiamare il filesystem
    int errorCode = renameFile(tokens->data[1], tokens->data[2]);
  

    if(errorCode == INVALID_PATH) println(STRING_IMMEDIATE, "Invalid path.\0");
    else if(errorCode == NOT_FOUND) println(STRING_IMMEDIATE, "The specified file does not exist.\0");
    else if(errorCode == NAME_TOO_LONG) println(STRING_IMMEDIATE, "File name  is too long. Max 8 chars + 3 chars.\0");

    //Scaricare la RAM e liberare il terminale
    unloadArray(tokens);
    
}
void rndir(String* input){
     StringArray* tokens = split(input, ' '); //Separare il comando in una lista di parole;
    if(tokens->length < 3){ //Verificare la presenza di almeno 3 campi
       println(STRING_IMMEDIATE, "Incomplete command.\0");
       
       unloadArray(tokens);
       return;
    }
     
     //Chiamare il filesystem
    int errorCode = renameDirectory(tokens->data[1], tokens->data[2]);
   

    if(errorCode == INVALID_PATH) println(STRING_IMMEDIATE, "Invalid path.\0");
    else if(errorCode == NOT_FOUND) println(STRING_IMMEDIATE, "The specified directory does not exist.\0");
    else if(errorCode == NAME_TOO_LONG) println(STRING_IMMEDIATE, "Directory name is too long. Max 8 chars + 3 chars.\0");

    //Scaricare la RAM e liberare il terminale
    unloadArray(tokens);
    
    
}
void rm(String* input){
    StringArray* tokens = split(input, ' ');
    if(tokens->length == 1){ //Controllare la presenza di almeno un campo
        println(STRING_IMMEDIATE, "File name is missing.\0"); 
        unloadArray(tokens);  
        return;
    }

    //Ottenere il percorso completo
    String* fullPath = getFullPath(tokens->data[1]);
    int errorCode = deleteFile(fullPath); //Chiamare il filesystem

    //Gestione errori
    if(errorCode == INVALID_PATH) println(STRING_IMMEDIATE, "Invalid path.\0");
    else if(errorCode == NOT_FOUND) println(STRING_IMMEDIATE, "The specified file does not exist.\0");
    else if(errorCode == NAME_TOO_LONG) println(STRING_IMMEDIATE, "File name is too long. Max 8 chars + 3 chars.\0");

    //Scarica la RAM 
    unloadString(fullPath);
    unloadArray(tokens);
     
}
void rmdir(String* input){
    StringArray* tokens = split(input, ' ');
    if(tokens->length == 1){ //Verificare la presenza di almeno due campi
        println(STRING_IMMEDIATE, "Directory name is missing.\0");      
        unloadArray(tokens);    
        return;
    }
 
    ////Aprire la cartella 
    String* fullPath = getFullPath(tokens->data[1]);
    int code = deleteDirectory(fullPath);
     
    if(code == INVALID_PATH) println(STRING_IMMEDIATE, "Invalid path.\0");
    else if(code == NOT_FOUND) println(STRING_IMMEDIATE, "The directory does not exist.\0");
    else if(code == NOT_EMPTY) println(STRING_IMMEDIATE, "The directory is not empty.\0");
    else if(code == NAME_TOO_LONG) println(STRING_IMMEDIATE, "Directory name is too long. Max 8 chars + 3 chars.\0");
    
    //Scaricare la RAM 
    unloadString(fullPath);
    unloadArray(tokens);
    
}
void shutDown(String* input){
    clearTerminal();
    setTerminalColor(GRAY);
  
    println(STRING_IMMEDIATE, "You may now shutdown your computer.\0");
    println(STRING_IMMEDIATE, "(No, I can't do it myself. My OS my rules. Now go.)\0");

    asm volatile("cli");
    asm volatile("hlt"); //Il processore si incastra in un loop dal quale non può uscire. 
}
void help(String* input){
    int colors[2] = {YELLOW, (int)getTerminalColor()};
    char* messages[] = { //Messaggi di aiuto
        "echo [STRING] \0","- prints something\n\0",
        "ls \0","- displays current directory contents\n\0",
        "sysinfo \0","- displays info about RAM and storage\n\0",
        "mkdir [DIRECTORY_NAME] \0", "- creates a directory\n\0",
        "cd [DIRECTORY_NAME] \0", "- enters the specified diretory. Type in '..' to go back\n\0",
        "clear \0","- clears the screen\n\0",
        "mkfile [FILE_NAME] \0", "- creates a file\n\0",
        "rename [OLD_NAME] [NEW_NAME] \0", "- renames a file\n\0",
        "rndir [OLD_NAME] [NEW_NAME] \0", "- renames a directory\n\0",
        "shutdown \0", "- prepares the computer for manual shutdown\n\0",
        "suicidenow \0", "- dies on the spot\n\0",
        "cat [FILE_NAME] \0", "- displays a file's contents\n\0",
        "write [FILE_NAME] [DATA] \0", "- overwrites a file\n\0",
        "cp [FILE1_NAME] [FILE2_NAME] \0", "- copies a file into another, creating a new one or overwriting an existing one\n\0",
        "help \0","- prints this\n\0",
        "load [FILE_NAME] \0", "- listens to serial using the SLUP protocol and writes the contents to a file\n\0",
        "run [PROGRAM_NAME] \0", "- runs a .bin file\n\0",
        "bench \0", " - runs the same thing over and over and monitors how much it takes\n\0",
        "send [STRING] \0", "- sends something via serial using the SLUP protocol\n\0", "\0"
    };

    //Stampare i messaggi di aiuto
    for(int i = 0; messages[i][0] != '\0'; i++){
        setTerminalColor(colors[i%2]); //Il colore si alterna
        print(STRING_IMMEDIATE, messages[i]);
    }
    setTerminalColor(colors[1]);
   
}
void suicideNow(String* input){
    panic(SUICIDE); //Si uccide
}
void cat(String* input){
    StringArray* tokens = split(input, ' ');
    
    if(tokens->length == 1){
        println(STRING_IMMEDIATE, "File name is missing.\0");
        unloadArray(tokens);
        return;
    }

    //Aprire il file
    String* fullPath = getFullPath(tokens->data[1]); //Ottenere il percorso completo
    File_t* file = openFile(fullPath); 
    
    //Stamparne i contenuti se presenti o un messaggio di errore 
    switch((uint32_t)file->contents){
        case INVALID_PATH:
            println(STRING_IMMEDIATE, "Invalid path.\0");
            break;
        case NOT_FOUND:
            println(STRING_IMMEDIATE, "The specified file does not exist.\0");
            break;
        case NAME_TOO_LONG:
            println(STRING_IMMEDIATE, "File name is too long, max 8 characters.\0");
            break;
        case 0:
            println(STRING_IMMEDIATE, "The specified file is empty\0");
            break;
        default:
            println(STRING_IMMEDIATE, file->contents);
    }

    //Scaricare la RAM
    unloadString(fullPath);
    unloadArray(tokens);
    closeFile(file);
}
void writeCommand(String* input){
    StringArray* tokens = splitQuotes(input, ' ');
    if(tokens->length < 3){
        println(STRING_IMMEDIATE, "Incomplete command. Type in 'help' for more info.\0");
        unloadArray(tokens);
        return;
    }

    //Aprire il file al percorso specificato
    String* fullPath = getFullPath(tokens->data[1]);
    File_t* file = openFile(fullPath); //Ottenere il percorso completo
    
    //Gestione errori
    if(file == (File_t*)INVALID_PATH) println(STRING_IMMEDIATE, "Invalid path.\0");
    else if(file == (File_t*)NOT_FOUND) println(STRING_IMMEDIATE, "File does not exist.\0");
    else if(file == (File_t*)NAME_TOO_LONG) println(STRING_IMMEDIATE, "File name is too long. Max 8 chars + 3 chars.\0");
    else {
        //Scrivere il file
        char* ptr = strPointer(tokens->data[2]);
        writeToFile(file, ptr, strLength(tokens->data[2]));
        genericFree(ptr);
    }
    
    //Scaricare la RAM
    unloadArray(tokens);
    unloadString(fullPath);
    closeFile(file);
     
}
void cp(String* input){
    StringArray* tokens = split(input, ' ');
    if(tokens->length < 3){
        print(STRING_IMMEDIATE, "Incomplete command. Type in 'help' for more info.\0");
        unloadArray(tokens);
        return;
    }

    //Ottenere il percorso completo dei due file
    String* firstPath = getFullPath(tokens->data[1]);
    String* secondPath = getFullPath(tokens->data[2]);

    //Aprire il primo file
    File_t* first = openFile(firstPath);
    if(first == (File_t*)INVALID_PATH || first == (File_t*)NOT_FOUND || first == (File_t*)NAME_TOO_LONG){ //Gestione errori
        if(first == (File_t*)INVALID_PATH) println(STRING_IMMEDIATE, "Invalid first path.\0");
        else if(first == (File_t*)NOT_FOUND) println(STRING_IMMEDIATE, "First file does not exist.\0");
        else if(first == (File_t*)NAME_TOO_LONG) println(STRING_IMMEDIATE, "First file name is too long. Max 8 chars + 3 chars.\0");

        //Scaricare la RAM e liberare il terminale
        unloadString(firstPath);
        unloadString(secondPath);
        unloadArray(tokens);
        
        return;
    }


    //Provare a creare un secondo file o aprire il file se esiste già
    newFile(secondPath); //Se esiste non succede nulla
    File_t* second = openFile(secondPath);
    if(second == (File_t*)INVALID_PATH || second == (File_t*)NOT_FOUND || second == (File_t*)NAME_TOO_LONG){ //Gestione errori
        if(second == (File_t*)INVALID_PATH) println(STRING_IMMEDIATE, "Invalid second path.\0");
        else if(second == (File_t*)NOT_FOUND) println(STRING_IMMEDIATE, "First file does not exist.\0");
        else if(second == (File_t*)NAME_TOO_LONG) println(STRING_IMMEDIATE, "First file name is too long. Max 8 chars + 3 chars.\0");

        //Scaricare la RAM e liberare il terminale
        unloadString(firstPath);
        unloadString(secondPath);
        unloadArray(tokens);
        closeFile(first);
        
        return;
    }

    //Scrivere il secondo file con i contenuti del primo
    writeToFile(second, first->contents, first->size);
    
    //Chiudere tutto e liberare la RAM
    closeFile(first);
    closeFile(second);
    unloadString(firstPath);
    unloadString(secondPath);
    unloadArray(tokens);
     
}
void run(String* input){
    StringArray* tokens = split(input, ' ');
    if(tokens->length < 2){
        println(STRING_IMMEDIATE, "Incomplete command.\0");
        unloadArray(tokens);
        return;
    }

    //Aprire il file al percorso specificato
    String* fullPath = getFullPath(tokens->data[1]); //Ottenere il percorso completo
    File_t* binary = openFile(fullPath);
    
    //Gestione errori
    if(binary == (File_t*)INVALID_PATH) println(STRING_IMMEDIATE, "Invalid path.\0");
    else if(binary == (File_t*)NOT_FOUND) println(STRING_IMMEDIATE, "File not found.\0");
    else if(binary == (File_t*)NAME_TOO_LONG) println(STRING_IMMEDIATE, "File name is too long. Max 8 + 3 characters.\0");

    //Tratta il file come una funzione e saltaci
    ((void (*)())(binary->contents))(); 

    closeFile(binary);
    unloadString(fullPath);
    unloadArray(tokens);
}
void load(String* input){
    StringArray* tokens = split(input, ' ');

    //Ottenere il percorso del file
    String* fullPath = getFullPath(tokens->data[1]);

    //Creare un nuovo file o aprirne uno esistente a quel percorso
    int isFileCreated = !newFile(fullPath);
    File_t* file = openFile(fullPath);
    
    void* data = genericAlloc(32768); //A whole cluster
    int size;
    
    println(STRING_IMMEDIATE, "Listening to serial...\0");
    if((size = slupReceive(COM1, data, 32768)) < 1){
        genericFree(data);
        closeFile(file);
        if(isFileCreated) deleteFile(fullPath);
        if(size == 1) println(STRING_IMMEDIATE, "Timeout.\0");
        else println(STRING_IMMEDIATE, "Something went wrong...\0");
        return;
    }

    //Scrivere il file
    writeToFile(file, data, size);
    println(STRING_IMMEDIATE, "All done. Enjoy your new file.\0");

    //Scaricare la RAM e liberare il terminale
    closeFile(file);
    unloadString(fullPath);
    unloadArray(tokens);
    genericFree(data);
}  
void benchmark(String* input){
    println(STRING_IMMEDIATE, "Starting benchmark...\0");
    
    int start = now(), end = 0;;
    #define TESTS 1000000
    #define OVERHEAD 50
    int i = 0;
     
    String* s = new("hey.txt"); 
     
    for(; i < TESTS; ++i){
        if((end = now()) - start > 1000 + OVERHEAD) break;
        
        void* allocation = genericAlloc(sizeof(int));
        genericFree(allocation);

        //printStatic(INT, &i); //Prints how many tests have been completed
       
    }
    end = now();
    println(STRING_IMMEDIATE, "");
    unloadString(s);

    float delta = (float)(end - start) - OVERHEAD; //There is an overhead of approx. 1.85s if dinamic logging is enabled
    uint8_t isSeconds = delta > 1000;
     

    println(STRING_IMMEDIATE, "Benchmark completed.\0");
    
    if(i == TESTS && delta > 0){
        print(STRING_IMMEDIATE, "1M tests completed in: ");
        if(isSeconds){
            delta /= 1000;
            print(FLOAT, &delta);
            println(STRING_IMMEDIATE, "s.");
        }
        else{
            print(FLOAT, &delta);
            println(STRING_IMMEDIATE, "ms.");
        }
    }
    else if(delta > 0){
        print(STRING_IMMEDIATE, "Tests completed in 1s: ");
        println(INT, &i);
    }
    else println(STRING_IMMEDIATE, "Time for test is less than 1ms.");
     
    
}
void send(String* input){
    StringArray* tokens = splitQuotes(input, ' ');
    if(tokens->length < 2){
        println(STRING_IMMEDIATE, "Incomplete command. Type in 'help' to get a list of commands.\0");
        unloadArray(tokens);
        return;
    }

    slupSend(COM1, strPointer(tokens->data[1]), strLength(tokens->data[1])); //Send the string in quotes via serial 
    unloadArray(tokens);
}