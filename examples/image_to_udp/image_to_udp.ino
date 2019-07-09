#include <SoftwareSerial.h>
#include <FPM.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

/* Send a fingerprint image to an Ethernet UDP server in 128-byte chunks.
 * Set the baud rate to 19200 first (with the `set_parameters` example)
 * before trying this. 
 * 19200 is chosen here because it's what worked for [hell77] with an Arduino Uno
 * and Ethernet shield, so you may get different results with an ESP8266/32 with UDP over WiFi, for instance.
 */

/*  pin #2 is IN from sensor (GREEN wire)
    pin #3 is OUT from arduino  (WHITE/YELLOW wire)
*/
SoftwareSerial fserial(2, 3);

FPM finger(&fserial);
FPM_System_Params params;

/* Ethernet setup, provide your MAC and IP details here */
byte mac[] = {
  0xDE, 0xAD, 0xDE, 0xEF, 0xFF, 0xEF
};

IPAddress local_ip(192, 168, 3, 177);
IPAddress remote_ip(192, 168, 3, 156);

unsigned int local_port = 8888;
unsigned int remote_port = 9999;
EthernetUDP client;

void setup()
{
    Serial.begin(19200);
    fserial.begin(19200);
    Serial.println("UDP IMAGE TRANSFER test");

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
    
    Ethernet.begin(mac, local_ip);
    client.begin(local_port);
}

void loop() {
    stream_image();
    while (1) yield();
}

/* set to the current sensor packet length, 128 by default */
#define TRANSFER_SZ     128
uint8_t buffer[TRANSFER_SZ];

void stream_image(void) {
    if (!set_packet_len_128()) {
        Serial.println("Could not set packet length");
        return;
    }

    delay(100);
    
    int16_t p = -1;
    Serial.println("Waiting for a finger...");
    while (p != FPM_OK) {
        p = finger.getImage();
        switch (p) {
            case FPM_OK:
                Serial.println("Image taken");
                break;
            case FPM_NOFINGER:
                break;
            case FPM_PACKETRECIEVEERR:
                Serial.println("Communication error");
                break;
            case FPM_IMAGEFAIL:
                Serial.println("Imaging error");
                break;
            default:
                Serial.println("Unknown error");
                break;
        }
        yield();
    }

    p = finger.downImage();
    switch (p) {
        case FPM_OK:
            Serial.println("Starting image stream...");
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return;
        case FPM_UPLOADFAIL:
            Serial.println("Cannot transfer the image");
            return;
    }
    
    /* flag to know when we're done */
    bool read_finished;
    /* indicate the max size to read, and also returns how much was actually read */
	uint16_t readlen = TRANSFER_SZ;
    uint16_t count = 0;

    while (true) {
        /* start composing packet to remote host */
        client.beginPacket(remote_ip, remote_port);
        
        bool ret = finger.readRaw(FPM_OUTPUT_TO_BUFFER, buffer, &read_finished, &readlen);
        if (ret) {
            count++;
            /* we now have a complete packet, so send it */
			client.write(buffer, readlen);
            client.endPacket();
            
            /* indicate the length to be read next time like before */
			readlen = TRANSFER_SZ;
            if (read_finished)
                break;
        }
        else {
            Serial.print("\r\nError receiving packet ");
            Serial.println(count);
            return;
        }
        yield();
    }

    Serial.println();
    Serial.print(count * FPM::packet_lengths[params.packet_len]); Serial.println(" bytes read.");
    Serial.println("Image stream complete.");
}


/* set packet length to 128 bytes,
   no need to call this for R308 sensor */
bool set_packet_len_128(void) {
    uint8_t param = FPM_SETPARAM_PACKET_LEN; // Example
    uint8_t value = FPM_PLEN_128;
    int16_t p = finger.setParam(param, value);
    switch (p) {
        case FPM_OK:
            Serial.println("Packet length set to 128 bytes");
            break;
        case FPM_PACKETRECIEVEERR:
            Serial.println("Comms error");
            break;
        case FPM_INVALIDREG:
            Serial.println("Invalid settings!");
            break;
        default:
            Serial.println("Unknown error");
    }

    return (p == FPM_OK);
}
