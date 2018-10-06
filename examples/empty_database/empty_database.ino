#include <SoftwareSerial.h>
#include <FPM.h>

/* Empty fingerprint database */

/*  pin #2 is IN from sensor (GREEN wire)
    pin #3 is OUT from arduino  (WHITE/YELLOW wire)
*/
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPM_System_Params params;

void setup()
{
    Serial.begin(9600);
    Serial.println("EMPTY DATABASE test");
    fserial.begin(57600);

    if (finger.begin()) {
        finger.readParams(&params);
        Serial.println("Found fingerprint sensor!");
        Serial.print("Capacity: "); Serial.println(params.capacity);
        Serial.print("Packet length: "); Serial.println(FPM::packet_lengths[params.packet_len]);
    } else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
}

void loop() {
    Serial.println("Send any character to empty the database...");
    while (Serial.available() == 0) yield();
    empty_database();
    while (Serial.read() != -1);
}

void empty_database(void) {
    int16_t p = finger.emptyDatabase();
    if (p == FPM_OK) {
        Serial.println("Database empty!");
    }
    else if (p == FPM_PACKETRECIEVEERR) {
        Serial.print("Communication error!");
    }
    else if (p == FPM_DBCLEARFAIL) {
        Serial.println("Could not clear database!");
    } 
    else if (p == FPM_TIMEOUT) {
        Serial.println("Timeout!");
    } 
    else if (p == FPM_READ_ERROR) {
        Serial.println("Got wrong PID or length!");
    } 
    else {
        Serial.println("Unknown error");
    }
}
