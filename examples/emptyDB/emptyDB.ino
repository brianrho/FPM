#include <SoftwareSerial.h>
#include <FPM.h>

//Empty the template database

// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE/YELLOW wire)
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
  
    uint8_t p = finger.emptyDatabase();
    if (p == FINGERPRINT_OK){
      Serial.println("Database empty!");
    }
    else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.print("Communication error!");
    }
    else if (p == FINGERPRINT_DBCLEARFAIL) {
      Serial.println("Could not clear database!");
    }
}

void loop() {
    
}
