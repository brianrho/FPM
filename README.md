This is an Arduino library for the R3xx/ZFMxx/FPMxx/R551 optical fingerprint sensors.

Included are a Python script and a Windows executable for extracting fingerprint images to a PC; 
the `image_to_pc` example must first be uploaded to the Arduino. 

For optimal reliability, baud rates <= 57600 are recommended regarding `SoftwareSerial` usage, 
especially when retrieving fingerprint images. 

Datasheet found at https://sicherheitskritisch.de/files/specifications-2.0-en.pdf

To match templates on your PC/server, check here: https://github.com/brianrho/fpmatch

A generic version of this library in C: https://github.com/brianrho/FPM-C

An Arduino library for the GT511C3 (and similar GT5x) fingerprint sensors: https://github.com/brianrho/GT5X
