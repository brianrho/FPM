#include <SoftwareSerial.h>
#include <FPM.h>
//Change the module password with this example

// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE/YELLOW wire)
SoftwareSerial mySerial(2, 3);

FPM finger;

uint32_t new_password = 0x78563412;

void setup()  
{
  Serial.begin(9600);
  mySerial.begin(57600);
  Serial.println("SET PASSWORD EXAMPLE");
  Serial.println("========================");
 
  if (finger.begin(&mySerial)) {
    Serial.println("Found fingerprint sensor!");
    Serial.print("Capacity: "); Serial.println(finger.capacity);
    Serial.print("Packet length: "); Serial.println(finger.packetLen);
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) yield();
  }
  while (Serial.read() != -1);    // clear buffer

  Serial.print("Send any character to set the module password to 0x"); Serial.println(new_password, HEX);
  while (Serial.available() == 0) yield();
  if (set_pwd(new_password)) {
    delay(1000);
    Serial.println("Testing new password by calling begin() again");
    if (finger.begin(&mySerial, new_password)) {
      Serial.println("Found fingerprint sensor!");
      Serial.print("Capacity: "); Serial.println(finger.capacity);
      Serial.print("Packet length: "); Serial.println(finger.packetLen);
    } else {
      Serial.println("Did not find fingerprint sensor :(");
      while (1) yield();
    }
  }
}

void loop()                    
{
  
}

bool set_pwd(uint32_t pwd) {
  uint8_t ret = finger.setPassword(pwd);
  switch (ret) {
    case FINGERPRINT_OK:
      Serial.println("Password changed successfully. Will take hold next time you call begin().");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Comms error!");
      break;
  }
  return (ret == FINGERPRINT_OK);
}

