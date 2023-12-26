#include <SoftwareSerial.h>
#include <fpm.h>

#include <Ethernet.h>
#include <EthernetUdp.h>

/* Send a fingerprint image to a UDP server over an Ethernet connection.
 * The image is transferred in chunks -- the size of the chunk in each UDP packet 
 * is no more than the current packet-length of the sensor.
 * 
 * Since writing to network sockets can block and take some time, you may need to lower 
 * the sensor baud rate to perhaps 19200, in order to avoid losing image data over Serial.
 * (But this may be unnecessary depending on your network transport and MCU.)
 */

/*  pin #2 is Arduino RX <==> Sensor TX
 *  pin #3 is Arduino TX <==> Sensor RX
 */
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPMSystemParams params;

/* Ethernet setup: provide your MAC and IP details here */
static const byte mac[] = {0xDE, 0xAD, 0xDE, 0xEF, 0xFF, 0xEF};

IPAddress udpServerIp(192, 168, 3, 156);
unsigned int udpServerPort = 9999;

EthernetUDP udpClient;
unsigned int localPort = 8888;

/* for convenience */
#define PRINTF_BUF_SZ   60
char printfBuf[PRINTF_BUF_SZ];

void setup()
{
    Serial.begin(57600);
    fserial.begin(57600);
    
    Serial.println("IMAGE-TO-UDP example");

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
    
    if (Ethernet.begin(mac) == 0)
    {
        Serial.println("Ethernet setup failed.");
        while (1) yield();
    }
    
    udpClient.begin(localPort);
}

void loop()
{
    imageToUdp();

    while (1) yield();
}

uint32_t imageToUdp(void)
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
    uint16_t readLen = 0;
    
    /* Now, the sensor will send us the image from its image buffer, one packet at a time. */
    bool readComplete = false;

    while (!readComplete) 
    {
        /* Start composing a packet to the remote server */
        udpClient.beginPacket(udpServerIp, udpServerPort);
        
        bool ret = finger.readDataPacket(NULL, &udpClient, &readLen, &readComplete);
        
        if (!ret)
        {
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("readDataPacket(): failed after reading %u bytes"), totalRead);
            Serial.println(printfBuf);
            return 0;
        }
        
        /* Complete the packet and send it */
        if (!udpClient.endPacket())
        {
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("imageToUdp(): failed to send packet, count = %u bytes"), totalRead);
            Serial.println(printfBuf);
            return 0;
        }
        
        totalRead += readLen;
        
        yield();
    }

    Serial.println();
    Serial.print(totalRead); Serial.println(" bytes transferred.");
    return totalRead;
}
