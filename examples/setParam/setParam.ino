#include <SoftwareSerial.h>
#include <FPM.h>

/* Set system parameters such as the baud rate */

/*  pin #2 is IN from sensor (GREEN wire)
    pin #3 is OUT from arduino  (WHITE/YELLOW wire)
*/
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPM_System_Params params;

void setup()
{
    Serial.begin(9600);
    Serial.println("SET PARAM test");
    fserial.begin(57600);

    if (finger.begin()) {
        finger.readParams(&params);
        Serial.println("Found fingerprint sensor!");
        Serial.print("Capacity: "); Serial.println(params.capacity);
        Serial.print("Packet length: "); Serial.println(fpm_packet_lengths[params.packet_len]);
    } else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
}

void change_baud_rate(void) {
    uint8_t param = FPM_SETPARAM_BAUD_RATE;
    uint8_t value = FPM_BAUD_9600;
    int16_t p = finger.setParam(param, value);
    switch (p) {
        case FPM_OK:
            Serial.println("Baud rate set to 9600 successfully");
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Comms error");
            return;
        case FPM_INVALIDREG:
            Serial.println("Invalid settings!");
            return;
        default:
            Serial.print("Unknown error "); Serial.println(p);
            return;
    }

    /* Switch to new baud rate here; may or may not work,
     * Module restart may still be required */
    fserial.end();
    Serial.println("Testing new baud rate by trying a GET_FREE_ID...");
    fserial.begin(9600);
    finger.begin();
    
    int16_t fid;
    get_free_id(&fid);
    /* best to restart module at this point,
     *  running setparam again or even other commands causes unreliable behaviour
     */
}

void loop() {
    while (Serial.read() != -1);
    Serial.println("Send any character to continue...");
    while (Serial.available() == 0) yield();
    change_baud_rate();
    while (Serial.read() != -1);
}

bool get_free_id(int16_t * fid) {
    int16_t p = -1;
    for (int page = 0; page < (params.capacity / FPM_TEMPLATES_PER_PAGE) + 1; page++) {
        p = finger.getFreeIndex(page, fid);
        switch (p) {
            case FPM_OK:
                if (*fid != FPM_NOFREEINDEX) {
                    Serial.print("Free slot at ID ");
                    Serial.println(*fid);
                    return true;
                }
            case FPM_PACKETRECIEVEERR:
                Serial.println("Communication error!");
                return false;
            case FPM_TIMEOUT:
                Serial.println("Timeout!");
                return false;
            case FPM_READ_ERROR:
                Serial.println("Got wrong PID or length!");
                return false;
            default:
                Serial.println("Unknown error!");
                return false;
        }
        yield();
    }
}