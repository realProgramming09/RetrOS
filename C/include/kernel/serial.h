#include "sys.h"

//Macro per definire valori comuni
#define MAX_BAUD_RATE 115200 
#define COM1 0x3F8
#define COM2 0x2F8

//Macro per le porte
#define READBUFFER_OFFSET 0
#define WRITEBUFFER_OFFSET 0 
#define DIVISOR_LOW_OFFSET 0
#define DIVISOR_HIGH_OFFSET 1
#define INTERRUPT_ENABLE_OFFSET 1
#define INTERRUPT_ID_OFFSET 2
#define FIFO_CONTROL_OFFSET 2
#define LINE_CONTROL_OFFSET 3
#define MODEM_CONTROL_OFFSET 4
#define LINE_STATUS_OFFSET 5
#define MODEM_STATUS_OFFSET 6
#define SCRATCH_OFFSET 7

//Buffer per l'input da seriale
#define BUFFER_SIZE 32768
extern uint8_t serialBuffer[BUFFER_SIZE];
extern uint32_t head, tail, timePassed, isListening; 

int serialInit(uint32_t ID, uint16_t divisor); //Imposta un divisore per il baud rate
void sendCOM(uint32_t ID, uint8_t* data, size_t size); //Manda dati via seriale
int listenCOM(uint32_t timeout); //Ascolta per una connessione via seriale
void recCOM(uint32_t ID, uint8_t* data, size_t size); //Legge dal buffer circolare della seriale