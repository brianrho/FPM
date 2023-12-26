#include <SoftwareSerial.h>
#include <fpm.h>

/* Read, delete and write a fingerprint template into the database:
 * 
 * A template will be moved from one location in the database to another --
 * by first reading it from its initial location into a buffer, 
 * deleting it from that location, and then writing it from the buffer to its new location.
 * 
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

/* This should be equal to the template size for your sensor,
 * which can be anywhere from 512 to 1536 bytes.
 * 
 * So check the printed result of readTemplate() to determine 
 * the correct template size for your sensor and adjust the buffer
 * size below accordingly. If this example doesn't work at first, try increasing
 * this value, provided you have sufficient RAM.
 */
#define TEMPLATE_SZ         512
uint8_t template_buffer[TEMPLATE_SZ];

void setup()
{
    Serial.begin(57600);
    fserial.begin(57600);
    
    Serial.println("TEMPLATES example");

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
    Serial.println(F("\r\nEnter the template ID # you want to move..."));
    uint16_t fid = getIDInput();

    /* read the template from its location into the buffer */
    uint16_t totalRead = readTemplate(fid, template_buffer, TEMPLATE_SZ);
    if (!totalRead) return;

    /* delete it from that location */
    deleteTemplate(fid);

    Serial.println(F("Enter the template's new ID #..."));
    uint16_t new_fid = getIDInput();
    
    /* write the template from the buffer into its new location */
    writeTemplate(new_fid, template_buffer, totalRead);
}

uint16_t getIDInput(void)
{
    /* clear the Serial RX buffer */
    while (Serial.read() != -1);
    
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
    
    return fid;
}

uint16_t readTemplate(uint16_t fid, uint8_t * buffer, uint16_t bufLen)
{
    /* Load the template from the sensor's storage into one of its Buffers
     * (Buffer 1 by default) */
    FPMStatus status = finger.loadTemplate(fid);
    
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
    
    /* Inform the sensor that we're about to download a template from that Buffer */
    status = finger.downloadTemplate();
    switch (status) 
    {
        case FPMStatus::OK:
            break;
            
        default:
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("downloadTemplate(%d): error 0x%X"), fid, static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false; 
    }
    
    /* Now, the sensor will send us the template from that Buffer, one packet at a time */
    bool readComplete = false;
    
    /* As an argument, this holds the max number of bytes to read from the sensor.
     * Whenever the function returns successfully, it then holds the number of bytes actually read. */
    uint16_t readLen = bufLen;
    
    uint16_t bufPos = 0;

    while (!readComplete) 
    {
        bool ret = finger.readDataPacket(buffer + bufPos, NULL, &readLen, &readComplete);
        
        if (!ret)
        {
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("readDataPacket(): failed after reading %u bytes"), bufPos);
            Serial.println(printfBuf);
            return 0;
        }
        
        bufPos += readLen;
        readLen = bufLen - bufPos;
        
        yield();
    }
    
    uint16_t totalBytes = bufPos;
    
    /* just for pretty-printing */
    uint16_t numRows = totalBytes / 16;
    
    Serial.println(F("Printing template:"));
    const char * dashes = "---------------------------------------------";
    Serial.println(dashes);
    
    for (int row = 0; row < numRows; row++) {
        for (int col = 0; col < 16; col++) {
            Serial.print(buffer[row * 16 + col], HEX);
            Serial.print(" ");
        }
        
        Serial.println();
        yield();
    }
    
    Serial.println(dashes);

    Serial.print(totalBytes); Serial.println(" bytes read.");
    return totalBytes;
}

bool deleteTemplate(int fid) 
{
    FPMStatus status = finger.deleteTemplate(fid);
    
    switch (status) 
    {
        case FPMStatus::OK:
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("Deleted ID #%u."), fid);
            Serial.println(printfBuf);
            break;
            
        case FPMStatus::DELETEFAIL:
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("Failed to delete ID #%u."), fid);
            Serial.println(printfBuf);
            return false;
            
        default:
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("deleteTemplate(): error 0x%X"), static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    return true;
}

bool writeTemplate(uint16_t fid, uint8_t * buffer, uint16_t templateSz) 
{
    /* Inform the sensor that we're about to upload a template to one of its Buffers
     * (Buffer 1 by default) */
    FPMStatus status = finger.uploadTemplate();
    
    switch (status) 
    {
        case FPMStatus::OK:
            Serial.println("Starting template upload...");
            break;
            
        default:
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("uploadTemplate(%u): error 0x%X"), fid, static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false; 
    }
    
    /* One packet at a time, upload the template to that Buffer */
    uint16_t writeLen = templateSz;
    uint16_t written = 0;
    const uint16_t PACKET_LENGTH = FPM::packetLengths[static_cast<uint8_t>(params.packetLen)];
    
    while (writeLen)
    {
        /* the write is completed when we have no more than PACKET_LENGTH left to write */
        bool ret = finger.writeDataPacket(buffer + written, NULL, &writeLen, writeLen <= PACKET_LENGTH);
        
        if (!ret)
        {
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("writeDataPacket(): failed after writing %u bytes"), written);
            Serial.println(printfBuf);
            return false;
        }
        
        written += writeLen;
        writeLen = templateSz - written;
        
        yield();
    }
    
    /* Finally, store the template at the specified location */
    status = finger.storeTemplate(fid);

    switch (status)
    {
        case FPMStatus::OK:
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("Template stored at ID %d!"), fid);
            Serial.println(printfBuf);
            break;
            
        case FPMStatus::BADLOCATION:
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("Could not store in that location %d!"), fid);
            Serial.println(printfBuf);
            return false;
            
        default:
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("storeModel(%u): error 0x%X"), fid, static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    return true;
}
