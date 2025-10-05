#include "sys.h"
#include "serial.h"

void slupSend(uint32_t ID, void* data, size_t size); //Send data on serial via the SLUP protocol
int slupReceive(uint32_t ID, void* data, size_t size); //Receive data into a buffer via the SLUP protocol. Returns the received data's size in bytes