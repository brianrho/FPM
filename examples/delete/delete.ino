#include <SoftwareSerial.h>
#include <fpm.h>

/* Delete fingerprints */

/*  pin #2 is Arduino RX <==> Sensor TX
 *  pin #3 is Arduino TX <==> Sensor RX
 */
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPMSystemParams params;

/* for convenience */
#define PRINTF_BUF_SZ   40
char printfBuf[PRINTF_BUF_SZ];

void setup()
{
    Serial.begin(57600);
    fserial.begin(57600);
    
    Serial.println("DELETE example");

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

void loop()                     // run over and over again
{
    Serial.println("\r\nEnter the template ID # you want to delete...");
    
    uint16_t fid = 0;
    
    while (true) 
    {
        while (! Serial.available()) yield();
        
        char c = Serial.read();
        if (! isdigit(c)) break;
        
        fid *= 10;
        fid += c - '0';
        
        yield();
    }
    
    Serial.print("Deleting ID #");
    Serial.println(fid);

    deleteFingerprint(fid);
    
    while (Serial.read() != -1) yield();
}

bool deleteFingerprint(int fid) 
{
    FPMStatus status = finger.deleteTemplate(fid);
    
    switch (status) 
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Deleted ID #%u.", fid);
            Serial.println(printfBuf);
            break;
            
        case FPMStatus::DELETEFAIL:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Failed to delete ID #%u.", fid);
            Serial.println(printfBuf);
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "deleteTemplate(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    return true;
}
