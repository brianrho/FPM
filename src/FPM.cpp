/***************************************************  
  Written by Brian Ejike <brianrho94@gmail.com> (2017)
  Distributed under the terms of the MIT license
 ****************************************************/

#include <Arduino.h>
#include "FPM.h"
#include "fpm_logging.h"

#define FPM_CHECKSUM_LENGTH     2

enum class {
    READ_HEADER,
    READ_METADATA
    READ_PAYLOAD,
    READ_CHECKSUM
} FPMState;

const uint16_t FPM::packetLengths[] = {32, 64, 128, 256};

FPM::FPM(Stream * ss) : 
    port(ss), password(FPM_DEFAULT_PASSWORD), ]
    address(FPM_DEFAULT_ADDRESS), manualSettings(false)
{
    
}

bool FPM::begin(uint32_t pwd, uint32_t addr, FPMSystemParams * params) {
    delay(1000);            // 500 ms at least according to datasheet
    
    address = addr;
    
    if(!verifyPassword(pwd)) {
        FPM_LOGLN_ERROR("[+]Password verification failed");
        return false;
    }
    
    /* check if the user has supplied parameters manually, 
     * needed for some sensors like the R308
     */
    if (params != NULL) {
        manualSettings = true;
        memcpy(&sysParams, params, sizeof(FPMSystemParams));
    }
    else if (readParams() != FPM_OK) {
        FPM_LOGLN_ERROR("[+]Read params failed");
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
    uint8_t confirmCode = 0;
    int16_t len = readAckGetResponse(&confirmCode);
    
    if (len < 0 || confirmCode != FPM_OK)
        return false;
    
    return true;
}

int16_t FPM::setPassword(uint32_t pwd) {
    buffer[0] = FPM_SETPASSWORD;
    buffer[1] = (pwd >> 24) & 0xff; buffer[2] = (pwd >> 16) & 0xff;
    buffer[3] = (pwd >> 8) & 0xff; buffer[4] = pwd & 0xff;
    
    return writeCommandGetResponse(5);
}

int16_t FPM::setAddress(uint32_t addr) {
    buffer[0] = FPM_SETADDRESS;
    buffer[1] = (addr >> 24) & 0xff; buffer[2] = (addr >> 16) & 0xff;
    buffer[3] = (addr >> 8) & 0xff; buffer[4] = addr & 0xff;
    
    return writeCommandGetResponse(5);
}

int16_t FPM::getImage(void) {
    buffer[0] = FPM_GETIMAGE;
    return writeCommandGetResponse(1);
}

/* tested with ZFM60 modules only */
int16_t FPM::getImageNoLED(void) {
    buffer[0] = FPM_GETIMAGE_SANSLED;
    return writeCommandGetResponse(1);
}

/* tested with ZFM60 modules only */
int16_t FPM::ledOn(void) {
    buffer[0] = FPM_LEDON;
    return writeCommandGetResponse(1);
}

/* tested with ZFM60 modules only */
int16_t FPM::ledOff(void) {
    buffer[0] = FPM_LEDOFF;
    return writeCommandGetResponse(1);
}
/* tested with R503 modules only */
int16_t FPM::ledConfigure(uint8_t controlCode, uint8_t speed, uint8_t colour, uint8_t numCycles) {
    buffer[0] = FPM_LED_CONTROL;
    buffer[1] = controlCode; buffer[2] = speed;
    buffer[3] = colour; buffer[4] = numCycles;
    
    return writeCommandGetResponse(5);
}    

int16_t FPM::standby(void) {
    buffer[0] = FPM_STANDBY;
    return writeCommandGetResponse(1);
}

int16_t FPM::image2Tz(uint8_t slot) {
    buffer[0] = FPM_IMAGE2TZ; 
    buffer[1] = slot;
    return writeCommandGetResponse(2);
}


int16_t FPM::generateTemplate(void) {
    buffer[0] = FPM_REGMODEL;
    return writeCommandGetResponse(1);
}


int16_t FPM::storeTemplate(uint16_t id, uint8_t slot) {
    buffer[0] = FPM_STORE;
    buffer[1] = slot;
    buffer[2] = id >> 8; buffer[3] = id & 0xFF;
    
    return writeCommandGetResponse(4);
}

int16_t FPM::loadTemplate(uint16_t id, uint8_t slot) {
    buffer[0] = FPM_LOAD;
    buffer[1] = slot;
    buffer[2] = id >> 8; buffer[3] = id & 0xFF;
    
    return writeCommandGetResponse(4);
}

int16_t FPM::setParameter(FPMParameter param, uint8_t value) {
    if (manual_settings) {
        return FPM_PACKETRECIEVEERR;
    }
    
	buffer[0] = FPM_SETSYSPARAM;
    buffer[1] = param; buffer[2] = value;
    
	writePacket(FPM_COMMANDPACKET, buffer, 3);
    uint8_t confirmCode = 0;
    int16_t len = readAckGetResponse(&confirmCode);
    
    if (len < 0)
        return len;
    
    if (confirmCode != FPM_OK)
        return confirmCode;
    
    delay(100);
    readParams();
    return confirmCode;
}

static void reverseBytes(void *start, uint16_t size) {
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
    uint8_t confirmCode = 0;
    int16_t len = readAckGetResponse(&confirmCode);
    
    if (len < 0)
        return len;
    
    if (confirmCode != FPM_OK)
        return confirmCode;
    
    if (len != 16) {
        FPM_LOG_ERROR("[+]Unexpected params length: %d", len);
        return FPM_READ_ERROR;
    }
    
    memcpy(&sys_params, &buffer[1], 16);
    reverseBytes(&sys_params.status_reg, 2);
    reverseBytes(&sys_params.system_id, 2);
    reverseBytes(&sys_params.capacity, 2);
    reverseBytes(&sys_params.security_level, 2);
    reverseBytes(&sys_params.device_addr, 4);
    reverseBytes(&sys_params.packet_len, 2);
    reverseBytes(&sys_params.baud_rate, 2);
    
    if (user_params != NULL)
        memcpy(user_params, &sys_params, 16);
    
    return confirmCode;
}

int16_t FPM::downImage(void) {
	buffer[0] = FPM_IMGUPLOAD;
    return writeCommandGetResponse(1);
}

/* Can read into an array or into a Stream object */
bool FPM::readRaw(void * dest, uint16_t * readLen, bool * readComplete) {
    Stream * outStream = NULL;
    uint8_t * outBuffer = NULL;
    
    if (readLen == NULL) {
        outStream = static_cast<Stream *>dest;
    }
    else {
        outBuffer = static_cast<uint8_t *>dest;
    }
    
    uint8_t pktId;
    int16_t len;
    
    len = readPacket(outBuffer, readLen == NULL ? 0 : *readLen, &pktId, outStream);
    
    if (len <= 0) {
        FPM_LOG_ERROR("[+]Invalid read length: %d", len);
        return false;
    }
    
    if (pktId == FPM_DATAPACKET || pktId == FPM_ENDDATAPACKET) 
    {
        if (readLen != NULL)
        {
            *readLen = len;
        }
        
        *readComplete = (pktId == FPM_ENDDATAPACKET);
        return true;
    }
    
    return false;
}

void FPM::writeRaw(uint8_t * data, uint16_t len) {
    uint16_t written = 0;
    uint16_t chunkSize = packetLengths[sys_params.packet_len];
    
    while (len > chunkSize) {
        writePacket(FPM_DATAPACKET, &data[written], chunkSize);
        written += chunkSize;
        len -= chunkSize;
    }
    
    writePacket(FPM_ENDDATAPACKET, &data[written], len);
}

int16_t FPM::downloadTemplate(uint8_t slot) {
    buffer[0] = FPM_UPCHAR;
    buffer[1] = slot;
    return writeCommandGetResponse(2);
}

int16_t FPM::uploadTemplate(uint8_t slot) {
    buffer[0] = FPM_DOWNCHAR;
    buffer[1] = slot;
    
    return writeCommandGetResponse(2);
}
    
int16_t FPM::deleteModel(uint16_t id, uint16_t num_to_delete) {
    buffer[0] = FPM_DELETE;
    buffer[1] = id >> 8; buffer[2] = id & 0xFF;
    buffer[3] = num_to_delete >> 8; buffer[4] = num_to_delete & 0xFF;
    
    return writeCommandGetResponse(5);
}

int16_t FPM::emptyDatabase(void) {
    buffer[0] = FPM_EMPTYDATABASE;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirmCode = 0;
    int16_t rc = readAckGetResponse(&confirmCode);

    return (rc < 0) ? rc : confirmCode;
}

int16_t FPM::searchDatabase(uint16_t * finger_id, uint16_t * score, uint8_t slot) {
    /* search from ID 0 to 'capacity' */
    buffer[0] = FPM_SEARCH;
    buffer[1] = slot;
    buffer[2] = 0x00; buffer[3] = 0x00;
    buffer[4] = (uint8_t)(sys_params.capacity >> 8); 
    buffer[5] = (uint8_t)(sys_params.capacity & 0xFF);
    
    writePacket(FPM_COMMANDPACKET, buffer, 6);
    uint8_t confirmCode = 0;
    int16_t len = readAckGetResponse(&confirmCode);
    
    if (len < 0) return len;
    if (confirmCode != FPM_OK) return confirmCode;
    if (len != 4) return FPM_READ_ERROR;

    *finger_id = buffer[1];
    *finger_id <<= 8;
    *finger_id |= buffer[2];

    *score = buffer[3];
    *score <<= 8;
    *score |= buffer[4];

    return confirmCode;
}

int16_t FPM::matchTemplatePair(uint16_t * score) {
    buffer[0] = FPM_PAIRMATCH;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirmCode = 0;
    
    int16_t len = readAckGetResponse(&confirmCode);
    
    if (len < 0) return len;
    if (confirmCode != FPM_OK) return confirmCode;
    if (len != 2) return FPM_READ_ERROR;
    
    *score = buffer[1]; 
    *score <<= 8;
    *score |= buffer[2];

    return confirmCode;
}

int16_t FPM::getTemplateCount(uint16_t * template_cnt) {
    buffer[0] = FPM_TEMPLATECOUNT;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirmCode = 0;
    
    int16_t len = readAckGetResponse(&confirmCode);
    
    if (len < 0) return len;
    if (confirmCode != FPM_OK) return confirmCode;
    if (len != 2) return FPM_READ_ERROR;
    
    *template_cnt = buffer[1];
    *template_cnt <<= 8;
    *template_cnt |= buffer[2];

    return confirmCode;
}

int16_t FPM::getFreeIndex(uint8_t page, int16_t * id) {
    buffer[0] = FPM_READTEMPLATEINDEX; 
    buffer[1] = page;
    
    writePacket(FPM_COMMANDPACKET, buffer, 2);
    uint8_t confirmCode = 0;
    
    int16_t len = readAckGetResponse(&confirmCode);
    
    if (len < 0) return len;
    if (confirmCode != FPM_OK) return confirmCode;
    
    /* each bit within a byte represents the occupancy status of a slot
     * so each byte == group of 8 slots */
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
                return confirmCode;
            }
        }
    }
    
    *id = FPM_NOFREEINDEX;  // no free space found
    return confirmCode;
}

int16_t FPM::getRandomNumber(uint32_t * number) {
    buffer[0] = FPM_GETRANDOM;
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    uint8_t confirmCode = 0;
    
    int16_t len = readAckGetResponse(&confirmCode);
    
    if (len < 0) return len;
    if (confirmCode != FPM_OK) return confirmCode;
    if (len != 4) return FPM_READ_ERROR;
    
    *number = buffer[1]; 
    *number <<= 8;
    *number |= buffer[2];
    *number <<= 8;
    *number |= buffer[3];
    *number <<= 8;
    *number |= buffer[4];

    return confirmCode;
}

bool FPM::handshake(void) {
    buffer[0] = FPM_HANDSHAKE;
    return writeCommandGetResponse(1) == FPM_HANDSHAKE_OK;
}

void FPM::writePacket(uint8_t packetType, uint8_t * packet, uint16_t packetLen) {
    /* Add length of checksum to get the total length */
    uint16_t totalLen = packetLen + 2;
    
    /* write the preamble */
    port->write((uint8_t)(FPM_STARTCODE >> 8));
    port->write((uint8_t)FPM_STARTCODE);
    port->write((uint8_t)(address >> 24));
    port->write((uint8_t)(address >> 16));
    port->write((uint8_t)(address >> 8));
    port->write((uint8_t)(address));
    port->write((uint8_t)packetType);
    port->write((uint8_t)(totalLen >> 8));
    port->write((uint8_t)(totalLen));
    
    /* then the actual packet content */
    port->write(packet, packetLen);
    
    /* calculate checksum and write that too */
    uint16_t sum = (totalLen >> 8) + (totalLen & 0xFF) + packetType;
    for (int i = 0; i < packetLen; i++) {
        sum += packet[i];
    }
    
    port->write((uint8_t)(sum >> 8));
    port->write((uint8_t)sum);
}

int16_t FPM::readPacket(uint8_t * outBuffer, uint16_t outLen, uint8_t * pktId, Stream * outStream) 
{
    FPMState state = FPMState::READ_HEADER;
    
    uint16_t header = 0;
    uint16_t packetLen = 0;
    uint16_t chksum = 0;
    const uint8_t CHUNK_SIZE = 32;
    
    uint32_t lastRead = millis();
    
    while ((uint32_t)(millis() - lastRead) < FPM_DEFAULT_TIMEOUT)
    {
        yield();
        
        switch (state)
        {
            case FPMState::READ_HEADER:
            {
                if (port->available() == 0)
                    break;
                
                /* check if we've just read the header */
                uint8_t byte = port->read();
                lastRead = millis();
                
                header <<= 8; header |= byte;
                if (header != FPM_STARTCODE)
                    break;
                
                FPM_INFO_PRINTLN("\r\n[+]Found Header");
                
                header = 0;
                state = FPMState::READ_ADDRESS;
                break;
            }
                
            case FPMState::READ_METADATA:
            {
                /* metadata consists of:
                 * Address (4), Packet ID (1), Length (2) */
                if (port->available() < (4 + 1 + 2))
                    break;
                    
                lastRead = millis();
                
                /* Read, reverse and compare address */
                uint32_t addr;
                port->readBytes(&addr, 4);
                reverseBytes(&addr, 4);
                
                if (addr != address) {
                    state = FPMState::READ_HEADER;
                    FPM_LOGLN_ERROR("[+]Wrong address: 0x%X", addr);
                    break;
                }
                
                FPM_LOG_VERBOSE("[+]Address: 0x%X", addr);
                
                /* read packet ID */
                *pktId = port->read();
                chksum = *pktId;
                FPM_LOGLN_VERBOSE("[+]PID: 0x%X", *pktId);
                
                /* read and compare length */
                port->readBytes(&packetLen, 2);
                reverseBytes(&packetLen, 2);
                
                /* ensure packet length is within acceptable bounds */
                if (packetLen <= FPM_CHECKSUM_LENGTH 
                    || packetLen > FPM_MAX_PACKET_LEN + FPM_CHECKSUM_LENGTH 
                    || (outStream == NULL && packetLen > outLen + FPM_CHECKSUM_LENGTH)) 
                {
                    state = FPMState::READ_HEADER;
                    FPM_LOGLN_ERROR("[+]Invalid length: %u", packetLen);
                    break;
                }
                
                FPM_LOGLN_VERBOSE("[+]Length: %u", packetLen - FPM_CHECKSUM_LENGTH);
                
                /* number of bytes left to read, excluding checksum */
                remaining = packetLen - FPM_CHECKSUM_LENGTH;
                
                chksum += packetLen >> 8; chksum += packetLen & 0xFF;
                state = FPMState::READ_PAYLOAD;
                break;
            }
            
            case FPMState::READ_PAYLOAD:
            {
                /* wait until we've received the chunk size or everything that's left,
                 * whichever is greater */
                uint16_t toRead = max(remaining, CHUNK_SIZE);
                if (port->available() < toRead)
                    break;
                
                lastRead = millis();
                
                uint8_t * head;
                
                /* if a Stream has been provided,
                 * read in the data first, then write it to the Stream */
                if (outStream != NULL) {
                    port->readBytes(buffer, toRead);
                    outStream->write(buffer, toRead);
                    head = buffer;
                }
                /* otherwise a buffer must have been provided,
                 * so read directly into it */
                else {
                    port->readBytes(outBuffer, toRead);
                    head = outBuffer;
                    data += toRead;
                }
                
                /* accumulate checksum */
                for (int i = 0; i < toRead; i++)
                {
                    chksum += head[i];
                }
                
                remaining -= toRead;
                state = (remaining == 0) ? FPMState::READ_CHECKSUM : state;
                break;  
            }
            
            case FPMState::READ_CHECKSUM:
            {
                if (port->available() < FPM_CHECKSUM_LENGTH)
                    break;
                
                lastRead = millis();
                
                uint8_t pktChksum = 0;
                port->readBytes(pktChksum, FPM_CHECKSUM_LENGTH);
                reverseBytes(pktChksum, FPM_CHECKSUM_LENGTH);
                
                if (pktChksum != chksum) {
                    state = FPMState::READ_HEADER;
                    FPM_LOGLN_ERROR("\r\n[+]Wrong checksum: 0x%X", pktChksum);
                    break;
                }
                
                FPM_LOGLN_VERBOSE("\r\n[+]Read complete.");
                return length - FPM_CHECKSUM_LENGTH;
            }
        }
    }
    
    FPM_LOGLN_ERROR("[+]readPacket timeout.\r\n");
    return FPM_TIMEOUT; 
}

int16_t FPM::readPacket(uint8_t * data, uint16_t dataLen, uint8_t * pktId, Stream * outStream) {
    FPM_State state = FPMState::READ_HEADER;
    
    uint16_t header = 0;
    uint8_t pid = 0;
    uint16_t length = 0;
    uint16_t chksum = 0;
    uint16_t remn = 0;
    
    uint32_t lastRead = millis();
    
    while ((uint32_t)(millis() - lastRead) < FPM_DEFAULT_TIMEOUT) {
        yield();
        
        switch (state) {
            case FPMState::READ_HEADER: {
                if (port->available() == 0)
                    continue;
                
                lastRead = millis();
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
                
                lastRead = millis();
                
                /* address read in big-endian format,
                 * need to reverse it first before comparison */
                uint32_t addr;
                port->readBytes(&addr, 4);
                reverseBytes(&addr, 4);
                
                if (addr != address) {
                    state = FPMState::READ_HEADER;
                    FPM_LOG_ERROR("[+]Wrong address: 0x");
                    FPM_ERROR_HEXLN(addr);
                    break;
                }
                
                state = FPM_STATE_READ_PID;
                FPM_LOG_VERBOSE("[+]Address: 0x"); FPM_INFO_HEXLN(address);
                
                break;
            }
            case FPM_STATE_READ_PID:
                if (port->available() == 0)
                    continue;
                
                lastRead = millis();
                pid = port->read();
                *pktId = pid;
                chksum = pid;
                
                state = FPM_STATE_READ_LENGTH;
                FPM_LOGLN_VERBOSE("PID: 0x%X", pid);
                
                break;
            case FPM_STATE_READ_LENGTH:
                if (port->available() < 2)
                    continue;
                
                lastRead = millis();
                port->readBytes(buffer, 2);
                length = buffer[0]; length <<= 8;
                length |= buffer[1];
                
                /* ensure packet length is within acceptable bounds */
                if (length <= 2 || length > FPM_MAX_PACKET_LEN + 2 || (outStream == NULL && length > buflen + 2)) {
                    state = FPMState::READ_HEADER;
                    FPM_LOGLN_ERROR("Invalid packet length: %u", length);
                    continue;
                }
                
                /* number of bytes left to read */
                remn = length;
                
                chksum += buffer[0]; chksum += buffer[1];
                state = FPM_STATE_READ_CONTENTS;
                
                FPM_LOGLN_VERBOSE("Length: %u", length - 2);
                break;
            case FPM_STATE_READ_CONTENTS: {
                if (remn <= 2) {
                    state = FPM_STATE_READ_CHECKSUM;
                    break;
                }
                
                if (port->available() == 0)
                    continue;
                
                lastRead = millis();
                
                /* have to stop using 'buffer' since
                 * we may be storing data in it now */
                uint8_t byte = port->read();
                if (outStream != NULL) {
                    outStream->write(byte);
                }
                else {
                    *data++ = byte;
                }
                
                chksum += byte;
                remn--;
                
                FPM_LOG_VERBOSE("%X ", byte);
                break;
            }
            case FPM_STATE_READ_CHECKSUM: {
                if (port->available() < 2)
                    continue;
                
                lastRead = millis();
                
                uint8_t temp[2];
                port->readBytes(temp, 2);
                uint16_t pktChksum = temp[0]; pktChksum <<= 8;
                pktChksum |= temp[1];
                
                if (pktChecksum != chksum) {
                    state = FPMState::READ_HEADER;
                    FPM_LOGLN_ERROR("\r\nWrong checksum: 0x%X", pktChksum);
                    continue;
                }
                
                FPM_LOGLN_VERBOSE("\r\nRead complete");
                /* without chksum */
                return length - 2;
            }
        }
    }
    
    FPM_LOGLN_ERROR("Response timeout\r\n");
    return FPM_TIMEOUT;
}

/* read standard ACK-reply into library buffer and
 * return packet length and confirmation code */
int16_t FPM::readAckGetResponse(uint8_t * confirmCode) {
    uint8_t pktId = 0;
    int16_t len = readPacket(buffer, FPM_BUFFER_SZ, &pktId);
    
    /* most likely timed out */
    if (len <= 0)
        return len;
    
    /* wrong pkt id */
    if (pktId != FPM_ACKPACKET) {
        FPM_LOGLN_ERROR("Wrong PID: 0x%X", pktId);
        return FPM_READ_ERROR;
    }
    
    *confirmCode = buffer[0];
    
    /* minus confirmation code */
    return --len;
}

int16_t FPM::writeCommandGetResponse(uint16_t len)
{
    writePacket(FPM_COMMANDPACKET, buffer, len);
    
    uint8_t confirmCode = 0;
    int16_t rc = readAckGetResponse(&confirmCode);
    
    return (rc < 0) ? rc : confirmCode;
}

