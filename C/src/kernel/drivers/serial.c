#include "kernel/serial.h"
#include "kernel/memory.h"
#include "kernel/sysio.h"



//Macro to wait for hardware
#define sendWait while((recByte(ID + LINE_CONTROL_OFFSET) & 0x01) == 0)
#define recWait while((recByte(ID + LINE_STATUS_OFFSET) & 0x20) == 0 || head == tail)

#define TIMEOUT 100 //Timeout after a connection is established


uint8_t serialBuffer[BUFFER_SIZE];
uint32_t head = 0, tail = 0, timePassed = 0, isListening = 0;



static inline int abs(int x){
    return x > 0 ? x : -x;
}
int serialInit(uint32_t ID, uint16_t divisor){
    if(!divisor) return 1; //You can't divide by 0
    
    //Set the DLAB and send the divisor a byte at a time
    sendByte(ID + LINE_CONTROL_OFFSET, 0x80);
    sendByte(ID + DIVISOR_LOW_OFFSET, divisor & 0xFF);
    sendByte(ID + DIVISOR_HIGH_OFFSET, (divisor >> 8) & 0xFF);
    sendByte(ID + LINE_CONTROL_OFFSET, 0); //Clean the DLAB!
    
    //Set line mode: 8N1 (8bytes, 1 stop bit, no parit7)
    sendByte(ID + LINE_CONTROL_OFFSET, 0x03);

    //Enable interrupts
    sendByte(ID + FIFO_CONTROL_OFFSET, 0x87); //Enable FIFO buffer
    sendByte(ID + MODEM_CONTROL_OFFSET, 0x1B); //Loopback mode
    
    //Small test: if what ew receive isn't what we sent, the port is faulty
    sendByte(ID+WRITEBUFFER_OFFSET, 0xDD);
    if(recByte(ID + READBUFFER_OFFSET) != 0xDD) return 1;

    sendByte(ID + MODEM_CONTROL_OFFSET, 0x0B); //Standard mode
    sendByte(ID + INTERRUPT_ENABLE_OFFSET, 0x01);

    return 0;
     
}
void sendCOM(uint32_t ID, uint8_t* data, size_t size){
    if(!data || size < 1) return; //Error checking

    for(int i = 0; i < size; i++){
        sendWait; //Wait before sending data
        sendByte(ID + WRITEBUFFER_OFFSET, data[i]);
    }
}
void recCOM(uint32_t ID, uint8_t* data, size_t size){
    if(!data || size < 1) return; //Error checking
    recWait;
    asm volatile("cli"); //We don't wanna be disturbed while working with critical data

    
    uint32_t length = tail-head; //Queue length

    //Loop the circular buffer and copy data, popping as we go
    for(int i = 0; i < length && i < size; i++){
        data[i] = serialBuffer[(head++) % BUFFER_SIZE];
    }


    asm volatile("sti"); //Re-enable interrupts


   
}
int listenCOM(uint32_t timeout){
    head = 0, tail = 0, timePassed = 0; //Reset the circular buffer and the timeout counter

    //Listern until timeout
    isListening = 1; 
    while(timePassed < timeout){
        if(tail > 0) timeout = TIMEOUT; //If we received something lower the timeout to 100ms
    }
    isListening = 0;

    return !tail; //0 if all right, 1 otherwise
}