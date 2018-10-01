#include <SoftwareSerial.h>
#include <FPM.h>

/* Read, delete and move a fingerprint template within the database */

/*  pin #2 is IN from sensor (GREEN wire)
    pin #3 is OUT from arduino  (WHITE/YELLOW wire)
*/
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPM_System_Params params;

void setup()
{
    Serial.begin(9600);
    Serial.println("TEMPLATES test");
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

void loop()
{
    Serial.println("Enter the template ID # you want to get...");
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

    if (!read_template(fid))
        return;
        
    delete_template(fid);

    Serial.println("Enter the template's new ID...");
    int16_t new_id = 0;

    while (Serial.read() != -1);
    while (true) {
        while (! Serial.available()) yield();
        char c = Serial.read();
        if (! isdigit(c)) break;
        new_id *= 10;
        new_id += c - '0';
        yield();
    }

    move_template(new_id);
}

#define BUFF_SZ     512

uint8_t template_buffer[BUFF_SZ];

bool read_template(uint16_t fid)
{
    int16_t p = finger.loadModel(fid);
    switch (p) {
        case FPM_OK:
            Serial.print("Template "); Serial.print(fid); Serial.println(" loaded");
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return false;
        case FPM_DBREADFAIL:
            Serial.println("Invalid template");
            return false;
        default:
            Serial.print("Unknown error "); Serial.println(p);
            return false;
    }

    // OK success!

    p = finger.getModel();
    switch (p) {
        case FPM_OK:
            break;
        default:
            Serial.print("Unknown error "); Serial.println(p);
            return false; 
    }

    bool read_finished;
    int16_t count = 0;
    uint16_t readlen = BUFF_SZ;
    uint16_t pos = 0;

    while (true) {
        bool ret = finger.readRaw(FPM_OUTPUT_TO_BUFFER, template_buffer + pos, &read_finished, &readlen);
        if (ret) {
            count++;
            pos += readlen;
            readlen = BUFF_SZ - pos;
            if (read_finished)
                break;
        }
        else {
            Serial.println("Error receiving packet");
            return false;
        }
        yield();
    }

    Serial.println("Printing template:");
    Serial.println("---------------------------------------------");
    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 16; col++) {
            Serial.print(template_buffer[row * 16 + col], HEX);
            Serial.print(" ");
        }
        Serial.println();
        yield();
    }
    Serial.println("--------------------------------------------");

    Serial.print(count * fpm_packet_lengths[params.packet_len]); Serial.println(" bytes read.");
    return true;
}

void delete_template(uint16_t fid) {
    int16_t p = finger.deleteModel(fid);
    switch (p) {
        case FPM_OK:
            Serial.print("Template "); Serial.print(fid); Serial.println(" deleted");
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Comms error");
            break;
        case FPM_BADLOCATION:
            Serial.println("Could not delete from that location");
            break;
        case FPM_FLASHERR:
            Serial.println("Error writing to flash");
            break;
        default:
            Serial.println("Unknown error");
    }
    return;
}

void move_template(uint16_t fid) {
    int16_t p = finger.uploadModel();
    switch (p) {
        case FPM_OK:
            Serial.println("Starting template upload");
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Comms error");
            return;
        case FPM_PACKETRESPONSEFAIL:
            Serial.println("Did not receive packet");
            return;
        default:
            Serial.println("Unknown error");
            return;
    }

    yield();
    finger.writeRaw(template_buffer, BUFF_SZ);

    p = finger.storeModel(fid);
    switch (p) {
        case FPM_OK:
            Serial.print("Template moved to ID "); Serial.println(fid);
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Comms error");
            break;
        case FPM_BADLOCATION:
            Serial.println("Could not store in that location");
            break;
        case FPM_FLASHERR:
            Serial.println("Error writing to flash");
            break;
        default:
            Serial.println("Unknown error");
    }
    return;
}
