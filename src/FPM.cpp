/*************************************************** 
  This is an improved library for the FPM10/R305/ZFM20 optical fingerprint sensor
  Based on the Adafruit R305 library https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library
  
  Written by Brian Ejike <brianrho94@gmail.com> (2017)
  Distributed under the terms of the MIT license
 ****************************************************/

#include <Arduino.h>
#include "FPM.h"

#if (FPM_DEBUG_LEVEL == 0)
    
    #define FPM_INFO_PRINT(x)
    #define FPM_INFO_PRINTLN(x)
    #define FPM_INFO_DEC(x)
    #define FPM_INFO_DECLN(x)
    #define FPM_INFO_HEX(x)
    #define FPM_INFO_HEXLN(x)
    
    #define FPM_ERROR_PRINT(x)
    #define FPM_ERROR_PRINTLN(x)
    #define FPM_ERROR_DEC(x)
    #define FPM_ERROR_DECLN(x)
    #define FPM_ERROR_HEX(x)
    #define FPM_ERROR_HEXLN(x)
    
#else
    
    #define FPM_DEFAULT_STREAM          Serial
    
    #define FPM_ERROR_PRINT(x)          FPM_DEFAULT_STREAM.print(x)
    #define FPM_ERROR_PRINTLN(x)        FPM_DEFAULT_STREAM.println(x)
    #define FPM_ERROR_DEC(x)            FPM_DEFAULT_STREAM.print(x)
    #define FPM_ERROR_DECLN(x)          FPM_DEFAULT_STREAM.println(x)
    #define FPM_ERROR_HEX(x)            FPM_DEFAULT_STREAM.print(x, HEX)
    #define FPM_ERROR_HEXLN(x)          FPM_DEFAULT_STREAM.println(x, HEX)
    
    #if (FPM_DEBUG_LEVEL == 1)        
        #define FPM_INFO_PRINT(x)
        #define FPM_INFO_PRINTLN(x)
        #define FPM_INFO_DEC(x)
        #define FPM_INFO_DECLN(x)
        #define FPM_INFO_HEX(x)
        #define FPM_INFO_HEXLN(x)
    #else
        #define FPM_INFO_PRINT(x)           FPM_DEFAULT_STREAM.print(x)
        #define FPM_INFO_PRINTLN(x)         FPM_DEFAULT_STREAM.println(x)
        #define FPM_INFO_DEC(x)             FPM_DEFAULT_STREAM.print(x)
        #define FPM_INFO_DECLN(x)           FPM_DEFAULT_STREAM.println(x)
        #define FPM_INFO_HEX(x)             FPM_DEFAULT_STREAM.print(x, HEX)
        #define FPM_INFO_HEXLN(x)           FPM_DEFAULT_STREAM.println(x, HEX)
    #endif

#endif

typedef enum {
    FPM_STATE_READ_HEADER,
    FPM_STATE_READ_ADDRESS,
    FPM_STATE_READ_PID,
    FPM_STATE_READ_LENGTH,
    FPM_STATE_READ_CONTENTS,
    FPM_STATE_READ_CHECKSUM
} FPM_State;

const uint16_t FPM::packet_lengths[] = {32, 64, 128, 256};

FPM::FPM(Stream * ss) : 
    port(ss), password(0), address(0xFFFFFFFF)
{
    
}

bool FPM::begin(uint32_t pwd, uint32_t addr, FPM_System_Params * params) {
    delay(1000);            // 500 ms at least according to datasheet
    
    address = addr;
    
    if(!verifyPassword(pwd)) return false;

    manual_settings = false;
    if (params != NULL) {
        manual_settings = true;
        memcpy(&sys_params, params, 16);
    }
    else if (readParams() != FPM_OK) {
        FPM_ERROR_PRINTLN("[+]Read params failed.");
        return false;
    }
    
    return true;
}

bool FPM::verifyPassword(uint32_t pwd) {
    password = pwd;
    
    buffer[0] = FPM_VERIFYPASSWORD;
    buffer[1] = (password >> 24) & 0xff; buffer[2] = (password >> 16) & 0xff;
    buffer[3] = (password >> 8) & 0xff; buffer[4] = password & 0xff;
    writePacket(FPM_COMMANDPACKET, buffer, 5);
    uint8_t confirm_code = 0;
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0 || confirm_code != FPM_OK)
        return false;
    
    return true;
}

int16_t FPM::setPassword(uint32_t pwd) {
    buffer[0] = FPM_SETPASSWORD;
    buffer[1] = (pwd >> 24) & 0xff; buffer[2] = (pwd >> 16) & 0xff;
    buffer[3] = (pwd >> 8) & 0xff; buffer[4] = pwd & 0xff;
    
    writePacket(FPM_COMMANDPACKET, buffer, 5);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

int16_t FPM::setAddress(uint32_t addr) {
    buffer[0] = FPM_SETADDRESS;
    buffer[1] = (addr >> 24) & 0xff; buffer[2] = (addr >> 16) & 0xff;
    buffer[3] = (addr >> 8) & 0xff; buffer[4] = addr & 0xff;
    
    writePacket(FPM_COMMANDPACKET, buffer, 5);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

int16_t FPM::getImage(void) {
    buffer[0] = FPM_GETIMAGE;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

/* tested with ZFM60 modules only */
int16_t FPM::getImageNL(void) {
    buffer[0] = FPM_GETIMAGE_NOLIGHT;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

/* tested with ZFM60 modules only */
int16_t FPM::led_on(void) {
    buffer[0] = FPM_LEDON;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

/* tested with ZFM60 modules only */
int16_t FPM::led_off(void) {
    buffer[0] = FPM_LEDOFF;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}
/* tested with R503 modules only */
int16_t FPM::led_control(uint8_t control_code, uint8_t speed, uint8_t color_idx, uint8_t times) {
    buffer[0] = FPM_LED_CONTROL;
    buffer[1] = control_code;
    buffer[2] = speed;
    buffer[3] = color_idx;
    buffer[4] = times;
    
    writePacket(FPM_COMMANDPACKET, buffer, 5);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);

    if (rc < 0)
        return rc;

    return confirm_code;
}    

int16_t FPM::standby(void) {
    buffer[0] = FPM_STANDBY;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

int16_t FPM::image2Tz(uint8_t slot) {
    buffer[0] = FPM_IMAGE2TZ; 
    buffer[1] = slot;
    writePacket(FPM_COMMANDPACKET, buffer, 2);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}


int16_t FPM::createModel(void) {
    buffer[0] = FPM_REGMODEL;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}


int16_t FPM::storeModel(uint16_t id, uint8_t slot) {
    buffer[0] = FPM_STORE;
    buffer[1] = slot;
    buffer[2] = id >> 8; buffer[3] = id & 0xFF;
    
    writePacket(FPM_COMMANDPACKET, buffer, 4);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

int16_t FPM::loadModel(uint16_t id, uint8_t slot) {
    buffer[0] = FPM_LOAD;
    buffer[1] = slot;
    buffer[2] = id >> 8; buffer[3] = id & 0xFF;
    
    writePacket(FPM_COMMANDPACKET, buffer, 4);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

int16_t FPM::setParam(uint8_t param, uint8_t value) {
    if (manual_settings) {
        return FPM_PACKETRECIEVEERR;
    }
    
	buffer[0] = FPM_SETSYSPARAM;
    buffer[1] = param; buffer[2] = value;
    
	writePacket(FPM_COMMANDPACKET, buffer, 3);
    uint8_t confirm_code = 0;
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0)
        return len;
    
    if (confirm_code != FPM_OK)
        return confirm_code;
    
    delay(100);
    readParams();
    return confirm_code;
}

static void reverse_bytes(void *start, uint16_t size) {
    uint8_t *lo = (uint8_t *)start;
    uint8_t *hi = (uint8_t *)start + size - 1;
    uint8_t swap;
    while (lo < hi) {
        swap = *lo;
        *lo++ = *hi;
        *hi-- = swap;
    }
}

int16_t FPM::readParams(FPM_System_Params * user_params) {
    if (manual_settings) {
        if (user_params != NULL)
            memcpy(user_params, &sys_params, 16);
        return FPM_OK;
    }
    
    buffer[0] = FPM_READSYSPARAM;
    
	writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0)
        return len;
    
    if (confirm_code != FPM_OK)
        return confirm_code;
    
    if (len != 16) {
        FPM_ERROR_PRINT("[+]Unexpected params length: ");
        FPM_ERROR_DECLN(len);
        return FPM_READ_ERROR;
    }
    
    memcpy(&sys_params, &buffer[1], 16);
    reverse_bytes(&sys_params.status_reg, 2);
    reverse_bytes(&sys_params.system_id, 2);
    reverse_bytes(&sys_params.capacity, 2);
    reverse_bytes(&sys_params.security_level, 2);
    reverse_bytes(&sys_params.device_addr, 4);
    reverse_bytes(&sys_params.packet_len, 2);
    reverse_bytes(&sys_params.baud_rate, 2);
    
    if (user_params != NULL)
        memcpy(user_params, &sys_params, 16);
    
    return confirm_code;
}

int16_t FPM::downImage(void) {
	buffer[0] = FPM_IMGUPLOAD;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);

    if (rc < 0)
        return rc;

    return confirm_code;
}

/* Can read into an array or into a Stream object */
bool FPM::readRaw(uint8_t outType, void * out, bool * read_complete, uint16_t * read_len) {
    Stream * outStream;
    uint8_t * outBuf;
    
    if (outType == FPM_OUTPUT_TO_BUFFER)
        outBuf = (uint8_t *)out;
    else if (outType == FPM_OUTPUT_TO_STREAM)
        outStream = (Stream *)out;
    else
        return false;
    
    uint8_t pid;
    int16_t len;
    
    if (outType == FPM_OUTPUT_TO_BUFFER)
        len = getReply(outBuf, *read_len, &pid);
    else if (outType == FPM_OUTPUT_TO_STREAM)
        len = getReply(NULL, 0, &pid, outStream);
    
    /* check that the length is > 0 */
    if (len <= 0) {
        FPM_ERROR_PRINT("[+]Wrong read length: ");
        FPM_ERROR_DECLN(len);
        return false;
    }
    
    *read_complete = false;
    
    if (pid == FPM_DATAPACKET || pid == FPM_ENDDATAPACKET) {
        if (outType == FPM_OUTPUT_TO_BUFFER)
            *read_len = len;
        if (pid == FPM_ENDDATAPACKET)
            *read_complete = true;
        return true;
    }
    
    return false;
}

void FPM::writeRaw(uint8_t * data, uint16_t len) {
    uint16_t written = 0;
    uint16_t chunk_sz = packet_lengths[sys_params.packet_len];
    
    while (len > chunk_sz) {
        writePacket(FPM_DATAPACKET, &data[written], chunk_sz);
        written += chunk_sz;
        len -= chunk_sz;
    }
    writePacket(FPM_ENDDATAPACKET, &data[written], len);
}

int16_t FPM::downloadModel(uint8_t slot) {
    buffer[0] = FPM_UPCHAR;
    buffer[1] = slot;
    writePacket(FPM_COMMANDPACKET, buffer, 2);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);

    if (rc < 0)
        return rc;

    return confirm_code;
}

int16_t FPM::uploadModel(uint8_t slot) {
    buffer[0] = FPM_DOWNCHAR;
    buffer[1] = slot;
    writePacket(FPM_COMMANDPACKET, buffer, 2);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);

    if (rc < 0)
        return rc;

    return confirm_code;
}
    
int16_t FPM::deleteModel(uint16_t id, uint16_t num_to_delete) {
    buffer[0] = FPM_DELETE;
    buffer[1] = id >> 8; buffer[2] = id & 0xFF;
    buffer[3] = num_to_delete >> 8; buffer[4] = num_to_delete & 0xFF;
    writePacket(FPM_COMMANDPACKET, buffer, 5);
    
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);

    if (rc < 0)
        return rc;

    return confirm_code;
}

int16_t FPM::emptyDatabase(void) {
    buffer[0] = FPM_EMPTYDATABASE;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);

    if (rc < 0)
        return rc;

    return confirm_code;
}

int16_t FPM::searchDatabase(uint16_t * finger_id, uint16_t * score, uint8_t slot) {
    /* search from ID 0 to 'capacity' */
    buffer[0] = FPM_SEARCH;
    buffer[1] = slot;
    buffer[2] = 0x00; buffer[3] = 0x00;
    buffer[4] = (uint8_t)(sys_params.capacity >> 8); 
    buffer[5] = (uint8_t)(sys_params.capacity & 0xFF);
    
    writePacket(FPM_COMMANDPACKET, buffer, 6);
    uint8_t confirm_code = 0;
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0)
        return len;
    
    if (confirm_code != FPM_OK)
        return confirm_code;
    
    if (len != 4)
        return FPM_READ_ERROR;

    *finger_id = buffer[1];
    *finger_id <<= 8;
    *finger_id |= buffer[2];

    *score = buffer[3];
    *score <<= 8;
    *score |= buffer[4];

    return confirm_code;
}

int16_t FPM::matchTemplatePair(uint16_t * score) {
    buffer[0] = FPM_PAIRMATCH;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0)
        return len;
    
    if (confirm_code != FPM_OK)
        return confirm_code;
    
    if (len != 2)
        return FPM_READ_ERROR;
    
    *score = buffer[1]; 
    *score <<= 8;
    *score |= buffer[2];

    return confirm_code;
}

int16_t FPM::getTemplateCount(uint16_t * template_cnt) {
    buffer[0] = FPM_TEMPLATECOUNT;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0)
        return len;
    
    if (confirm_code != FPM_OK)
        return confirm_code;
    
    if (len != 2)
        return FPM_READ_ERROR;
    
    *template_cnt = buffer[1];
    *template_cnt <<= 8;
    *template_cnt |= buffer[2];

    return confirm_code;
}

int16_t FPM::getFreeIndex(uint8_t page, int16_t * id) {
    buffer[0] = FPM_READTEMPLATEINDEX; 
    buffer[1] = page;
    
    writePacket(FPM_COMMANDPACKET, buffer, 2);
    uint8_t confirm_code = 0;
    
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0)
        return len;
    
    if (confirm_code != FPM_OK)
        return confirm_code;
    
    for (int group_idx = 0; group_idx < len; group_idx++) {
        uint8_t group = buffer[1 + group_idx];
        if (group == 0xff)        /* if group is all occupied */
            continue;
        
        for (uint8_t bit_mask = 0x01, fid = 0; bit_mask != 0; bit_mask <<= 1, fid++) {
            if ((bit_mask & group) == 0) {
                #if defined(FPM_R551_MODULE)
                if (page == 0 && group_idx == 0 && fid == 0)     /* Skip LSb of first group */
                    continue;
                *id = (FPM_TEMPLATES_PER_PAGE * page) + (group_idx * 8) + fid - 1;      /* all IDs are off by one */
                #else
                *id = (FPM_TEMPLATES_PER_PAGE * page) + (group_idx * 8) + fid;
                #endif
                return confirm_code;
            }
        }
    }
    
    *id = FPM_NOFREEINDEX;  // no free space found
    return confirm_code;
}

int16_t FPM::getRandomNumber(uint32_t * number) {
    buffer[0] = FPM_GETRANDOM;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0)
        return len;
    
    if (confirm_code != FPM_OK)
        return confirm_code;
    
    if (len != 4)
        return FPM_READ_ERROR;
    
    *number = buffer[1]; 
    *number <<= 8;
    *number |= buffer[2];
    *number <<= 8;
    *number |= buffer[3];
    *number <<= 8;
    *number |= buffer[4];

    return confirm_code;
}

bool FPM::handshake(void) {
    buffer[0] = FPM_HANDSHAKE;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);

    if (rc < 0)
        return false;

    return confirm_code == FPM_HANDSHAKE_OK;
}

void FPM::writePacket(uint8_t packettype, uint8_t * packet, uint16_t len) {
    len += 2;
    
    port->write((uint8_t)(FPM_STARTCODE >> 8));
    port->write((uint8_t)FPM_STARTCODE);
    port->write((uint8_t)(address >> 24));
    port->write((uint8_t)(address >> 16));
    port->write((uint8_t)(address >> 8));
    port->write((uint8_t)(address));
    port->write((uint8_t)packettype);
    port->write((uint8_t)(len >> 8));
    port->write((uint8_t)(len));
  
    uint16_t sum = (len>>8) + (len&0xFF) + packettype;
    for (uint8_t i=0; i< len-2; i++) {
        port->write((uint8_t)(packet[i]));
        sum += packet[i];
    }
    port->write((uint8_t)(sum>>8));
    port->write((uint8_t)sum);
}

int16_t FPM::getReply(uint8_t * replyBuf, uint16_t buflen, uint8_t * pktid, Stream * outStream) {
    FPM_State state = FPM_STATE_READ_HEADER;
    
    uint16_t header = 0;
    uint8_t pid = 0;
    uint16_t length = 0;
    uint16_t chksum = 0;
    uint16_t remn = 0;
    
    uint32_t last_read = millis();
    
    while ((uint32_t)(millis() - last_read) < FPM_DEFAULT_TIMEOUT) {
        yield();
        
        switch (state) {
            case FPM_STATE_READ_HEADER: {
                if (port->available() == 0)
                    continue;
                
                last_read = millis();
                uint8_t byte = port->read();
                header <<= 8; header |= byte;
                if (header != FPM_STARTCODE)
                    break;
                
                state = FPM_STATE_READ_ADDRESS;
                header = 0;
                
                FPM_INFO_PRINTLN("\r\n[+]Got header");
                break;
            }
            case FPM_STATE_READ_ADDRESS: {
                if (port->available() < 4)
                    continue;
                
                last_read = millis();
                port->readBytes(buffer, 4);
                uint32_t addr = buffer[0]; addr <<= 8; 
                addr |= buffer[1]; addr <<= 8;
                addr |= buffer[2]; addr <<= 8;
                addr |= buffer[3];
                
                if (addr != address) {
                    state = FPM_STATE_READ_HEADER;
                    FPM_ERROR_PRINT("[+]Wrong address: 0x");
                    FPM_ERROR_HEXLN(addr);
                    break;
                }
                
                state = FPM_STATE_READ_PID;
                FPM_INFO_PRINT("[+]Address: 0x"); FPM_INFO_HEXLN(address);
                
                break;
            }
            case FPM_STATE_READ_PID:
                if (port->available() == 0)
                    continue;
                
                last_read = millis();
                pid = port->read();
                chksum = pid;
                *pktid = pid;
                
                state = FPM_STATE_READ_LENGTH;
                FPM_INFO_PRINT("[+]PID: 0x"); FPM_INFO_HEXLN(pid);
                
                break;
            case FPM_STATE_READ_LENGTH: {
                if (port->available() < 2)
                    continue;
                
                last_read = millis();
                port->readBytes(buffer, 2);
                length = buffer[0]; length <<= 8;
                length |= buffer[1];
                
                if (length > FPM_MAX_PACKET_LEN + 2 || (outStream == NULL && length > buflen + 2)) {
                    state = FPM_STATE_READ_HEADER;
                    FPM_ERROR_PRINT("[+]Packet too long: ");
                    FPM_ERROR_DECLN(length);
                    continue;
                }
                
                /* num of bytes left to read */
                remn = length;
                
                chksum += buffer[0]; chksum += buffer[1];
                state = FPM_STATE_READ_CONTENTS;
                FPM_INFO_PRINT("[+]Length: "); FPM_INFO_DECLN(length - 2);
                break;
            }
            case FPM_STATE_READ_CONTENTS: {
                if (remn <= 2) {
                    state = FPM_STATE_READ_CHECKSUM;
                    break;
                }
                
                if (port->available() == 0)
                    continue;
                
                last_read = millis();
                
                /* have to stop using 'buffer' since
                 * we may be storing data in it now */
                uint8_t byte = port->read();
                if (outStream != NULL) {
                    outStream->write(byte);
                }
                else {
                    *replyBuf++ = byte;
                }
                
                chksum += byte;
                
                FPM_INFO_HEX(byte); FPM_INFO_PRINT(" ");
                remn--;
                break;
            }
            case FPM_STATE_READ_CHECKSUM: {
                if (port->available() < 2)
                    continue;
                
                last_read = millis();
                uint8_t temp[2];
                port->readBytes(temp, 2);
                uint16_t to_check = temp[0]; to_check <<= 8;
                to_check |= temp[1];
                
                if (to_check != chksum) {
                    state = FPM_STATE_READ_HEADER;
                    FPM_ERROR_PRINT("\r\n[+]Wrong chksum: 0x");
                    FPM_ERROR_HEXLN(to_check);
                    continue;
                }
                
                FPM_INFO_PRINTLN("\r\n[+]Read complete");
                /* without chksum */
                return length - 2;
            }
        }
    }
    
    FPM_ERROR_PRINTLN("[+]Response timeout\r\n");
    return FPM_TIMEOUT;
}

/* read standard ACK-reply into library buffer and
 * return packet length and confirmation code */
int16_t FPM::read_ack_get_response(uint8_t * rc) {
    uint8_t pktid = 0;
    int16_t len = getReply(buffer, FPM_BUFFER_SZ, &pktid);
    
    /* most likely timed out */
    if (len < 0)
        return len;
    
    /* wrong pkt id */
    if (pktid != FPM_ACKPACKET) {
        FPM_ERROR_PRINT("[+]Wrong PID: 0x\r\n");
        FPM_ERROR_HEXLN(pktid);
        return FPM_READ_ERROR;
    }
    
    *rc = buffer[0];
    
    /* minus confirmation code */
    return --len;
}
