
#include "kernel/sys.h"
#include "kernel/terminal.h"
#include "kernel/sysio.h"
#include "kernel/memory.h"
#include "kernel/bsod.h"
#include "kernel/serial.h"
#include "kernel/syscallHandler.h"

//Macro per definire i codici interrupt
#define DIVIDE_BY_ZERO 0
#define PIT_FIRE 0x20
#define KEYBOARD_PRESS 0x21
#define COM1_RECEIVED 0x24
#define COM2_RECEIVED 0x23
#define SYSCALL 0x30
#define KERNEL_PANIC 0x31

extern void isr0(); //Eh, lo so, ma servono tutte che vanno mappate e ain't no way che lo faccio in asm la logica
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr30();
extern void isr31();
extern void isr32();
extern void isr33();
extern void isr34();
extern void isr35();
extern void isr36();
extern void isr37();
extern void isr38();
extern void isr39();
extern void isr40();
extern void isr41();
extern void isr42();
extern void isr43();
extern void isr44();
extern void isr45();
extern void isr46();
extern void isr47();
extern void isr48();
extern void isr49();
extern void isr50();
void isrNothing(){}; //ISR for non-mapped interrupts

;
uint32_t nullValue = 0;
uint8_t currentKeyPressed = 0;
 

void (*isrFunctions[])() = {
    isr0, isr1, isr2, isr3, isr4, isr5, isr6,
    isr7, isr8, isr9, isr10, isr11, isr12,
    isr13, isr14, isrNothing, isr16, isr17, isr18, isr19,
    isr20, isrNothing, isrNothing, isrNothing, isrNothing, isrNothing, isrNothing, isrNothing, isrNothing, isrNothing, isr30, isr31, isr32, isr33, isr34, 
    isr35, isr36, isr37, isr38, isr39, isr40,
    isr41, isr42, isr43, isr44, isr45, isr46, 
    isr47, isr48,  isr49,
     
};

struct IDT{
    uint16_t limit; //Size
    uint32_t base; //Starting address
}__attribute__((packed));

struct IDTEntry{
    uint16_t offsetLow; //First 16bits of the ISR's address
    uint16_t segment; //GDT code segment
    uint8_t reserved; //0
    uint8_t flags; //Flags
    uint16_t offsetHigh; //Last 16 bits of the ISR's address
}__attribute__((packed));

struct ISRFrame{
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; //Registers
    uint32_t  errorCode, isrNumber; //ISR number and error code
    uint32_t eip, cs, eflags; //Values pushed automatically onto the stack
}__attribute__((packed));

struct IDTEntry table[50];

extern void loadIDT(struct IDT* idt);
 
RegisterFrame createFrame(struct ISRFrame* f){
    RegisterFrame frame = {
        .eax = f->eax,
        .ebx = f->ebx,
        .ecx = f->ecx,
        .edx = f->edx,
        .edi = f->edi,
        .esi = f->esi,
        .esp = f->esp,
        .eip = f->eip,
        .isrNumber = f->isrNumber
    };
    return frame;
}
void fillEntry(struct IDTEntry* entry, void (*isr)(), uint8_t flags){
    //Set the entry
    uint32_t addr = (uint32_t)isr;
    entry->offsetLow = addr & 0xFFFF; //First 16bits of the ISR's address
    entry->segment = 0x08; //GDT code segment
    entry->reserved = 0;
    entry->flags = flags; //Flags
    entry->offsetHigh = (addr >> 16) & 0xFFFF; //Last 16 bits of the ISR's address
}
void sendEOI(int isrNumber){
    if(isrNumber >= 32 && isrNumber <= 48){ //Send the EOI only for IRQs
        if(isrNumber >= 40) sendByte(0xA0, 0x20);
        sendByte(0x20, 0x20);
    }
}



void idtInit(){

    //Fill the IDT
    for(int i = 0; i < 48; i++){
        fillEntry(&table[i], isrFunctions[i], 0x8E);
    }
    fillEntry(&table[48], isrFunctions[48], 0xEE); //Syscall
    fillEntry(&table[49], isrFunctions[49], 0xEE); //BSOD-specific int

    struct IDT idt = {
        .limit = (uint16_t)sizeof(table)-1,
        .base = (uint32_t)&table[0]
    };

    loadIDT((struct IDT*)&idt);

    //Inizialize the PICs
    sendByte(0x20, 0x11);
    sendByte(0xA0, 0x11);

    //Set them up
    sendByte(0x21, 0x20);
    sendByte(0xA1, 0x28);

    //Connect them up
    sendByte(0x21, 4);
    sendByte(0xA1, 2);

    //Set 32bit mode
    sendByte(0x21, 1);
    sendByte(0xA1, 1);

    //Set which IRQs to activate: PIT, keyboard, COM1/2
    sendByte(0x21, 0xE0);
    sendByte(0xA1, 0xFF);

    //Setup the PIT every 1ms
    uint16_t res = 1193;
    sendByte(0x43, 0x36);
    sendByte(0x40, res & 0xF);
    sendByte(0x40, res >> 8);

    asm volatile ("sti");
 
}
void isrHandler(struct ISRFrame* frame){
    switch(frame->isrNumber){
         
        case PIT_FIRE:{
            if(isListening) timePassed++; //Increment serial timeout counter
            break;
        }
        case KEYBOARD_PRESS: {
            currentKeyPressed = recByte(0x60);
            break;
        }
        case SYSCALL: {
            sendEOI(frame->isrNumber); //Syscall are gonna use IRQs, re-enable them
            asm volatile("sti"); 
            handleSyscall(&frame->eax, &frame->ebx, &frame->ecx, &frame->edx);
            break;
        }
        case COM1_RECEIVED: {
            while((recByte(COM1+LINE_STATUS_OFFSET) & 1) != 0) serialBuffer[(tail++) % BUFFER_SIZE] = recByte(COM1); //Read from COM1 and append onto the buffer
            timePassed = 0;
            
            break;
        }
        case COM2_RECEIVED: {
            while((recByte(COM2+LINE_STATUS_OFFSET) & 1) != 0) serialBuffer[(tail++) % BUFFER_SIZE] = recByte(COM1); //Read from COM2 and append onto the buffer
            timePassed = 0;
            break;
        }  
        case KERNEL_PANIC : {
            launchBSOD(createFrame(frame), panicCode);
            break;
        }
        default: {
            launchBSOD(createFrame(frame), 0);
            break;
        }
    }
    

    //Send an EOI to the PIC: we're ready for another IRQ
    sendEOI(frame->isrNumber);

    
}
 