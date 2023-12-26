#include <SoftwareSerial.h>
#include <fpm.h>

/* Read a fingerprint image from the sensor into a buffer.
 * 
 * NB: This example will not compile unless you have enough RAM (> 37 kB) to hold the entire image.
 */

/*  pin #2 is Arduino RX <==> Sensor TX
 *  pin #3 is Arduino TX <==> Sensor RX
 */
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPMSystemParams params;

/* for convenience */
#define PRINTF_BUF_SZ   60
char printfBuf[PRINTF_BUF_SZ];

void setup()
{
    Serial.begin(57600);
    fserial.begin(57600);
    
    Serial.println("IMAGE-TO-BUFFER example");

    if (finger.begin()) {
        finger.readParams(&params);
        Serial.println("Found fingerprint sensor!");
        Serial.print("Capacity: "); Serial.println(params.capacity);
        Serial.print("Packet length: "); Serial.println(FPM::packetLengths[static_cast<uint8_t>(params.packetLen)]);
    } 
    else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }    
}

void loop()
{
    imageToBuffer();

    while (1) yield();
}

#define IMAGE_SZ	36864UL
uint8_t imageBuffer[IMAGE_SZ];

uint32_t imageToBuffer(void)
{
    FPMStatus status;
    
    /* Take a snapshot of the finger */
    Serial.println("\r\nPlace a finger.");
    
    do {
        status = finger.getImage();
        
        switch (status) 
        {
            case FPMStatus::OK:
                Serial.println("Image taken.");
                break;
                
            case FPMStatus::NOFINGER:
                Serial.println(".");
                break;
                
            default:
                /* allow retries even when an error happens */
                snprintf(printfBuf, PRINTF_BUF_SZ, "getImage(): error 0x%X", static_cast<uint16_t>(status));
                Serial.println(printfBuf);
                break;
        }
        
        yield();
    }
    while (status != FPMStatus::OK);
    
    /* Initiate the image transfer */
    status = finger.downloadImage();
    
    switch (status) 
    {
        case FPMStatus::OK:
            Serial.println("Starting image stream...");
            break;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "downloadImage(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return 0;
    }
    
    uint32_t totalRead = 0;
    uint16_t readLen = IMAGE_SZ;
    
    /* Now, the sensor will send us the image from its image buffer, one packet at a time */
    bool readComplete = false;

    while (!readComplete) 
    {
        bool ret = finger.readDataPacket(imageBuffer + totalRead, NULL, &readLen, &readComplete);
        
        if (!ret)
        {
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("readDataPacket(): failed after reading %u bytes"), totalRead);
            Serial.println(printfBuf);
            return 0;
        }
        
        totalRead += readLen;
        readLen = IMAGE_SZ - totalRead;
        
        yield();
    }

    Serial.println();
    Serial.print(totalRead); Serial.println(" bytes transferred.");
    return totalRead;
}

