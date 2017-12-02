#include <SoftwareSerial.h>
#include <FPM.h>

//Count stored fingerprints with this example

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
  
  int p = finger.getTemplateCount();
  if (p == FINGERPRINT_OK){
    Serial.print(finger.templateCount); Serial.println(" print(s) in module memory.");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
    Serial.println("Communication error!");
  else
    Serial.println("Unknown error!");
}

void loop() {
  // put your main code here, to run repeatedly:

}
