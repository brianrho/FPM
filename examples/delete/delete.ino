#include <SoftwareSerial.h>
#include <FPM.h>

// Search the DB for a print with this example

SoftwareSerial mySerial(2, 3);

FPM finger;

void setup()  
{
  Serial.begin(9600);
  Serial.println("fingertest");
  mySerial.begin(57600);
  
  if (finger.begin(&mySerial)) {
    Serial.println("Found fingerprint sensor!");
    Serial.print("Capacity: "); Serial.println(finger.capacity);
    Serial.print("Packet length: "); Serial.println(finger.packetLen);
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) yield();
  }
}

void loop()                     // run over and over again
{
  Serial.println("Type in the ID # you want to delete...");
  int id = 0;
  while (true) {
    while (! Serial.available()) yield();
    char c = Serial.read();
    if (! isdigit(c)) break;
    id *= 10;
    id += c - '0';
    yield();
  }
  Serial.print("Deleting ID #");
  Serial.println(id);
  
  deleteFingerprint(id);
  while (Serial.read() != -1) yield();
}

int deleteFingerprint(int id) {
  int p = -1;
  
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }   
}
