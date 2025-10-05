#include "kernel/slup.h"

#define LEAD_SIGNATURE 0xD72A90B1

#define CONNECTION_TIMEOUT 5000

int slupReceive(uint32_t ID, void* data, size_t size){
    if(!data || size < 1) return 1;

    int errorCode = listenCOM(CONNECTION_TIMEOUT); //Wait for everything to be sent into the buffer
    if(errorCode) return 1;

    int leadSignature, receivedSize; //Declare all the variables we need to fill

    recCOM(ID, (void*)&leadSignature, sizeof(leadSignature)); //Read the signature
    if(leadSignature != LEAD_SIGNATURE) return 1;

    recCOM(ID, (void*)&receivedSize, sizeof(receivedSize)); //Read how much data there is
    if(receivedSize == 0) return 1;

    recCOM(ID, data, size < receivedSize ? size : receivedSize); //Read the data

     
    return 0;

}
void slupSend(uint32_t ID, void* data, size_t size){
    if(!data || size < 1) return;

    int leadSignature = LEAD_SIGNATURE;

    //Send: signature, size, data
    sendCOM(ID, (void*)&leadSignature, sizeof(leadSignature));
    sendCOM(ID, (void*)&size, sizeof(size));
    sendCOM(ID, data, size);


}