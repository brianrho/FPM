#include <SoftwareSerial.h>
#include <FPM.h>

/* Set module password */

/*  pin #2 is IN from sensor (GREEN wire)
    pin #3 is OUT from arduino  (WHITE/YELLOW wire)
*/
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPM_System_Params params;

uint32_t new_password = 0x12345678;

void setup()
{
    Serial.begin(9600);
    Serial.println("SET PASSWORD test");
    fserial.begin(57600);

    if (finger.begin()) {
        finger.readParams(&params);
        Serial.println("Found fingerprint sensor!");
        Serial.print("Capacity: "); Serial.println(params.capacity);
        Serial.print("Packet length: "); Serial.println(FPM::packet_lengths[params.packet_len]);
    }
    else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
}

void loop()
{
    while (Serial.read() != -1);
    Serial.print("Send any character to set the module password to 0x");
    Serial.println(new_password, HEX);

    while (Serial.available() == 0) yield();

    if (!set_pwd(new_password)) {
        Serial.println("Failed to set password!");
        return;
    }

    delay(1000);
    Serial.println("Testing new password by calling begin() again");
    if (finger.begin(new_password)) {
        finger.readParams(&params);
        Serial.println("Found fingerprint sensor!");
        Serial.print("Capacity: "); Serial.println(params.capacity);
        Serial.print("Packet length: "); Serial.println(FPM::packet_lengths[params.packet_len]);
    }
    else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
}

bool set_pwd(uint32_t pwd) {
    int16_t ret = finger.setPassword(pwd);
    switch (ret) {
        case FPM_OK:
            Serial.println("Password changed successfully. Will take hold next time you call begin().");
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Comms error!");
            break;
        default:
            Serial.println("Unknown error");
            break;
    }
    return (ret == FPM_OK);
}