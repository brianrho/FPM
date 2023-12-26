#include <SoftwareSerial.h>
#include <fpm.h>

/* Get number of stored templates/prints in database */

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
    
    Serial.println("COUNT PRINTS example");

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
    Serial.println("\r\nSend any character to get the number of stored templates...");
    while (Serial.available() == 0) yield();
    
    getTemplateCount();
    
    while (Serial.read() != -1);
}

bool getTemplateCount(void) 
{
    uint16_t count;
    
    FPMStatus status = finger.getTemplateCount(&count);
    
    switch (status) 
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "%u prints in sensor database", count);
            Serial.println(printfBuf);
            break;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "getTemplateCount(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    return true;
}
