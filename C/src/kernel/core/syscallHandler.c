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
            *number = genericAlloc(*param1);
            break;
        }
        case SYSCALL_FREE : {
            genericFree(*param1);
            break;
        }
        case SYSCALL_REALLOC : {
            *number = genericRealloc(*param1, *param2);
            break;
        }
        case SYSCALL_PRINT : {
            print(*param2, *param1);
            *number = 0;
            break;
        }
        case SYSCALL_GETCHAR : {
            *number = getChar();
            break;
        }
        case SYSCALL_SCAN : {
            scanTerminal(*param1, *param2);
            break;
        }
        case SYSCALL_OPEN : {
            String* path = new(*param2);
            
            //Aprire il file al percorso richiesto
            newFile(path);
            File_t* f = openFile(path);
            
            //Gestione errori
            if(f == INVALID_PATH || f == NOT_FOUND || f == NAME_TOO_LONG){
                *number = f;
                unloadString(path);
                return;
            }

            //Mettere nei registri i dati del file_t
            *number = f->contents;
            **(uint32_t**)param1 = f->size;
            
            char* name = strPointer(path);
            strncpy(name, *param2, strlen(name));
            genericFree(name);

            unloadString(f->name);
            unloadString(f->path);
            genericFree(f);
            unloadString(path);
            break;
        }
        case SYSCALL_WRITE : {
            String* path = new(*param1);
            File_t* f = openFile(path);
            writeToFile(f, *param2, *param3);
            closeFile(f);
            unloadString(path);
            break;
        }
        case SYSCALL_EXIT : {
            //Boh...
            break;
        }
        case SYSCALL_WRITECOM : {
            sendCOM(*param1, *param2, *param3);
            break;
        }
    }
}