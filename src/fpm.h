/*************************************************** 
  Library for the FPM10/R305/ZFM20 family of optical fingerprint sensors
  Originally based on the Adafruit R305 library https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library
  
  Copyright (c) 2023, Brian Ejike <bcejike@gmail.com>
  Distributed under the terms of the MIT license
 ****************************************************/
 
#ifndef FPM_H_
#define FPM_H_

#include <stdint.h>
#include <stddef.h>

/* R551 is different in a few ways (no high-speed search, off-by-one template indices)
   uncomment this line if you have one of those sensors. */
//#define FPM_R551_MODULE

/* signature and packet ids */
#define FPM_STARTCODE               0xEF01

#define FPM_COMMANDPACKET           0x1
#define FPM_DATAPACKET              0x2
#define FPM_ACKPACKET               0x7
#define FPM_ENDDATAPACKET           0x8

/* commands */
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
#define FPM_LEDCONTROL              0x35
#define FPM_GETIMAGE_ONLY           0x52
#define FPM_GETRANDOM               0x14

#define FPM_READPRODINFO            0x3C

enum class FPMStatus : uint16_t {
    /******* sensor status/confirmation codes ********/
    OK                  = 0x00,
    HANDSHAKE_OK        = 0x55,
    PACKETRECIEVEERR    = 0x01,
    NOFINGER            = 0x02,
    FPM_IMAGEFAIL       = 0x03,
    IMAGEMESS           = 0x06,
    FEATUREFAIL         = 0x07,
    NOMATCH             = 0x08,
    NOTFOUND            = 0x09,
    ENROLLMISMATCH      = 0x0A,
    BADLOCATION         = 0x0B,
    DBREADFAIL          = 0x0C,
    UPLOADFEATUREFAIL   = 0x0D,
    PACKETRESPONSEFAIL  = 0x0E,
    UPLOADFAIL          = 0x0F,
    DELETEFAIL          = 0x10,
    DBCLEARFAIL         = 0x11,
    PASSFAIL            = 0x13,
    INVALIDIMAGE        = 0x15,
    FLASHERR            = 0x18,
    INVALIDREG          = 0x1A,
    ADDRCODE            = 0x20,
    PASSVERIFY          = 0x21,
    
    /******* library status codes ********/
    
    /* successful operation code; only used internally */
    LIB_OK              = 0xFF00,
    /* returned upon timeouts */
    TIMEOUT             = 0xFF01,
    /* returned upon receiving an unexpected PID or length */
    READ_ERROR          = 0xFF02,
    /* returned whenever there's no free ID/slot in the sensor flash */
    NO_FREE_INDEX       = 0xFF03,
    /* returned for any invalid params */
    INVALID_PARAMS      = 0xFF04,
    
    /* end of library status codes */
    ERROR_END           = 0xFFF0
};

#define FPM_MAX_PACKET_LEN          256
#define FPM_PKT_OVERHEAD_LEN        12

/* Length in bytes of the system parameters read from the sensor */
#define FPM_SYS_PARAMS_LEN          16

/* default timeout for reading responses/data */
#define FPM_DEFAULT_TIMEOUT         2000

/* max number of templates in each "page" returned by FPM_READTEMPLATEINDEX command */
#define FPM_TEMPLATES_PER_PAGE      256

/* default address and password, common to most if not all sensors */
#define FPM_DEFAULT_PASSWORD        0x00000000
#define FPM_DEFAULT_ADDRESS         0xFFFFFFFF

/* use these constants when setting system 
 * parameters with the setParam() method */
enum class FPMParameter {
    BAUD_RATE = 4,
    SECURITY_LEVEL,
    PACKET_LENGTH
};

/* baud rates */
enum class FPMBaud : uint16_t {
    B9600 = 1,
    B19200,
    B28800,
    B38400,
    B48000,
    B57600,
    B67200,
    B76800,
    B86400,
    B96000,
    B105600,
    B115200
};

/* security levels */
enum class FPMSecurityLevel : uint16_t {
    FRR_1 = 1,
    FRR_2,
    FRR_3,
    FRR_4,
    FRR_5
};

/* packet lengths */
enum class FPMPacketLength : uint16_t {
    PLEN_32,
    PLEN_64,
    PLEN_128,
    PLEN_256
};
 
typedef struct {
    uint16_t statusReg;
    uint16_t systemId;
    uint16_t capacity;
    FPMSecurityLevel securityLevel;
    uint32_t deviceAddr;
    FPMPacketLength packetLen;
    FPMBaud baudRate;
} FPMSystemParams;

/* Max lengths of each string field in the Product Info */
#define FPM_PRODUCT_INFO_MODULE_MODEL_LEN       16
#define FPM_PRODUCT_INFO_BATCH_NUMBER_LEN       4
#define FPM_PRODUCT_INFO_SERIAL_NUMBER_LEN      8
#define FPM_PRODUCT_INFO_SENSOR_MODEL_LEN       8

/* This should mirror the size of the FPMProductInfo struct, excluding the NUL-terminator bytes */
#define FPM_PRODUCT_INFO_LEN                    (FPM_PRODUCT_INFO_MODULE_MODEL_LEN + FPM_PRODUCT_INFO_BATCH_NUMBER_LEN + \
                                                 FPM_PRODUCT_INFO_SERIAL_NUMBER_LEN + FPM_PRODUCT_INFO_SENSOR_MODEL_LEN + \
                                                 2 + 2 + 2 + 2 + 2)
                                                 
typedef struct {
    char moduleModel[FPM_PRODUCT_INFO_MODULE_MODEL_LEN+1];
    char batchNumber[FPM_PRODUCT_INFO_BATCH_NUMBER_LEN+1];
    char serialNumber[FPM_PRODUCT_INFO_SERIAL_NUMBER_LEN+1];
    
    struct {
        uint8_t major;
        uint8_t minor;
    } hwVersion;
    
    char sensorModel[FPM_PRODUCT_INFO_SENSOR_MODEL_LEN+1];
    uint16_t imageWidth;
    uint16_t imageHeight;
    uint16_t templateSize;
    uint16_t databaseSize;
} FPMProductInfo;

/* Size of the general-purpose buffer used internally.
 * FPM_PRODUCT_INFO_LEN is the max payload length for ACKed commands, +1 for confirmation code */
#define FPM_BUFFER_SZ               (FPM_PRODUCT_INFO_LEN + 1)

/* Default parameters to be used with R308 (and similar)

   statusReg: 0x0000,
   systemId: 0x0000,
   capacity: <Your-module-capacity>,
   securityLevel: FPMSecurityLevel::FRR_5,
   deviceAddr: 0xFFFFFFFF,
   packetLen: FPMPacketLength::PLEN_128,
   baudRate: FPMBaud::B57600
   
*/
 
class Stream;

class FPM 
{
    public:
    FPM(Stream * ss);
    
    /** #params argument is only for R308 sensors that must be set manually. 
        Make sure to use the defaults listed above -- only capacity and packet length are actually relevant */
    bool begin(uint32_t password = FPM_DEFAULT_PASSWORD, uint32_t address = FPM_DEFAULT_ADDRESS, FPMSystemParams * params = NULL);

    bool verifyPassword(uint32_t pwd);
    FPMStatus getImage(void);
    FPMStatus image2Tz(uint8_t slot = 1);
    FPMStatus generateTemplate(void);

    FPMStatus emptyDatabase(void);
    FPMStatus storeTemplate(uint16_t id, uint8_t slot = 1);
    
    /** loads template with ID #id from the database into buffer #slot */
    FPMStatus loadTemplate(uint16_t id, uint8_t slot = 1);
    
    FPMStatus setBaudRate(FPMBaud baudRate);
    FPMStatus setSecurityLevel(FPMSecurityLevel securityLevel);
    FPMStatus setPacketLength(FPMPacketLength packetLen);
    
    /** Read the current System Parameters from the sensor, into #params.
     * Some sensors do not support this, such as the R308. */
    FPMStatus readParams(FPMSystemParams * params = NULL);
    
    /** Read the Product Information from the sensor into #info. String fields are already NUL-terminated.
     *  Only the R503 sensor is known to support this, so far. */
    FPMStatus readProductInfo(FPMProductInfo * info);
    
    /* initiate transfer of the fingerprint image from the sensor to the Arduino */
    FPMStatus downloadImage(void);
    
    /* for reading and writing data packets from/to sensor */
    bool readDataPacket(uint8_t * destBuffer, Stream * destStream, uint16_t * readLen, bool * readComplete);
    bool writeDataPacket(uint8_t * srcBuffer, Stream * srcStream, uint16_t * writeLen, bool writeComplete);
    
    /** initiates the transfer of a template in buffer #slot to the MCU */
    FPMStatus downloadTemplate(uint8_t slot = 1);
    
    /** initiates the transfer of a template from the MCU to buffer #slot */
    FPMStatus uploadTemplate(uint8_t slot = 1);
    FPMStatus deleteTemplate(uint16_t id, uint16_t howMany = 1);
    FPMStatus searchDatabase(uint16_t * finger_id, uint16_t * score, uint8_t slot = 1);
    FPMStatus getTemplateCount(uint16_t * template_cnt);
    FPMStatus getFreeIndex(uint8_t page, int16_t * id);
    FPMStatus matchTemplatePair(uint16_t * score);
    FPMStatus setPassword(uint32_t pwd);
    FPMStatus setAddress(uint32_t addr);
    FPMStatus getRandomNumber(uint32_t * number);

    /* these 3 have been tested successfully only on ZFM60 so far. 
       May yet work on other/newer sensors */
    FPMStatus ledOn(void);
    FPMStatus ledOff(void);
    FPMStatus getImageOnly(void);

    /* tested on R503 sensors */
    FPMStatus ledConfigure(uint8_t controlCode, uint8_t speed, uint8_t colour, uint8_t numCycles);
    
    /* tested on R551 sensors (by user [xsrf]),
       standby current measured at 10uA, UART and LEDs turned off,
       no other documentation available */
    FPMStatus standby(void);

    /* Returns true if the sensor is functional.
     * Supported by Z70 at least */
    bool handshake(void);
    
    static const uint16_t packetLengths[];
        
    private:
    uint8_t buffer[FPM_BUFFER_SZ];
    Stream * port;
    uint32_t password;
    uint32_t address;
    
    FPMSystemParams sysParams;
    bool useFixedParams;
    
    /**
     *   @brief         Send a simple packet to the sensor.
                                
     *   @param[in]     srcBuffer   A buffer holding the packet to be written
     *   @param[inout]  writeLen    The length of the packet to be written.                
     *   @param[in]     pktId       The Packet ID
     
     *   @return        void
     */
     
    void writePacket(uint8_t pktId, uint8_t * srcBuffer, uint16_t writeLen);
    /**
     *   @brief         Send a packet (COMMAND or DATA) to the sensor.
                                
     *   @param[in]     srcBuffer   A buffer holding the packet to be written
     *   @param[in]     srcStream   A Stream holding the packet to be written
     *   @param[inout]  writeLen    A pointer to the length of the packet to be written.
                                    At the end of a successful write, it holds the length actually written.                  
     *   @param[in]     pktId       The Packet ID; usually COMMAND or *DATA
     
     *   @return                    If successful, LIB_OK. Else, an error code.
     */
    FPMStatus writePacket(uint8_t * srcBuffer, Stream * srcStream, uint16_t * writeLen, uint8_t pktId);
    /**
     *   @brief         Read a packet (ACK or DATA) and after parsing its header,
                        copy the payload into the supplied buffer or write it directly to the supplied Stream.
                        
     *   @param[out]    destBuffer  The buffer in which to store the received payload. 
                                    Should be NULL if a Stream has been provided instead.
                                    
     *   @param[out]    destStream  A Stream to which the received payload should be written.
                                    Should be NULL if a buffer has been provided instead.
                                
     *   @param[inout]  readLen     When it's not NULL, it's a pointer to the max length
                                    to be written to either #destBuffer or #destStream.
                                    At the end of a successful read, it holds the payload length actually read.
                                    
     *   @param[out]    pktId       If successful, this holds the received Packet ID
     *   @return                    If successful, LIB_OK. Else, an error code.
     */
    FPMStatus readPacket(uint8_t * destBuffer, Stream * destStream, uint16_t * readLen, uint8_t * pktId); 
    
    /**
     *   @brief                         Read an ACK-packet from the sensor and return its confirmation code
     *   @param[out]    confirmCode     The ACK-packet confirmation code
     *   @param[out]    readLen         At the end of a successful read, it holds the payload length actually read, if any.
     *   @return                        If successful, LIB_OK. Else, an error code.
     */
    FPMStatus readAckGetResponse(FPMStatus * confirmCode, uint16_t * readLen); 
    
    /**
     *   @brief         Send a command from the pre-filled library buffer to the sensor, 
                        read the ACK-packet and return its confirmation code or a library error code upon failure
                        
     *   @param[in]     payloadLen  Length of the command-payload placed in the pre-filled library buffer
     *   @return                    If successful, LIB_OK. Else, an error code.
     */ 
    FPMStatus writeCommandGetResponse(uint16_t payloadLen);
    
    FPMStatus setParam(FPMParameter param, uint8_t value);
    
    static inline bool isErrorCode(FPMStatus status);
};

#endif
