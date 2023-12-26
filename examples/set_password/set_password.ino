#include <SoftwareSerial.h>
#include <fpm.h>

/* Set the sensor password
 *
 * WARNING: Changing the password of your sensor is Serious Business.
 *          Run this sketch only if you know what you're doing.
 *
 * This command is not supported on R308 sensors.
 */
 
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
    
    Serial.println("SET_PASSWORD example");

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
    /* Change this to any value you like */
    const uint32_t newPassword = 0x00000000;
    
    Serial.print("\r\nSend any character to set the sensor password to 0x");
    Serial.println(newPassword, HEX);
    
    while (Serial.available() == 0) yield();
    
    if (setPassword(newPassword))
    {
        /* Restarting the sensor is recommended. */       
        Serial.println("\r\nYou may need to restart your sensor and Arduino.\r\nTesting new password...\r\n");
        
        /* Just in case */
        delay(1000);
        
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
    
    while (Serial.read() != -1);
}


bool setPassword(uint32_t password) 
{   
    FPMStatus status = finger.setPassword(password);
    
    switch (status) 
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Password set to 0x%08lX successfully.", password);
            Serial.println(printfBuf);
            break;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "setPassword(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }

    return true;
}
