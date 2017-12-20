This is an Arduino library for the R30x/ZFMxx/FPMxx optical fingerprint sensors. 
Also included is a Python script and an executable for extracting fingerprint images to a PC; the 'finger2PC' example must first be uploaded to the Arduino. 
For optimum reliability, baud rates <= 57600 are recommended regarding SoftwareSerial usage, especially when retrieving fingerprint images.

Datasheet found at https://sicherheitskritisch.de/files/specifications-2.0-en.pdf

Note: The led_on(), led_off() and getImageNL() methods have only been tested successfully on ZFM60 modules.
