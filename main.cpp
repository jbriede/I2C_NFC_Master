// header
#include "mbed.h"
#include "lib_TagType4.h"
#include "lib_NDEF.h"
#include "I2C.h"
#define BITSIZE 100
#define MAXMEMORY 8190 //pretty sure....

Serial pc(SERIAL_TX, SERIAL_RX);
 
 I2C isquaredc(D14,D15); // I2C pins are D14 and D15
 
DigitalOut myled(LED1);
//various LEDs on NFC Shield
DigitalOut nfcled1(D5);
DigitalOut nfcled2(D4);
DigitalOut nfcled3(D2);
//acknowledgement pins
DigitalOut indicatorM2S(D13);
DigitalIn indicatorS2M(D10);
DigitalIn NFC_Written(D12);

int main() {
    // Create NDEF buffer
    uint8_t NDEF_Buffer[BITSIZE], Zero_Buffer[MAXMEMORY];
    memset(NDEF_Buffer, 0, sizeof(NDEF_Buffer));
    memset(Zero_Buffer, 0, sizeof(Zero_Buffer)); // will not change
    
    // start up info... the following code is necessary to read the NFC shield
    sCCFileInfo CCFileStruct;
    sCCFileInfo *pCCFile;
    pCCFile = &CCFileStruct;

    uint16_t status = SUCCESS;
    uint8_t CCBuffer[15];
    
    status = TagT4Init( CCBuffer, sizeof(CCBuffer));
    
    if( status == SUCCESS)
    {   
        pCCFile->NumberCCByte = (uint16_t) ((CCBuffer[0x00]<<8) | CCBuffer[0x01]);
        pCCFile->Version = CCBuffer[0x02];
        pCCFile->MaxReadByte = (uint16_t) ((CCBuffer[0x03]<<8) | CCBuffer[0x04]);
        pCCFile->MaxWriteByte = (uint16_t) ((CCBuffer[0x05]<<8) | CCBuffer[0x06]);
        pCCFile->TField = CCBuffer[0x07];
        pCCFile->LField = CCBuffer[0x08];
        pCCFile->FileID = (uint16_t) ((CCBuffer[0x09]<<8) | CCBuffer[0x0A]);
        pCCFile->NDEFFileMaxSize = (uint16_t) ((CCBuffer[0x0B]<<8) | CCBuffer[0x0C]);
        pCCFile->ReadAccess = CCBuffer[0x0D];
        pCCFile->WriteAccess = CCBuffer[0x0E];  
    }
    
    // open NDEF session 
    OpenNDEFSession(pCCFile->FileID, ASK_FOR_SESSION);
    
    // write all zeros to NDEF memory
    NDEF_WriteNDEF(Zero_Buffer);
    
    // close NDEF session
    CloseNDEFSession(pCCFile->FileID);
    
    // creat while loop
    
    
    indicatorM2S = 0;
    
    pc.printf("init \n");
    while(1) {
        if (NFC_Written==0) {
            pc.printf("session\n");
            wait(4);
            //open NDEF session
            OpenNDEFSession(pCCFile->FileID, ASK_FOR_SESSION);
            
            // read NDEF
            NDEF_ReadNDEF(NDEF_Buffer);
            
            // close NDEF session
            CloseNDEFSession(pCCFile->FileID);
                 
            
            // if NDEF is not all 0s, take the store memory in variable
            if ((memcmp(Zero_Buffer, NDEF_Buffer, sizeof(NDEF_Buffer)))!=0){ 
        
                pc.printf("Tag Identified...\n");
                    
                indicatorM2S = 1; // Master indicates to slave that there is a message waiting
                
                pc.printf("M2S set HIGH...\n");
                
                char send[BITSIZE]; // converting to char in order to send via i2c
                memcpy(send, NDEF_Buffer, BITSIZE);
                
                 while(1){ // waiting for slave to indicate it is ready to receive message
                     if (indicatorS2M.read() == 1)
                     
                    break;
                // probably should add some type of time out function
            }
                pc.printf("S2M awknowledgement recieved...\n");
                //Send data via I2C
                isquaredc.write(0x10, send, BITSIZE, 1);
                pc.printf("I2C.write...\n");
               // turn off M2S signal
               indicatorM2S =0;
                
                // write all 0s to the NFC memory
                OpenNDEFSession(pCCFile->FileID, ASK_FOR_SESSION);
                NDEF_WriteNDEF(Zero_Buffer); 
                CloseNDEFSession(pCCFile->FileID);
                memset(NDEF_Buffer, 0, sizeof(NDEF_Buffer));
            }
        }
    }
}