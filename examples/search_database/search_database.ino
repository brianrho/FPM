#include <SoftwareSerial.h>
#include <fpm.h>

/* Search database for fingerprints */

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
    
    Serial.println("1:N MATCH example");

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
    Serial.println("\r\nSend any character to search for a print...");
    while (Serial.available() == 0) yield();
    
    searchDatabase();
    
    while (Serial.read() != -1);
}

bool searchDatabase(void) 
{
    FPMStatus status;
    
    /* Take a snapshot of the input finger */
    Serial.println("Place a finger.");
    
    do {
        status = finger.getImage();
        
        switch (status) 
        {
            case FPMStatus::OK:
                Serial.println("Image taken");
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
    
    /* Extract the fingerprint features */
    status = finger.image2Tz();

    switch (status) 
    {
        case FPMStatus::OK:
            Serial.println("Image converted");
            break;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "image2Tz(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }

    /* Search the database for the converted print */
    uint16_t fid, score;
    status = finger.searchDatabase(&fid, &score);
    
    switch (status)
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Found a match at ID #%u with confidence %u", fid, score);
            Serial.println(printfBuf);
            break;
            
        case FPMStatus::NOTFOUND:
            Serial.println("Did not find a match.");
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "searchDatabase(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    /* Now wait for the finger to be removed, though not necessary. 
       This was moved here after the Search operation because of the R503 sensor, 
       whose searches oddly fail if they happen after the image buffer is cleared  */
    Serial.println("Remove finger.");
    delay(1000);
    do {
        status = finger.getImage();
        delay(200);
    }
    while (status != FPMStatus::NOFINGER);
    
    return true;
}
