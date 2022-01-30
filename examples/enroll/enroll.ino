#include <SoftwareSerial.h>
#include <fpm.h>

/* Enroll fingerprints */

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
    
    Serial.println("ENROLL example");

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
    Serial.println("\r\nSend any character to enroll a finger...");
    while (Serial.available() == 0) yield();
    
    Serial.println("Searching for a free slot to store the template...");
    
    int16_t fid;
    if (getFreeId(&fid)) 
    {
        enrollFinger(fid);
    }
    else 
    {
        Serial.println("No free slot/ID in database!");
    }
    
    while (Serial.read() != -1);  // clear buffer
}

bool getFreeId(int16_t * fid) 
{
    for (int page = 0; page < (params.capacity / FPM_TEMPLATES_PER_PAGE) + 1; page++) 
    {
        FPMStatus status = finger.getFreeIndex(page, fid);
        
        switch (status) 
        {
            case FPMStatus::OK:
                if (*fid != -1) {
                    Serial.print("Free slot at ID ");
                    Serial.println(*fid);
                    return true;
                }
                break;
                
            default:
                snprintf(printfBuf, PRINTF_BUF_SZ, "getFreeIndex(%d): error 0x%X", page, static_cast<uint16_t>(status));
                Serial.println(printfBuf);
                return false;
        }
        
        yield();
    }
    
    Serial.println("No free slots!");
    return false;
}

bool enrollFinger(int16_t fid) 
{
    FPMStatus status;
    const int NUM_SNAPSHOTS = 2;
    
    /* Take snapshots of the finger,
     * and extract the fingerprint features from each image */
    for (int i = 0; i < NUM_SNAPSHOTS; i++)
    {
        Serial.println(i == 0 ? "Place a finger" : "Place same finger again");
        
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

        status = finger.image2Tz(i+1);
        
        switch (status) 
        {
            case FPMStatus::OK:
                Serial.println("Image converted");
                break;
                
            default:
                snprintf(printfBuf, PRINTF_BUF_SZ, "image2Tz(%d): error 0x%X", i+1, static_cast<uint16_t>(status));
                Serial.println(printfBuf);
                return false;
        }

        Serial.println("Remove finger");
        delay(1000);
        do {
            status = finger.getImage();
            delay(200);
        }
        while (status != FPMStatus::NOFINGER);
        
    }
    
    /* Images have been taken and converted into features a.k.a character files.
     * Now, need to create a model/template from these features and store it. */
     
    status = finger.generateTemplate();
    switch (status)
    {
        case FPMStatus::OK:
            Serial.println("Template created from matching prints!");
            break;
            
        case FPMStatus::ENROLLMISMATCH:
            Serial.println("The prints do not match!");
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "createModel(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    status = finger.storeTemplate(fid);
    switch (status)
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Template stored at ID %d!", fid);
            Serial.println(printfBuf);
            break;
            
        case FPMStatus::BADLOCATION:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Could not store in that location %d!", fid);
            Serial.println(printfBuf);
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "storeModel(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    return true;
}
