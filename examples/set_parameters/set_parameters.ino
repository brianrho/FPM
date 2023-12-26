#include <SoftwareSerial.h>
#include <fpm.h>

/* Set sensor parameters -- in this case, the sensor baud rate
 *
 * WARNING: Changing the parameters of your sensor is Serious Business.
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

const uint32_t baudRates[] = {
    9600,
    19200,
    28800,
    38400,
    48000,
    57600,
    67200,
    76800,
    86400,
    96000,
    105600,
    115200
};

void setup()
{
    Serial.begin(57600);
    fserial.begin(57600);
    
    Serial.println("SET_PARAMETERS example");

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
    /* Change this to any baud rate you like */
    FPMBaud newRate = FPMBaud::B57600;
    
    Serial.print("\r\nSend any character to set the sensor baud rate to ");
    Serial.println(baudRates[static_cast<uint8_t>(newRate) - 1]);
    
    while (Serial.available() == 0) yield();
    
    if (setBaudRate(newRate))
    {
        /* Restart the Serial port at the new rate.
         * Restarting the sensor is also recommended. */
        fserial.end();
        
        Serial.println("\r\nYou may need to restart your sensor and Arduino.\r\nTesting new baud rate...\r\n");
        
        fserial.begin(baudRates[static_cast<uint8_t>(newRate) - 1]);
        
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

bool setBaudRate(FPMBaud newRate) 
{
    FPMStatus status = finger.setBaudRate(newRate);
    
    switch (status) 
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Baud rate set to %lu successfully.", baudRates[static_cast<uint8_t>(newRate) - 1]);
            Serial.println(printfBuf);
            break;
            
        case FPMStatus::INVALIDREG:
            Serial.println("Invalid parameters!");
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "setBaudRate(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }

    return true;
}
