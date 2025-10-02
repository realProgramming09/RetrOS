#include "kernel/syscallHandler.h"
#include "user/kernelAPI.h"
#include "kernel/mmu.h"
#include "kernel/files.h"
#include "kernel/sysio.h"
#include "kernel/serial.h"

void handleSyscall(uint32_t* number, uint32_t* param1, uint32_t* param2, uint32_t* param3){
    if(!number || !param1 || !param2 || !param3) return;

    switch(*number){
        case SYSCALL_MALLOC : {
            *number = (uint32_t)genericAlloc((size_t)*param1);
            break;
        }
        case SYSCALL_FREE : {
            genericFree((void*)*param1);
            break;
        }
        case SYSCALL_REALLOC : {
            *number = (uint32_t)genericRealloc((void*)*param1, (size_t)*param2);
            break;
        }
        case SYSCALL_PRINT : {
            print((DataType)*param2, (void*)*param1);
            *number = 0;
            break;
        }
        case SYSCALL_GETCHAR : {
            *number = (uint32_t)getChar();
            break;
        }
        case SYSCALL_SCAN : {
            scanTerminal((uint8_t*)*param1, (size_t)*param2);
            break;
        }
        case SYSCALL_OPEN : {
            String* path = new((char*)*param2);
            
            //Aprire il file al percorso richiesto
            newFile(path);
            File_t* f = openFile(path);
            
            //Gestione errori
            if(f == (File_t*)INVALID_PATH || f == (File_t*)NOT_FOUND || f == (File_t*)NAME_TOO_LONG){
                *number = (uint32_t)f;
                unloadString(path);
                return;
            }

            //Mettere nei registri i dati del file_t
            *number = (uint32_t)f->contents;
            **(uint32_t**)param1 = (uint32_t)f->size;
            
            char* name = strPointer(path);
            strncpy(name, (char*)*param2, strlen(name));
            genericFree(name);

            unloadString(f->name);
            unloadString(f->path);
            genericFree(f);
            unloadString(path);
            break;
        }
        case SYSCALL_WRITE : {
            String* path = new((char*)*param1);
            File_t* f = openFile(path);
            writeToFile(f, (void*)*param2, (size_t)*param3);
            closeFile(f);
            unloadString(path);
            break;
        }
        case SYSCALL_EXIT : {
            //Boh...
            break;
        }
        case SYSCALL_WRITECOM : {
            sendCOM(*param1, (uint8_t*)*param2, (size_t)*param3);
            break;
        }
    }
}