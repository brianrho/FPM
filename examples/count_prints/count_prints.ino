#include <SoftwareSerial.h>
#include <FPM.h>

/* Get number of stored templates/prints in database */

/*  pin #2 is IN from sensor (GREEN wire)
    pin #3 is OUT from arduino  (WHITE/YELLOW wire)
*/
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPM_System_Params params;

void setup()
{
    Serial.begin(9600);
    Serial.println("COUNT TEMPLATES test");
    fserial.begin(57600);

    if (finger.begin()) {
        finger.readParams(&params);
        Serial.println("Found fingerprint sensor!");
        Serial.print("Capacity: "); Serial.println(params.capacity);
        Serial.print("Packet length: "); Serial.println(fpm_packet_lengths[params.packet_len]);
    }
    else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
}

void loop() {
    while (Serial.read() != -1);
    Serial.println("Send any character to get the number of stored templates...");
    while (Serial.available() == 0) yield();

    uint16_t template_cnt;
    if (!get_template_count(&template_cnt))
        return;
    Serial.print(template_cnt);
    Serial.println(" print(s) in module database.");
}

bool get_template_count(uint16_t * count) {
    int16_t p = finger.getTemplateCount(count);
    if (p == FPM_OK) {
        return true;
    }
    else {
        Serial.print("Unknown error: ");
        Serial.println(p);
        return false;
    }
}