#include <SoftwareSerial.h>
#include <FPM.h>

// a sketch to stream fingerprint images to a PC over Serial
// a python script is provided to work in conjunction with this sketch

SoftwareSerial myser(2, 3);

FPM finger;

void setup()  
{
  Serial.begin(57600);
  myser.begin(57600);
  
  if (finger.begin(&myser)) {
    Serial.println("Found fingerprint sensor!");
    Serial.print("Capacity: "); Serial.println(finger.capacity);
    Serial.print("Packet length: "); Serial.println(finger.packetLen);
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) yield();
  }

  sendtoPC();
}

void loop() {
  
}

void sendtoPC(){
  set_packet_len();

  delay(100);
  int p = -1;
  Serial.println("Waiting for a finger...");
  while (p != FINGERPRINT_OK){
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
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
    case FINGERPRINT_OK:
      Serial.println("Streaming image...");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return;
    case FINGERPRINT_UPLOADFAIL:
      Serial.println("Cannot transfer the image");
      return;
  }

  Serial.write('\t');  // to indicate start of image stream to PC
  bool last = true;
  int count = 0;
  
  while (true){
    bool ret = finger.readRaw(&Serial, STREAM_TYPE, &last);
    if (ret){
      count++;
      if (last)
        break;
    }
    else {
      Serial.print("Error receiving packet ");
      Serial.println(count);
      return;
    }
    yield();
  }
  Serial.print(count * finger.packetLen); Serial.println(" bytes read");
  Serial.println("Image stream complete");
}

void set_packet_len(void){
  uint8_t param = SET_PACKET_LEN; // Example
  uint8_t value = PACKET_128;
  int p = finger.setParam(param, value);
  switch (p){
    case FINGERPRINT_OK:
      Serial.println("Packet length set to 128 bytes");
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
}

