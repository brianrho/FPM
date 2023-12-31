/*  Read a fingerprint image from the sensor and render it on a TFT display.

    This example depends on the TFT_eSPI library. 
    It was tested with an ESP32-WROOM + FPM10 sensor + 2.8" 240x320 ST7789 TFT display (using SPI).
*/

#if defined(ARDUINO_ARCH_ESP32)
#include <HardwareSerial.h>
#else
#include <SoftwareSerial.h>
#endif

#include <fpm.h>

#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#if defined(ARDUINO_ARCH_ESP32)
/*  For ESP32 only, use Hardware UART1:
    GPIO-25 is Arduino RX <==> Sensor TX
    GPIO-32 is Arduino TX <==> Sensor RX
*/
HardwareSerial fserial(1);
#else
/*  pin #2 is Arduino RX <==> Sensor TX
    pin #3 is Arduino TX <==> Sensor RX
*/
SoftwareSerial fserial(2, 3);
#endif

FPM finger(&fserial);
FPMSystemParams params;

/* for convenience */
#define PRINTF_BUF_SZ   60
char printfBuf[PRINTF_BUF_SZ];

/* The expected pixel width and height of the image -- this may vary, per sensor. */
#define IMAGE_WIDTH     256
#define IMAGE_HEIGHT    288

/*  The scaling ((mult/div)^2) applied to the original image area, when drawing it.
    e.g. When mult == 1 and div == 2, the drawn image will occupy IMAGE_WIDTH/2 by IMAGE_HEIGHT/2 pixels on the display,
    always preserving the aspect ratio.

    This is useful when the TFT display is not large enough to hold the entire image
    (e.g. 240x320 TFT cannot display 256x288 fingerprint).

    To function properly, div must be a factor of both IMAGE_WIDTH*mul and IMAGE_HEIGHT*mul.
*/
#define IMAGE_SCALING_MULT  3
#define IMAGE_SCALING_DIV   4

/* Dark green for the background */
const uint16_t BG_COLOUR = tft.color565(0, 25, 0);

/*  Define a special kind of Stream which writes to the TFT directly,
    as each chunk/packet of image data is read from the sensor. */
    
class TftStream : public Stream
{
    public:

        TftStream(bool raw) : startx(0), starty(0),
                              canvasWidth(0), canvasHeight(0), 
                              pixelCount(0), rawMode(raw)
        {

        }

        void resetCanvas(void)
        {
            /* Clear the existing canvas, if any */
            if (canvasWidth != 0 && canvasHeight != 0)
            {
                tft.fillRect(startx, starty, canvasWidth, canvasHeight, BG_COLOUR);
            }

            /* Setup the bounds of the canvas. Make sure to center the image. */
            canvasWidth = IMAGE_WIDTH * IMAGE_SCALING_MULT / IMAGE_SCALING_DIV;
            canvasHeight = IMAGE_HEIGHT * IMAGE_SCALING_MULT / IMAGE_SCALING_DIV;
            
            startx = (TFT_WIDTH / 2) - (canvasWidth / 2);
            starty = (TFT_HEIGHT / 2) - (canvasHeight / 2);

            pixelCount = 0;

            tft.fillRect(startx, starty, canvasWidth, canvasHeight, BG_COLOUR);
            tft.setAddrWindow(startx, starty, canvasWidth, canvasHeight);
        }

        uint16_t bmpToTftColour(uint8_t bmp_colour)
        {
            /* In raw mode, R, G, B have equal components, as in the original 8-bit grayscale. */
            if (rawMode)
            {
                return tft.color565(bmp_colour, bmp_colour, bmp_colour);
            }

            /* Otherwise, use only shades of green to display the print.
             * Apply a threshold for contrast, to make it look nicer. Adjust as you like. */
            #define MAX_8BIT_GRAYSCALE      255
            #define BACKGROUND_THRESHOLD    180
            
            if (bmp_colour > BACKGROUND_THRESHOLD)
            {
                return BG_COLOUR;
            }
            else
            {
                return tft.color565(0, MAX_8BIT_GRAYSCALE - bmp_colour, 0);
            }
        }
        
        size_t write(const uint8_t *chunk, size_t chunkLen)
        {            
            for (int i = 0; i < chunkLen; i++)
            {
                /*  Convert the colour in grayscale to RGB565. */
                uint16_t colour = bmpToTftColour(chunk[i]);

                /* According to the datasheets, each received byte is a lossily-compressed value,
                 * holding part of the colour information for 2 adjacent pixels in the same row.
                 * i.e. Upper nibble = Upper nibble of Pixel X, Lower nibble = Lower nibble of Pixel X + 1,
                 * where X must be even in the range [0, IMAGE_WIDTH).

                 * So, for simplicity, assume that both pixels were originally close enough in colour,
                 * to now be assigned the same colour.

                /* Push Pixel X, after applying scaling. */
                uint16_t row = pixelCount / IMAGE_WIDTH;
                uint16_t column = pixelCount % IMAGE_WIDTH;

                if ((row % IMAGE_SCALING_DIV) < IMAGE_SCALING_MULT && (column % IMAGE_SCALING_DIV) < IMAGE_SCALING_MULT)
                {
                    tft.pushColor(colour);
                }

                pixelCount++;

                /* Push Pixel X+1, after applying scaling. */
                row = pixelCount / IMAGE_WIDTH;
                column = pixelCount % IMAGE_WIDTH;
                if ((row % IMAGE_SCALING_DIV) < IMAGE_SCALING_MULT && (column % IMAGE_SCALING_DIV) < IMAGE_SCALING_MULT)
                {
                    tft.pushColor(colour);
                }

                pixelCount++;
            }

            return chunkLen;
        }

        /* Dummy implementations to satisfy compiler */
        size_t write(uint8_t n) { return 0; }
        int available(void) { return 0; }
        int read(void) { return 0; }
        int peek(void) { return 0; }

    private:

        int startx;
        int starty;
        uint16_t canvasWidth;
        uint16_t canvasHeight;
        uint32_t pixelCount;
        bool rawMode;
};

/*  Create the TFT stream object.
 *  In raw mode, the fingerprint is displayed in grayscale, roughly as received from the sensor (after any scaling).
 *  Raw mode is disabled here, by default.
 */
TftStream tftStream(false);

void setup()
{
    Serial.begin(57600);
    
#if defined(ARDUINO_ARCH_ESP32)
    fserial.begin(57600, SERIAL_8N1, 25, 32);
#else
    fserial.begin(57600);
#endif

    Serial.println("IMAGE-TO-TFT example");

    if (finger.begin()) {
        finger.readParams(&params);
        Serial.println("Found fingerprint sensor!");
        Serial.print("Capacity: "); Serial.println(params.capacity);
        Serial.print("Packet length: "); Serial.println(FPM::packetLengths[static_cast<uint8_t>(params.packetLen)]);
    }
    else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
    
    /* TFT setup */
    tft.init();
    tft.setRotation(0); /* Portrait */

    tft.fillScreen(BG_COLOUR);
}

void loop()
{
    imageToTft();

    while (1) yield();
}

uint32_t imageToTft(void)
{
    FPMStatus status;

    /* Take a snapshot of the finger */
    Serial.println("\r\nPlace a finger.");

    do {
        status = finger.getImage();

        switch (status)
        {
            case FPMStatus::OK:
                Serial.println("Image taken.");
                break;

            case FPMStatus::NOFINGER:
                Serial.println(".");
                break;

            default:
                /* allow retries even when an error happens */
                snprintf(printfBuf, PRINTF_BUF_SZ, "getImage(): error 0x%X", static_cast<uint16_t>(status));
                Serial.println(printfBuf);
                break;
        }

        yield();
    }
    while (status != FPMStatus::OK);

    /* Prepare the canvas */
    tftStream.resetCanvas();

    /* Initiate the image transfer */
    status = finger.downloadImage();

    switch (status)
    {
        case FPMStatus::OK:
            Serial.println("Starting image stream...");
            break;

        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "downloadImage(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return 0;
    }
    
/* The total size (in bytes) of the image data, transferred by the sensor. */
#define IMAGE_SZ        36864UL

    uint32_t totalRead = 0;
    uint16_t readLen = IMAGE_SZ;

    /* Now, the sensor will send us the image from its image buffer, one packet at a time */
    bool readComplete = false;

    while (!readComplete)
    {
        /* The library will write the data directly into the TFT stream/display */
        bool ret = finger.readDataPacket(NULL, &tftStream, &readLen, &readComplete);

        if (!ret)
        {
            snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("readDataPacket(): failed after reading %u bytes"), totalRead);
            Serial.println(printfBuf);
            return 0;
        }

        totalRead += readLen;
        readLen = IMAGE_SZ - totalRead;

        yield();
    }
    
    Serial.println();
    Serial.print(totalRead); Serial.println(" bytes transferred.");
    return totalRead;
}
