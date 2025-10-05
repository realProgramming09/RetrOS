#include "kernel/syscallHandler.h"
#include "user/kernelAPI.h"
#include "kernel/memory.h"
#include "kernel/files.h"
#include "kernel/print.h"
#include "kernel/serial.h"

void handleSyscall(uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx){
    if(!eax || !ebx || !ecx || !edx) return;

    switch(*eax){
        case SYSCALL_MALLOC : {
            *eax = (uint32_t)genericAlloc((size_t)*ebx);
            break;
        }
        case SYSCALL_FREE : {
            genericFree((void*)*ebx);
            break;
        }
        case SYSCALL_REALLOC : {
            *eax = (uint32_t)genericRealloc((void*)*ebx, (size_t)*ecx);
            break;
        }
        case SYSCALL_PRINT : {
            print((DataType)*ecx, (void*)*ebx);
            *eax = 0;
            break;
        }
        case SYSCALL_GETCHAR : {
            *eax = (uint32_t)getChar();
            break;
        }
        case SYSCALL_SCAN : {
            scanTerminal((uint8_t*)*ebx, (size_t)*ecx);
            break;
        }
        case SYSCALL_OPEN : {
            String* path = new((char*)*ecx);
            
            //Aprire il file al percorso richiesto
            newFile(path);
            File_t* f = openFile(path);
            
            //Gestione errori
            if(f == (File_t*)INVALID_PATH || f == (File_t*)NOT_FOUND || f == (File_t*)NAME_TOO_LONG){
                *eax = (uint32_t)f;
                unloadString(path);
                return;
            }

            //Mettere nei registri i dati del file_t
            *eax = (uint32_t)f->contents;
            **(uint32_t**)ebx = (uint32_t)f->size;
            
            char* name = strPointer(path);
            strncpy(name, (char*)*ecx, strlen(name));
            genericFree(name);

            unloadString(f->name);
            unloadString(f->path);
            genericFree(f);
            unloadString(path);
            break;
        }
        case SYSCALL_WRITE : {
            String* path = new((char*)*ebx);
            File_t* f = openFile(path);
            writeToFile(f, (void*)*ecx, (size_t)*edx);
            closeFile(f);
            unloadString(path);
            break;
        }
        case SYSCALL_EXIT : {
            //Boh...
            break;
        }
        case SYSCALL_WRITECOM : {
            sendCOM(*ebx, (uint8_t*)*ecx, (size_t)*edx);
            break;
        }
    }
}