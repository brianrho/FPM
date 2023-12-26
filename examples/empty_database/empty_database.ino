#include <SoftwareSerial.h>
#include <fpm.h>

/* Empty fingerprint database */

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
    
    Serial.println("EMPTY DATABASE example");

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
    Serial.println("\r\nSend any character to empty the database...");
    while (Serial.available() == 0) yield();
    
    emptyDatabase();
    
    while (Serial.read() != -1);
}

bool emptyDatabase(void) 
{
    FPMStatus status = finger.emptyDatabase();
    
    switch (status) 
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Database empty.");
            Serial.println(printfBuf);
            break;
            
        case FPMStatus::DBCLEARFAIL:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Could not clear sensor database.");
            Serial.println(printfBuf);
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "emptyDatabase(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    return true;
}
