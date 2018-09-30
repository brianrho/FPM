/*************************************************** 
  This is an improved library for the FPM10/R305/ZFM20 optical fingerprint sensor
  Based on the Adafruit R305 library https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library
  
  Written by Brian Ejike <brianrho94@gmail.com> (2017)
  Distributed under the terms of the MIT license
 ****************************************************/

#include "FPM.h"

#if defined(FPM_ENABLE_DEBUG)
    #define FPM_DEFAULT_STREAM          Serial

    #define FPM_DEBUG_PRINT(x)          FPM_DEFAULT_STREAM.print(x)
    #define FPM_DEBUG_PRINTLN(x)        FPM_DEFAULT_STREAM.println(x)
    #define FPM_DEBUG_DEC(x)            FPM_DEFAULT_STREAM.print(x)
    #define FPM_DEBUG_DECLN(x)          FPM_DEFAULT_STREAM.println(x)
    #define FPM_DEBUG_HEX(x)            FPM_DEFAULT_STREAM.print(x, HEX)
    #define FPM_DEBUG_HEXLN(x)          FPM_DEFAULT_STREAM.println(x, HEX) 
#else
    #define FPM_DEBUG_PRINT(x)
    #define FPM_DEBUG_PRINTLN(x)
    #define FPM_DEBUG_DEC(x)          
    #define FPM_DEBUG_DECLN(x)
    #define FPM_DEBUG_HEX(x)
    #define FPM_DEBUG_HEXLN(x)
#endif

enum {
    FPM_STATE_READ_HEADER,
    FPM_STATE_READ_ADDRESS,
    FPM_STATE_READ_PID,
    FPM_STATE_READ_LENGTH,
    FPM_STATE_READ_CONTENTS,
    FPM_STATE_READ_CHECKSUM
} FPM_State;

static const uint16_t packet_lengths[] = {32, 64, 128, 256};

FPM::FPM(Stream * ss) : 
    port(ss), password(0), address(0xFFFFFFFF)
{
    
}

bool FPM::begin(uint32_t pwd, uint32_t addr) {
    delay(1000);            // 500 ms at least according to datasheet
    
    address = addr;
    password = pwd;
    
    buffer[0] = FPM_VERIFYPASSWORD;
    buffer[1] = (password >> 24) & 0xff; buffer[2] = (password >> 16) & 0xff;
    buffer[3] = (password >> 8) & 0xff; buffer[4] = password & 0xff;
    writePacket(FPM_COMMANDPACKET, buffer, 5);
    uint8_t confirm_code = 0;
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0 || confirm_code != FPM_OK)
        return false;

    if (readParams() != FPM_OK)
        return false;
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

int16_t FPM::getImage(void) {
    buffer[0] = FPM_GETIMAGE;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

// for ZFM60 modules
int16_t FPM::getImageNL(void) {
    buffer[0] = FPM_GETIMAGE_NOLIGHT;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

// for ZFM60 modules
int16_t FPM::led_on(void) {
    buffer[0] = FPM_LEDON;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);
    
    if (rc < 0)
        return rc;
    
    return confirm_code;
}

// for ZFM60 modules
int16_t FPM::led_off(void) {
    buffer[0] = FPM_LEDOFF;
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
    
//read a fingerprint template from flash into Char Buffer 1
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
    uint8_t *lo = start;
    uint8_t *hi = start + size - 1;
    uint8_t swap;
    while (lo < hi) {
        swap = *lo;
        *lo++ = *hi;
        *hi-- = swap;
    }
}

int16_t FPM::readParams(FPM_System_Params * user_params) {
    buffer[0] = FPM_READSYSPARAM;
    
	writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0)
        return len;
    
    if (confirm_code != FPM_OK)
        return confirm_code;
    
    if (len != 16)
        return FPM_READ_ERROR;
    
    memcpy(&sys_params, &buffer[1], 16);
    reverse_bytes(&sys_params.status_reg, 2);
    reverse_bytes(&sys_params.system_id, 2);
    reverse_bytes(&sys_params.capacity, 2);
    reverse_bytes(&sys_params.security_level, 2);
    reverse_bytes(&sys_params.device_addr, 4);
    reverse_bytes(&sys_params.packet_len, 2);
    reverse_bytes(&sys_params.baud_rate, 2);
    
    memcpy(user_params, &sys_params, 16);
    /*value = 0;
    uint8_t * loc;
    if (buffer[9] == FPM_OK){
        loc = &buffer[1] + param_offsets[param]*param_sizes[param];
        for (int i = 0; i < param_sizes[param]; i++){
            *((uint8_t *)value + i) = *(loc + param_sizes[param] - 1 - i);
        }
    } */     
    return confirm_code;
}

// NEW: download fingerprint image to pc
int16_t FPM::downImage(void) {
	buffer[0] = FPM_IMGUPLOAD;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);

    if (rc < 0)
        return rc;

    return confirm_code;
}

/* extract template/img data from packets and return true if successful */
bool FPM::readRaw(uint8_t outType, void * out, bool * read_complete, uint16_t * read_len = NULL) {
    Stream * outStream;
    uint8_t * outBuf;
    
    if (outType == FPM_OUTPUT_TO_BUFFER)
        outBuf = (uint8_t *)out;
    else if (outType == FPM_OUTPUT_TO_STREAM)
        outStream = (Stream *)out;
    else
        return false;
    
    uint16_t chunk_sz = packet_lengths[sys_params.packet_len];
    uint8_t pid;
    int16_t len;
    
    if (outType == FPM_OUTPUT_TO_BUFFER)
        len = getReply(outBuf, *read_len, &pid);
    else if (outType == FPM_OUTPUT_TO_STREAM)
        len = getReply(NULL, 0, &pid, outStream);
    
    /* check the length */
    if (len != chunk_sz) {
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

void FPM::writeRaw(uint8_t * data, uint16_t len){
    uint16_t written = 0;
    uint16_t chunk_sz = packet_lengths[sys_params.packet_len];
    
    while (len > chunk_sz) {
        writePacket(FPM_DATAPACKET, &data[written], chunk_sz);
        written += chunk_sz;
        len -= chunk_sz;
    }
    writePacket(FPM_ENDDATAPACKET, &data[written], len);
}

//transfer a fingerprint template from Char Buffer 1 to host computer
int16_t FPM::getModel(void) {
    buffer[0] = FPM_UPLOAD;
    buffer[1] = 0x01;
    writePacket(FPM_COMMANDPACKET, buffer, 2);
    uint8_t confirm_code = 0;
    int16_t rc = read_ack_get_response(&confirm_code);

    if (rc < 0)
        return rc;

    return confirm_code;
}

int16_t FPM::uploadModel(void){
    buffer[0] = FPM_DOWNCHAR;
    buffer[1] = 0x01;
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

int16_t FPM::fingerFastSearch(uint16_t * finger_id, uint16_t * score, uint8_t slot) {
    // high speed search of slot #1 starting at page 0 to 'capacity'
    buffer[0] = FPM_HISPEEDSEARCH;
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
    
    if (len != 2)
        return FPM_READ_ERROR;

    *finger_id = buffer[1];
    *finger_id <<= 8;
    *finger_id |= buffer[2];

    *score = buffer[3];
    *score <<= 8;
    *score |= buffer[4];

    return confirm_code;
}

int16_t FPM::matchTemplatePair(uint16_t * score){
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

int16_t FPM::getFreeIndex(uint8_t page, int16_t * id){
    buffer[0] = FPM_READTEMPLATEINDEX; 
    buffer[1] = page;
    
    writePacket(FPM_COMMANDPACKET, buffer, 2);
    uint8_t confirm_code = 0;
    
    int16_t len = read_ack_get_response(&confirm_code);
    
    if (len < 0)
        return len;
    
    if (confirm_code != FPM_OK)
        return confirm_code;
    
    for (int group = 0; group < len; i++) {
        if (buffer[1+group] == 0xff)
            continue;
        
        uint8_t bit_index = 0;
        for (uint8_t bit = 0x01; bit != 0; bit <<= 1) {
            if ((bit & buffer[1+group]) == 0){
                *id = (FPM_TEMPLATES_PER_PAGE * page) + (group * 8) + bit_index;
                return confirm_code;
            }
            
            bit_index++;
        }
    }
    
    *id = FPM_NOFREEINDEX;  // no free space found
    return confirm_code;
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

int16_t FPM::getReply(uint8_t * pktid) {
    return getReply(buffer, FPM_BUFFER_SZ, pktid, NULL);
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
        switch (state) {
            case FPM_STATE_READ_HEADER:
                if (port->available() == 0)
                    continue;
                
                last_read = millis();
                port->read(buffer, 1);
                header <<= 8; header |= buffer[0];
                if (header != FPM_STARTCODE)
                    break;
                
                state = FPM_STATE_READ_ADDRESS;
                header = 0;
                
                FPM_DEBUG_PRINTLN("[+]Got header");
                break;
            case FPM_STATE_READ_ADDRESS: {
                if (port->available() < 4)
                    continue;
                
                last_read = millis();
                port->read(buffer, 4);
                uint32_t addr = buffer[0]; addr <<= 8; 
                addr |= buffer[1]; addr <<= 8;
                addr |= buffer[2]; addr <<= 8;
                addr |= buffer[3];
                
                if (addr != address) {
                    state = FPM_STATE_READ_HEADER;
                    FPM_DEBUG_PRINTLN("[+]Wrong address");
                    break;
                }
                
                state = FPM_STATE_READ_PID;
                FPM_DEBUG_PRINT("[+]Address: 0x"); FPM_DEBUG_HEXLN(address);
                
                break;
            }
            case FPM_STATE_READ_PID:
                if (port->available() == 0)
                    continue;
                
                last_read = millis();
                port->read(&pid, 1);
                chksum = pid;
                *pktid = pid;
                
                state = FPM_STATE_READ_LENGTH;
                FPM_DEBUG_PRINT("[+]PID: 0x"); FPM_DEBUG_HEXLN(pid);
                
                break;
            case FPM_STATE_READ_LENGTH: {
                if (port->available() < 2)
                    continue;
                
                last_read = millis();
                port->read(buffer, 2);
                length = buffer[0]; length <<= 8;
                length |= buffer[1];
                
                if (length > FPM_MAX_PACKET_LEN + 2 || (outStream == NULL && length > buflen + 2)) {
                    state = FPM_STATE_READ_HEADER;
                    FPM_DEBUG_PRINTLN("[+]Packet too long");
                    continue;
                }
                
                /* num of bytes left to read */
                remn = length;
                
                chksum += buffer[0]; chksum += buffer[1];
                state = FPM_STATE_READ_CONTENTS;
                FPM_DEBUG_PRINT("[+]Length: "); FPM_DEBUG_DECLN(length - 2);
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
                uint8_t byte;
                port->read(&byte, 1);
                if (outStream != NULL) {
                    outStream->write(byte);
                }
                else {
                    *replyBuf++ = byte;
                    chksum += byte;
                }
                
                FPM_DEBUG_PRINT("[+]0x"); FPM_DEBUG_HEXLN(byte);
                remn--;
                break;
            }
            case FPM_STATE_READ_CHECKSUM: {
                if (port->available() < 2)
                    continue;
                
                last_read = millis();
                uint8_t temp[2];
                port->read(temp, 2);
                uint16_t to_check = temp[0]; to_check <<= 8;
                to_check |= temp[1];
                
                if (to_check != chksum) {
                    state = FPM_STATE_READ_HEADER;
                    FPM_DEBUG_PRINTLN("[+]Wrong chksum");
                    continue;
                }
                
                FPM_DEBUG_PRINTLN("[+]Read complete");
                /* without chksum */
                return length - 2;
            }
        }
    }
    
    return FPM_TIMEOUT;
}

/* read standard ACK-reply into library buffer and
 * return packet length and confirmation code */
int16_t read_ack_get_response(uint8_t * rc) {
    uint8_t pktid = 0;
    int16_t len = getReply(&pktid);
    
    /* most likely timed out */
    if (len < 0)
        return len;
    
    /* wrong pkt id */
    if (pktid != FPM_ACKPACKET)
        return FPM_READ_ERROR;
    
    *rc = buffer[0];
    
    /* minus confirmation code */
    return --len;
}