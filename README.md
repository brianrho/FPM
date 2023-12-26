# FPM
This is an Arduino library for most of the FPMxx/R30x/ZFMxx/R551 optical fingerprint sensors.\
(*See notes below.*)

There are several examples included to demonstrate usage -- the `enroll` example is a great place to start, especially if you have an ESP32.\
(This is because ESP32 requires usage of its `HardwareSerial` ports, since the Arduino ESP32 core doesn't support `SoftwareSerial`. The `enroll` example shows how to setup those ports, guarded under the macro `ARDUINO_ARCH_ESP32`.)

Also included is a Python 3 script for extracting fingerprint images to a PC over a Virtual COM port. To use it:
- The `image_to_pc` example must first be uploaded to the Arduino, which is connected to the sensor. 
- The script requires the `pyserial` Python package, so you need to install that first with `pip3 install pyserial` on your command line.
- Run `python3 getImage.py -h` to see general usage help. (For instance, usage on Windows could look like this: `python3 getImage.py COM3 57600 print.bmp`)

To get the most reliability with `SoftwareSerial`, baud rates should not exceed 57600, especially during sustained data transfers e.g. extracting fingerprint images. *(This recommendation is based off old tests with an Arduino Uno -- more powerful chips like the ESPxxxx may be able to handle higher rates just fine. Test and find out.)*

A generic list of commands for these sensors can be found [here](https://usermanual.wiki/Document/Fingerprintusermanual.1754127921/view). Keep in mind that not all sensors (few, really) support all commands. You'll just have to try them out for yourself.\
To match templates on your PC/server, check [here](https://github.com/brianrho/fpmatch).\
A device-agnostic version of this [library in C](https://github.com/brianrho/FPM-C).\
An Arduino library for the [GT511C3 (and similar GT5x) fingerprint sensors](https://github.com/brianrho/GT5X).\


## Notes
* The R308 sensor is tentatively supported for now. Since its settings cannot be read by the usual commands, they have to be set manually to defaults based on the datasheet, at the risk that these defaults may be wrong. In any case, **make sure** to check the `setup()` of the `R308_search_database` example for how to properly initialize your sensor.

* The R551 seems to have several clones, and a datasheet/SDK that's inconsistent with the sensor's actual behaviour. To use one of these sensors, make sure to uncomment `FPM_R551_MODULE` in `FPM.h`.\
Despite that, you may still encounter problems, especially with more advanced functionality like image/template downloads. So instead, you may want to buy an FPM10, R305/7, ZFM60 or R503. Naturally, caveat emptor.

* If you have an ESP32, the `enroll` example already shows how to setup the `HardwareSerial` ports, so this is basically clearer repetition:

```cpp
#include <HardwareSerial.h>
#include <fpm.h>

/* select UART1 */
HardwareSerial fserial(1);

FPM finger(&fserial);
FPMSystemParams params;

void setup(void) {
    Serial.begin(57600);
    
    /* ESP32 RX = IO25, TX = IO32 */
    fserial.begin(57600, SERIAL_8N1, 25, 32);
    
    /* the rest of the code follows, same as in the examples */
    if (finger.begin()) {
        ...
    }
}

void loop(void) {
    ...
}

```
