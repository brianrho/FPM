#include <SoftwareSerial.h>
#include <FPM.h>

//Set system parameters
#define TEMPLATES_PER_PAGE  256

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
    while (1);
  }

  Serial.println("Send any character to continue...");
  while (Serial.available() == 0);
}

void loop(){
  uint8_t param = BAUD_RATE; // Change baud rate to 9600
  uint8_t value = BAUD_9600;
  int p = finger.setParam(param, value);
  switch (p){
    case FINGERPRINT_OK:
      Serial.println("Baud rate set to 9600 successfully");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Comms error");
      break;
    case FINGERPRINT_INVALIDREG:
      Serial.println("Invalid settings!");
      break;
    default:
      Serial.println("Unknown error");
  }
  mySerial.end();   // Switch to new baud rate here
  Serial.println("Testing new baud rate by running a GET_FREE_ID command...");
  mySerial.begin(57600);
  int16_t id;     // just test any command to see if the change was successful
  get_free_id(&id);
  while (1);
}

bool get_free_id(int16_t * id){
  int p = -1;
  for (int page = 0; page < (finger.capacity / TEMPLATES_PER_PAGE) + 1; page++){
    p = finger.getFreeIndex(page, id);
    switch (p){
      case FINGERPRINT_OK:
        if (*id != FINGERPRINT_NOFREEINDEX){
          Serial.print("Free slot at ID ");
          Serial.println(*id);
          return true;
        }
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error!");
        return false;
      default:
        Serial.println("Unknown error!");
        return false;
    }
  }
}
