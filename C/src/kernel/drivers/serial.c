#include "kernel/serial.h"
#include "kernel/mmu.h"
#include "kernel/sysio.h"



//Macro per aspettare l'hardware
#define sendWait while((recByte(ID + LINE_CONTROL_OFFSET) & 0x01) == 0)
#define recWait while((recByte(ID + LINE_CONTROL_OFFSET) & 0x20) == 0 || head == tail)

#define TIMEOUT 100 //Timeout per la connessione


uint8_t serialBuffer[BUFFER_SIZE];
uint32_t head = 0, tail = 0, timePassed = 0, isListening = 0;



static inline int abs(int x){
    return x > 0 ? x : -x;
}
int serialInit(uint32_t ID, uint16_t divisor){
    if(!divisor) return 1; //Abbiamo solo due porte e non puoi dividere per 0
    
    //Impostare il DLAB e mandare il divisore un byte per volta
    sendByte(ID + LINE_CONTROL_OFFSET, 0x80);
    sendByte(ID + DIVISOR_LOW_OFFSET, divisor & 0xFF);
    sendByte(ID + DIVISOR_HIGH_OFFSET, (divisor >> 8) & 0xFF);
    sendByte(ID + LINE_CONTROL_OFFSET, 0); //Pulire il DLAB!
    
    //Impostare la modalità di trasferimento: 8byte alla volta, no parity, 1bit di intermezzo 
    sendByte(ID + LINE_CONTROL_OFFSET, 0x03);

    //Attivare le interrupt
    sendByte(ID + FIFO_CONTROL_OFFSET, 0x87); //Abilitare il buffer FIFO
    
    sendByte(ID + MODEM_CONTROL_OFFSET, 0x1B); //Loopback mode
    
    //Piccolo test
    sendByte(ID+WRITEBUFFER_OFFSET, 0xDD);
    if(recByte(ID + READBUFFER_OFFSET) != 0xDD) return 1;

    sendByte(ID + MODEM_CONTROL_OFFSET, 0x0B); //Standard mode
    sendByte(ID + INTERRUPT_ENABLE_OFFSET, 0x01);

    return 0;
     
}
void sendCOM(uint32_t ID, uint8_t* data, size_t size){
    if(!data || size < 1) return; //Gestione errori

    for(int i = 0; i < size; i++){
        sendWait; //Aspettare di poter mandare qualcosa
        sendByte(ID + WRITEBUFFER_OFFSET, data[i]);
    }
}
void recCOM(uint32_t ID, uint8_t* data, size_t size){
    if(!data || size < 1) return; //Gestione errori
    recWait;
    asm volatile("cli"); //Non vogliamo essere disturbati a manipolare un buffer critico

    
    uint32_t length = tail-head; //Lunghezza della fila

    //Scorrere il buffer circolare e copiare i byte necessari
    for(int i = 0; i < length && i < size; i++){
        data[i] = serialBuffer[(head++) % BUFFER_SIZE];
    }


    asm volatile("sti");


   
}
int listenCOM(uint32_t timeout){
    head = 0, tail = 0, timePassed = 0; //Reimpostare il buffer circolare e il timeout counter

    //Ascoltare fino al timeout
    isListening = 1; 
    while(timePassed < timeout){
        if(tail > 0) timeout = TIMEOUT; //Se abbiamo ricevuto abbassare il timeout a 100ms
    }
    isListening = 0;

    return !tail; //0 se successo, 1 se fallito
}