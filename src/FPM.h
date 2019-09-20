/*************************************************** 
  This is an improved library for the FPM10/R305/ZFM20 optical fingerprint sensor
  Based on the Adafruit R305 library https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library
  
  Written by Brian Ejike <brianrho94@gmail.com> (2017)
  Distributed under the terms of the MIT license
 ****************************************************/
#ifndef FPM_H_
#define FPM_H_

#include <stdint.h>
#include <stddef.h>

/* R551 is different in a few ways (no high-speed search, off-by-one template indices)
   uncomment this line if you have one of those */
//#define FPM_R551_MODULE

/* Set the debug level
   0: Disabled
   1: Errors only
   2: Everything
 */
#define FPM_DEBUG_LEVEL             1

// confirmation codes
#define FPM_OK                      0x00
#define FPM_HANDSHAKE_OK            0x55
#define FPM_PACKETRECIEVEERR        0x01
#define FPM_NOFINGER                0x02
#define FPM_IMAGEFAIL               0x03
#define FPM_IMAGEMESS               0x06
#define FPM_FEATUREFAIL             0x07
#define FPM_NOMATCH                 0x08
#define FPM_NOTFOUND                0x09
#define FPM_ENROLLMISMATCH          0x0A
#define FPM_BADLOCATION             0x0B
#define FPM_DBREADFAIL              0x0C
#define FPM_UPLOADFEATUREFAIL       0x0D
#define FPM_PACKETRESPONSEFAIL      0x0E
#define FPM_UPLOADFAIL              0x0F
#define FPM_DELETEFAIL              0x10
#define FPM_DBCLEARFAIL             0x11
#define FPM_PASSFAIL                0x13
#define FPM_INVALIDIMAGE            0x15
#define FPM_FLASHERR                0x18
#define FPM_INVALIDREG              0x1A
#define FPM_ADDRCODE                0x20
#define FPM_PASSVERIFY              0x21

// signature and packet ids
#define FPM_STARTCODE               0xEF01

#define FPM_COMMANDPACKET           0x1
#define FPM_DATAPACKET              0x2
#define FPM_ACKPACKET               0x7
#define FPM_ENDDATAPACKET           0x8

// commands
#define FPM_GETIMAGE                0x01
#define FPM_IMAGE2TZ                0x02
#define FPM_REGMODEL                0x05
#define FPM_STORE                   0x06
#define FPM_LOAD                    0x07
#define FPM_UPCHAR                  0x08
#define FPM_DOWNCHAR                0x09
#define FPM_IMGUPLOAD               0x0A
#define FPM_DELETE                  0x0C
#define FPM_EMPTYDATABASE           0x0D
#define FPM_SETSYSPARAM             0x0E
#define FPM_READSYSPARAM            0x0F
#define FPM_VERIFYPASSWORD          0x13
#define FPM_SEARCH                  0x04
#define FPM_HISPEEDSEARCH           0x1B
#define FPM_TEMPLATECOUNT           0x1D
#define FPM_READTEMPLATEINDEX       0x1F
#define FPM_PAIRMATCH               0x03
#define FPM_SETPASSWORD             0x12
#define FPM_SETADDRESS              0x15
#define FPM_STANDBY                 0x33
#define FPM_HANDSHAKE               0x53

#define FPM_LEDON                   0x50
#define FPM_LEDOFF                  0x51
#define FPM_LED_CONTROL             0x35
#define FPM_GETIMAGE_NOLIGHT        0x52
#define FPM_GETRANDOM               0x14

/* returned whenever we time out while reading */
#define FPM_TIMEOUT                 -1
/* returned whenever we get an unexpected PID or length */
#define FPM_READ_ERROR              -2
/* returned whenever there's no free ID */
#define FPM_NOFREEINDEX             -1

#define FPM_MAX_PACKET_LEN          256
#define FPM_PKT_OVERHEAD_LEN        12

/* 32 is max packet length for ACKed commands, +1 for confirmation code */
#define FPM_BUFFER_SZ               (32 + 1)

/* default timeout is 2 seconds */
#define FPM_DEFAULT_TIMEOUT         2000
#define FPM_TEMPLATES_PER_PAGE      256

#define FPM_DEFAULT_PASSWORD        0x00000000
#define FPM_DEFAULT_ADDRESS         0xFFFFFFFF

/* use these constants when setting system 
 * parameters with the setParam() method */
enum {
    FPM_SETPARAM_BAUD_RATE = 4,
    FPM_SETPARAM_SECURITY_LEVEL,
    FPM_SETPARAM_PACKET_LEN
};

/* possible values for system parameters that can be set with setParam() */

/* baud rates */
enum {
    FPM_BAUD_9600 = 1,
    FPM_BAUD_19200,
    FPM_BAUD_28800,
    FPM_BAUD_38400,
    FPM_BAUD_48000,
    FPM_BAUD_57600,
    FPM_BAUD_67200,
    FPM_BAUD_76800,
    FPM_BAUD_86400,
    FPM_BAUD_96000,
    FPM_BAUD_105600,
    FPM_BAUD_115200
};

/* security levels */
enum {
    FPM_FRR_1 = 1,
    FPM_FRR_2,
    FPM_FRR_3,
    FPM_FRR_4,
    FPM_FRR_5
};

/* packet lengths */
enum {
    FPM_PLEN_32,
    FPM_PLEN_64,
    FPM_PLEN_128,
    FPM_PLEN_256,
    FPM_PLEN_NONE = 0xff
};

/* possible output containers for template/image data read from the module */
enum {
    FPM_OUTPUT_TO_STREAM,
    FPM_OUTPUT_TO_BUFFER
};
 
typedef struct {
    uint16_t status_reg;
    uint16_t system_id;
    uint16_t capacity;
    uint16_t security_level;
    uint32_t device_addr;
    uint16_t packet_len;
    uint16_t baud_rate;
} FPM_System_Params;

/* Default parameters to be used with R308 (and similar)

   status_reg: 0x0000,
   system_id: 0x0000,
   capacity: <Your-module-capacity>,
   security_level: FPM_FRR_5,
   device_addr: 0xFFFFFFFF,
   packet_len: FPM_PLEN_128,
   baud_rate: FPM_BAUD_57600
   
 */
 
class Stream;

class FPM {
    public:
        FPM(Stream * ss);
        
        /* 'params' argument is for R308 sensors that must be set manually, make sure to use the defaults above,
           only capacity and packet length are actually relevant */
        bool begin(uint32_t password=FPM_DEFAULT_PASSWORD, uint32_t address=FPM_DEFAULT_ADDRESS, FPM_System_Params * params = NULL);

        bool verifyPassword(uint32_t pwd);
        int16_t getImage(void);
        int16_t image2Tz(uint8_t slot = 1);
        int16_t createModel(void);

        int16_t emptyDatabase(void);
        int16_t storeModel(uint16_t id, uint8_t slot = 1);
        
        /* loads template with ID #'id' from the database into buffer #'slot' */
        int16_t loadModel(uint16_t id, uint8_t slot = 1);
        int16_t setParam(uint8_t param, uint8_t value);
        int16_t readParams(FPM_System_Params * params = NULL);
        
        /* initiate transfer of fingerprint image to host */
        int16_t downImage(void);
        
        /* for reading and writing data packets from/to sensor */
        bool readRaw(uint8_t outType, void * out, bool * read_complete, uint16_t * read_len = NULL);
        void writeRaw(uint8_t * data, uint16_t len);
        
        /* initiates the transfer of a template in buffer #'slot' to the MCU */
        int16_t downloadModel(uint8_t slot = 1);
        
        /* initiates the transfer of a template from the MCU to buffer #'slot' */
        int16_t uploadModel(uint8_t slot = 1);
        int16_t deleteModel(uint16_t id, uint16_t how_many = 1);
        int16_t searchDatabase(uint16_t * finger_id, uint16_t * score, uint8_t slot = 1);
        int16_t getTemplateCount(uint16_t * template_cnt);
        int16_t getFreeIndex(uint8_t page, int16_t * id);
        int16_t matchTemplatePair(uint16_t * score);
        int16_t setPassword(uint32_t pwd);
        int16_t setAddress(uint32_t addr);
        int16_t getRandomNumber(uint32_t * number);

        /* these 3 work only on ZFM60 so far */
        int16_t led_on(void);
        int16_t led_off(void);
        int16_t getImageNL(void);
  
        /* tested on R503 sensors */
        int16_t led_control(uint8_t control_code, uint8_t speed, uint8_t color_idx, uint8_t times);
        
        /* tested on R551 sensors (by user [xsrf]),
           standby current measured at 10uA, UART and LEDs turned off,
           no other documentation available */
        int16_t standby(void);

        /* NEW: found in Z70 sensor datasheet but not tested yet. 
           Should return true if the sensor is ready to accept commands */
        bool handshake(void);
        
        static const uint16_t packet_lengths[];
        
    private: 
        uint8_t buffer[FPM_BUFFER_SZ];
        Stream * port;
        uint32_t password;
        uint32_t address;
        
        FPM_System_Params sys_params;
        bool manual_settings;
        
        void writePacket(uint8_t packettype, uint8_t * packet, uint16_t len);
        int16_t getReply(uint8_t * replyBuf, uint16_t buflen, uint8_t * pktid, Stream * outStream = NULL);
        int16_t read_ack_get_response(uint8_t * rc);
};

#endif
