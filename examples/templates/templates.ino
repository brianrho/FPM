#include <SoftwareSerial.h>
#include <FPM.h>

/* Read, delete and move a fingerprint template within the database */

/*  pin #2 is IN from sensor (GREEN wire)
    pin #3 is OUT from arduino  (WHITE/YELLOW wire)
*/
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPM_System_Params params;

/* this is equal to the template size for the fingerprint module,
 * some modules have 512-byte templates while others have
 * 768-byte templates. Check the printed result of read_template()
 * to determine the case for your module and adjust the needed buffer
 * size below accordingly */
#define BUFF_SZ     768

uint8_t template_buffer[BUFF_SZ];

void setup()
{
    Serial.begin(9600);
    Serial.println("TEMPLATES test");
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

    /* read the template from its location into the buffer */
    uint16_t total_read = read_template(fid, template_buffer, BUFF_SZ);
    if (!total_read)
        return;

    /* delete it from that location */
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

    /* copy it from the buffer to its new location */
    move_template(new_id, template_buffer, total_read);
}

uint16_t read_template(uint16_t fid, uint8_t * buffer, uint16_t buff_sz)
{
    int16_t p = finger.loadModel(fid);
    switch (p) {
        case FPM_OK:
            Serial.print("Template "); Serial.print(fid); Serial.println(" loaded");
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return 0;
        case FPM_DBREADFAIL:
            Serial.println("Invalid template");
            return 0;
        default:
            Serial.print("Unknown error "); Serial.println(p);
            return 0;
    }

    // OK success!

    p = finger.getModel();
    switch (p) {
        case FPM_OK:
            break;
        default:
            Serial.print("Unknown error "); Serial.println(p);
            return 0; 
    }

    bool read_finished;
    int16_t count = 0;
    uint16_t readlen = buff_sz;
    uint16_t pos = 0;

    while (true) {
        bool ret = finger.readRaw(FPM_OUTPUT_TO_BUFFER, buffer + pos, &read_finished, &readlen);
        if (ret) {
            count++;
            pos += readlen;
            readlen = buff_sz - pos;
            if (read_finished)
                break;
        }
        else {
            Serial.println("Error receiving packet");
            return 0;
        }
        yield();
    }
    
    uint16_t total_bytes = count * FPM::packet_lengths[params.packet_len];
    
    /* just for pretty-printing */
    uint16_t num_rows = total_bytes / 16;
    
    Serial.println("Printing template:");
    Serial.println("---------------------------------------------");
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < 16; col++) {
            Serial.print(buffer[row * 16 + col], HEX);
            Serial.print(" ");
        }
        Serial.println();
        yield();
    }
    Serial.println("--------------------------------------------");

    Serial.print(total_bytes); Serial.println(" bytes read.");
    return total_bytes;
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

void move_template(uint16_t fid, uint8_t * buffer, uint16_t to_write) {
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
    finger.writeRaw(buffer, to_write);

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