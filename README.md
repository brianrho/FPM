This is an Arduino library for most of the FPMxx/R30x/ZFMxx/R551 optical fingerprint sensors.\
(*See note below.*)

Included are a Python script and a Windows executable for extracting fingerprint images to a PC; 
the `image_to_pc` example must first be uploaded to the Arduino. \
For optimal reliability, baud rates <= 57600 are recommended regarding `SoftwareSerial` usage, 
especially when retrieving fingerprint images. 

Datasheet found [here](https://sicherheitskritisch.de/files/specifications-2.0-en.pdf).\
To match templates on your PC/server, check [here](https://github.com/brianrho/fpmatch).\
A generic version of this [library in C](https://github.com/brianrho/FPM-C).\
An Arduino library for the [GT511C3 (and similar GT5x) fingerprint sensors](https://github.com/brianrho/GT5X).\
\
**Note**: 
* The R308 is tentatively supported for now. Since its settings cannot be read by the usual commands, they have to be\
set manually to defaults based on the datasheet, at the risk that these defaults may be wrong.

* The R551 seems to have clones, and a datasheet/SDK inconsistent with the sensor's actual behaviour.\
To use one of them with this library, make sure to uncomment `FPM_R551_MODULE` in `FPM.h`.
However, they generally have trouble working with the library, particularly more advanced functionality like image/template downloads.\
To take full advantage of the available functionality, it's recommended you get an FPM10, R305/7 or ZFM60,\
especially the first which has been well-tested with this library.