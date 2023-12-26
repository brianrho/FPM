#include <SoftwareSerial.h>
#include <fpm.h>

/* Match 2 fingerprints/templates against each other */

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
    
    Serial.println("1:1 MATCH example");

    if (finger.begin()) 
    {
        finger.readParams(&params);
        Serial.println("Found fingerprint sensor!");
        Serial.print("Capacity: "); Serial.println(params.capacity);
        Serial.print("Packet length: "); Serial.println(FPM::packetLengths[static_cast<uint8_t>(params.packetLen)]);
    } 
    else 
    {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
}

void loop()
{
    Serial.println("\r\nEnter the template ID # to be matched...");
    
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
    
    Serial.print("Template ID: "); Serial.println(fid);
    Serial.println();
    
    comparePrints(fid);
    
    while (Serial.read() != -1) yield();
}

bool comparePrints(uint16_t fid) 
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
    
    /* Extract the fingerprint features into Buffer 1 */
    status = finger.image2Tz(1);

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
    
    /* Now wait for the finger to be removed */
    Serial.println("Remove finger.");
    delay(1000);
    do {
        status = finger.getImage();
        delay(200);
    }
    while (status != FPMStatus::NOFINGER);
    
    /* read the other template into Buffer 2 */
    status = finger.loadTemplate(fid, 2);
    
    switch (status) 
    {
        case FPMStatus::OK:
            Serial.print("Template "); Serial.print(fid); Serial.println(" loaded");
            break;
            
        case FPMStatus::DBREADFAIL:
            Serial.println(F("Invalid template or location"));
            return false;
            
        default:
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("loadTemplate(%d): error 0x%X"), fid, static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    /* Compare the contents of both Buffers to see if they match */
    uint16_t score;
    status = finger.matchTemplatePair(&score);
    
    switch (status)
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Both prints are a match! Confidence: %u", score);
            Serial.println(printfBuf);
            break;
            
        case FPMStatus::NOMATCH:
            Serial.println("Both prints are NOT a match.");
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "matchTemplatePair(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    return true;
}
