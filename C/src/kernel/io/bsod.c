#include "kernel/bsod.h"
#include "kernel/terminal.h"
#include "kernel/sysio.h"

int panicCode = 0;

static inline void printConcat(const char* name, uint32_t* value){
    print(STRING_IMMEDIATE, name);
    println(INT, value);
}
void panic(int code){
    panicCode = code;
    asm volatile("int $0x31");
}
void launchBSOD(RegisterFrame frame, int code){
    setTerminalColor(0x1F);
    clearTerminal();
 

    char* codes[] = {
        "UNKNOWN ERROR\0",
        "MALLOC_FAILED\0",
        "MMU_CORRUPTION\0",
        "No error, bro killed himself -_-\0",
        "FILESYSTEM_CORRUPTION\0",
        "DISK_ERROR\0",
        "MMU_OVERWRITE\0",
        "SERIAL_ERROR\0"
    };
    char* desc[] = {
        "An unkown interrupt triggered by the CPU\0",
        "The MMU couldn't allocate RAM. Most likely system memory ran out.\0",
        "The MMU got corrupted. Its internal pointer to RAM data got overwritten most\n likely by a bug\0",
        "Maybe you decided that you couldn't take it anymore and ended your suffering.\0",
        "The filesystem got corrupted. Check the FAT and critical structures.\0",
        "Couldn't read from disk. It indicates faulty sectors. If this happens often,\nconsider changing your HDD.\0",
        "The MMU got corrupted. More specifically, while the bitmap that controls RAM\n is intact, data about memory capacity got overwritten, most likely by a bug.\0",
        "The serial port isn't working.\0"
    };

   

    println(STRING_IMMEDIATE, "UH OH. YOU JUST DIED :(\0");
    println(STRING_IMMEDIATE, "ERROR CODE: \0");
    println(STRING_IMMEDIATE, codes[code]);
    
    println(STRING_IMMEDIATE, desc[code]);
     


    if(code != SUICIDE){
        
        if(!frame.isrNumber) println(STRING_IMMEDIATE, "Critical bug/software error/non-essential component fault, not a CPU fault.\nTriggered on purpose bymyself when I noticed something was off.\n\0");
        else{
            printConcat("ISR NUMBER: ", &frame.isrNumber);
        }
        
        //Stampa i registri
        printConcat("EAX: ", &frame.eax);
        printConcat("EBX: ", &frame.ebx);         
        printConcat("ECX: ", &frame.ecx);         
        printConcat("EDX: ", &frame.edx);        
        printConcat("ESI: ", &frame.esi);       
        printConcat("EDI: ", &frame.edi);        
        printConcat("ESP: ", &frame.esp);      
        printConcat("EBP: ", &frame.ebp);       
        printConcat("EIP: ", &frame.eip);
        if(!frame.isrNumber) println(STRING_IMMEDIATE, " <-- Where I noticed, not where it happened. To get precise info,\n hop on GDB and use watchpoints\0");
         

        println(STRING_IMMEDIATE, "\nReboot your PC. Take action (debugging, changing faulty components) to get a\nbetter user experience.\0");
    }
    else{
        println(STRING_IMMEDIATE, "You don't need register info, you just committed suicide.\0");
        
    }
    
     

    asm volatile("cli");
    asm volatile("hlt");

}