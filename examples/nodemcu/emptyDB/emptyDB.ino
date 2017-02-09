#include <SoftwareSerial.h>
#include <FPM.h>

//Empty the template database

SoftwareSerial mySerial(12, 14);

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
    while (1);
  }
  Serial.println("Send any character to continue...");
  while (Serial.available() == 0);
}

void loop() {
  int p = -1;
  while (p != FINGERPRINT_OK){
    p = finger.emptyDatabase();
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
  while (1);
}
