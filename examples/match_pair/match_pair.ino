#include <SoftwareSerial.h>
#include <FPM.h>

/* Match 2 templates */

/*  pin #2 is IN from sensor (GREEN wire)
    pin #3 is OUT from arduino  (WHITE/YELLOW wire)
*/
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPM_System_Params params;

void setup()
{
    Serial.begin(9600);
    Serial.println("PAIR MATCH test");
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

void loop()
{
    Serial.println("Enter the template ID # to be matched...");
    int16_t fid = 0;
    while (Serial.read() != -1);
    while (true) {
        while (! Serial.available()) yield();
        char c = Serial.read();
        if (! isdigit(c)) break;
        fid *= 10;
        fid += c - '0';
        yield();
    }

    match_prints(fid);
}

/* capture a print, load it into slot 1, 
 *  load a second print #fid into the slot 2,
 *  and check if they match
 */
void match_prints(int16_t fid) {
    int16_t p = -1;

    /* first get the finger image */
    Serial.println("Waiting for valid finger");
    while (p != FPM_OK) {
        p = finger.getImage();
        switch (p) {
            case FPM_OK:
                Serial.println("Image taken");
                break;
            case FPM_NOFINGER:
                Serial.println(".");
                break;
            case FPM_PACKETRECIEVEERR:
                Serial.println("Communication error");
                return;
            default:
                Serial.println("Unknown error");
                return;
        }
        yield();
    }

    /* convert it and place in slot 1*/
    p = finger.image2Tz(1);
    switch (p) {
        case FPM_OK:
            Serial.println("Image converted");
            break;
        case FPM_IMAGEMESS:
            Serial.println("Image too messy");
            return;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return;
        default:
            Serial.println("Unknown error");
            return;
    }

    Serial.println("Remove finger");
    p = 0;
    while (p != FPM_NOFINGER) {
        p = finger.getImage();
        yield();
    }
    Serial.println();

    /* read template into slot 2 */
    p = finger.loadModel(fid, 2);
    switch (p) {
        case FPM_OK:
            Serial.print("Template "); Serial.print(fid); Serial.println(" loaded.");
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return;
        case FPM_DBREADFAIL:
            Serial.println("Invalid template");
            return;
        default:
            Serial.print("Unknown error "); Serial.println(p);
            return;
    }
    
    uint16_t match_score = 0;
    p = finger.matchTemplatePair(&match_score);
    switch (p) {
        case FPM_OK:
            Serial.print("Prints matched. Score: "); Serial.println(match_score);
            break;
        case FPM_NOMATCH:
            Serial.println("Prints did not match.");
            break;
        default:
            Serial.println("Unknown error");
            return;
    }
}
