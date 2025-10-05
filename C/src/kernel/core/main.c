#include "kernel/print.h"
#include "kernel/terminal.h"
#include "kernel/sys.h"
#include "kernel/memory.h"
#include "kernel/disk.h"
#include "kernel/files.h"
#include "kernel/str.h"
#include "kernel/fixedMath.h"
#include "kernel/bsod.h"
#include "kernel/serial.h"
#include "kernel/shell.h"
#include "kernel/keyboard.h"

void main(){
    //Inizializzare le risorse
    loadMMU((MMU_t*)0x5000); 
    
    initKeyboard();
    terminalInit();
    rootDirInit();
    initShell();
    if(serialInit(COM1, 6) != 0){
        panic(SERIAL_ERROR);
    };

    //Scrivere un messaggio di benvenuto
    setTerminalColor(LIGHT_BLUE);
    print(STRING_IMMEDIATE, "WELCOME TO RetrOS v0.0.1\nSTART DOING STUFF OR TYPE IN 'help' TO GET A LIST OF COMMANDS\n\0");
    setTerminalColor(WHITE);
     
    //Lanciare la shell
    launchShell();

    for(;;){
        
         asm volatile ("hlt");
        
    }
    
}
