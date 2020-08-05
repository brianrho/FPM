/***************************************************  
  Copyright (c) 2023, Brian Ejike <bcejike@gmail.com>
  Distributed under the terms of the MIT license
 ****************************************************/

#include "fpm.h"
#include "fpm_logging.h"

#include <Arduino.h>

#define FPM_CHECKSUM_LENGTH     2

enum class FPMState {
    READ_HEADER,
    READ_METADATA,
    READ_PAYLOAD,
    READ_CHECKSUM
};

const uint16_t FPM::packetLengths[] = {32, 64, 128, 256};

FPM::FPM(Stream * ss) : 
    port(ss), password(FPM_DEFAULT_PASSWORD),
    address(FPM_DEFAULT_ADDRESS), useFixedParams(false)
{
    
}

bool FPM::begin(uint32_t pwd, uint32_t addr, FPMSystemParams * params) 
{
    delay(1000);            /* 500 ms at least according to datasheet */
    
#if (FPM_LOG_LEVEL != FPM_LOG_LEVEL_SILENT)
    printf_begin();
#endif
    
    address = addr;
    password = pwd;
    
    if (!verifyPassword(password)) {
        FPM_LOGLN_ERROR("begin: password verification failed");
        return false;
    }
    
    /* check if the user has supplied fixed parameters manually, 
     * this is needed for some sensors like the R308, which don't support SET_PARAM */
    if (params != NULL) {
        useFixedParams = true;
        memcpy(&sysParams, params, sizeof(FPMSystemParams));
        FPM_LOGLN_VERBOSE("begin: using fixed params");
    }
    else if (readParams() != FPMStatus::OK) {
        FPM_LOGLN_ERROR("begin: read params failed");
        return false;
    }
    
    return true;
}

bool FPM::verifyPassword(uint32_t pwd) 
{    
    buffer[0] = FPM_VERIFYPASSWORD;
    buffer[1] = (password >> 24) & 0xff; buffer[2] = (password >> 16) & 0xff;
    buffer[3] = (password >> 8) & 0xff; buffer[4] = password & 0xff;
    
    writePacket(FPM_COMMANDPACKET, buffer, 5);
    
    FPMStatus confirmCode; 
    uint16_t readLen;   
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    return (!FPM::isErrorCode(status) && confirmCode == FPMStatus::OK);
}

FPMStatus FPM::setPassword(uint32_t pwd) 
{
    buffer[0] = FPM_SETPASSWORD;
    buffer[1] = (pwd >> 24) & 0xff; buffer[2] = (pwd >> 16) & 0xff;
    buffer[3] = (pwd >> 8) & 0xff; buffer[4] = pwd & 0xff;
    
    return writeCommandGetResponse(5);
}

FPMStatus FPM::setAddress(uint32_t addr) 
{
    buffer[0] = FPM_SETADDRESS;
    buffer[1] = (addr >> 24) & 0xff; buffer[2] = (addr >> 16) & 0xff;
    buffer[3] = (addr >> 8) & 0xff; buffer[4] = addr & 0xff;
    
    return writeCommandGetResponse(5);
}

FPMStatus FPM::getImage(void) 
{
    buffer[0] = FPM_GETIMAGE;
    return writeCommandGetResponse(1);
}

/* tested with ZFM60 modules only */
FPMStatus FPM::getImageOnly(void) 
{
    buffer[0] = FPM_GETIMAGE_ONLY;
    return writeCommandGetResponse(1);
}

/* tested with ZFM60 modules only */
FPMStatus FPM::ledOn(void) 
{
    buffer[0] = FPM_LEDON;
    return writeCommandGetResponse(1);
}

/* tested with ZFM60 modules only */
FPMStatus FPM::ledOff(void) 
{
    buffer[0] = FPM_LEDOFF;
    return writeCommandGetResponse(1);
}

/* tested with R503 modules only */
FPMStatus FPM::ledConfigure(uint8_t controlCode, uint8_t speed, uint8_t colour, uint8_t numCycles)
 {
    buffer[0] = FPM_LEDCONTROL;
    buffer[1] = controlCode; buffer[2] = speed;
    buffer[3] = colour; buffer[4] = numCycles;
    
    return writeCommandGetResponse(5);
}    

FPMStatus FPM::standby(void) 
{
    buffer[0] = FPM_STANDBY;
    return writeCommandGetResponse(1);
}

FPMStatus FPM::image2Tz(uint8_t slot) 
{
    buffer[0] = FPM_IMAGE2TZ; 
    buffer[1] = slot;
    return writeCommandGetResponse(2);
}


FPMStatus FPM::generateTemplate(void) 
{
    buffer[0] = FPM_REGMODEL;
    return writeCommandGetResponse(1);
}


FPMStatus FPM::storeTemplate(uint16_t id, uint8_t slot) 
{
    buffer[0] = FPM_STORE;
    buffer[1] = slot;
    buffer[2] = id >> 8; buffer[3] = id & 0xFF;
    
    return writeCommandGetResponse(4);
}

FPMStatus FPM::loadTemplate(uint16_t id, uint8_t slot) 
{
    buffer[0] = FPM_LOAD;
    buffer[1] = slot;
    buffer[2] = id >> 8; buffer[3] = id & 0xFF;
    
    return writeCommandGetResponse(4);
}

FPMStatus FPM::setBaudRate(FPMBaud baudRate)
{
    return setParam(FPMParameter::BAUD_RATE, static_cast<uint8_t>(baudRate));
}

FPMStatus FPM::setSecurityLevel(FPMSecurityLevel securityLevel)
{
    return setParam(FPMParameter::SECURITY_LEVEL, static_cast<uint8_t>(securityLevel));
}

FPMStatus FPM::setPacketLength(FPMPacketLength packetLen)
{
    return setParam(FPMParameter::PACKET_LENGTH, static_cast<uint8_t>(packetLen));
}

FPMStatus FPM::setParam(FPMParameter param, uint8_t value) 
{
    /* if fixed parameters are in use, return an error at any attempt to set any parameter */
    if (useFixedParams) return FPMStatus::INVALID_PARAMS;
    
	buffer[0] = FPM_SETSYSPARAM;
    buffer[1] = static_cast<uint8_t>(param); buffer[2] = value;
    
	writePacket(FPM_COMMANDPACKET, buffer, 3);
	
    FPMStatus confirmCode;
    uint16_t readLen = 0;
    
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    if (FPM::isErrorCode(status)) return status;
    if (confirmCode != FPMStatus::OK) return confirmCode;
    
    /* wait for a bit and then read back the params,
     * to update our local copy */
    delay(100);
    readParams();
    
    return confirmCode;
}

static void reverseBytes(void *start, uint16_t size) 
{
    uint8_t *lo = (uint8_t *)start;
    uint8_t *hi = (uint8_t *)start + size - 1;
    uint8_t swap;
    while (lo < hi) {
        swap = *lo;
        *lo++ = *hi;
        *hi-- = swap;
    }
}

FPMStatus FPM::readParams(FPMSystemParams * params) 
{
    if (useFixedParams) {
        if (params != NULL) memcpy(params, &sysParams, FPM_SYS_PARAMS_LEN);
        return FPMStatus::OK;
    }
    
    buffer[0] = FPM_READSYSPARAM;
    
	writePacket(FPM_COMMANDPACKET, buffer, 1);
    FPMStatus confirmCode;
    uint16_t readLen = 0;
    
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    if (FPM::isErrorCode(status)) return status;
    if (confirmCode != FPMStatus::OK) return confirmCode;
    if (readLen != FPM_SYS_PARAMS_LEN) return FPMStatus::READ_ERROR;
    
    memcpy(&sysParams, &buffer[1], FPM_SYS_PARAMS_LEN);
    
    reverseBytes(&sysParams.statusReg, sizeof(sysParams.statusReg));
    reverseBytes(&sysParams.systemId, sizeof(sysParams.systemId));
    reverseBytes(&sysParams.capacity, sizeof(sysParams.capacity));
    reverseBytes(&sysParams.securityLevel, sizeof(sysParams.securityLevel));
    reverseBytes(&sysParams.deviceAddr, sizeof(sysParams.deviceAddr));
    reverseBytes(&sysParams.packetLen, sizeof(sysParams.packetLen));
    reverseBytes(&sysParams.baudRate, sizeof(sysParams.baudRate));
    
    if (params != NULL) memcpy(params, &sysParams, FPM_SYS_PARAMS_LEN);
    
    return confirmCode;
}

FPMStatus FPM::readProductInfo(FPMProductInfo * info) 
{    
    buffer[0] = FPM_READPRODINFO;
    
	writePacket(FPM_COMMANDPACKET, buffer, 1);
    FPMStatus confirmCode;
    uint16_t readLen = 0;
    
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    if (FPM::isErrorCode(status)) return status;
    if (confirmCode != FPMStatus::OK) return confirmCode;
    if (readLen != FPM_PRODUCT_INFO_LEN) return FPMStatus::READ_ERROR;
    
    memset(info, 0, sizeof(info));
    uint8_t *ptr = &buffer[1];
    
    memcpy(&info->moduleModel, ptr, FPM_PRODUCT_INFO_MODULE_MODEL_LEN);
    ptr += FPM_PRODUCT_INFO_MODULE_MODEL_LEN;
    
    memcpy(&info->batchNumber, ptr, FPM_PRODUCT_INFO_BATCH_NUMBER_LEN);
    ptr += FPM_PRODUCT_INFO_BATCH_NUMBER_LEN;
    
    memcpy(&info->serialNumber, ptr, FPM_PRODUCT_INFO_SERIAL_NUMBER_LEN);
    ptr += FPM_PRODUCT_INFO_SERIAL_NUMBER_LEN;
    
    memcpy(&info->hwVersion, ptr, sizeof(info->hwVersion));
    ptr += sizeof(info->hwVersion);
    
    memcpy(&info->sensorModel, ptr, FPM_PRODUCT_INFO_SENSOR_MODEL_LEN);
    ptr += FPM_PRODUCT_INFO_SENSOR_MODEL_LEN;
    
    memcpy(&info->imageWidth, ptr, sizeof(info->imageWidth));
    reverseBytes(&info->imageWidth, sizeof(info->imageWidth));
    ptr += sizeof(info->imageWidth);
    
    memcpy(&info->imageHeight, ptr, sizeof(info->imageHeight));
    reverseBytes(&info->imageHeight, sizeof(info->imageHeight));
    ptr += sizeof(info->imageHeight);
    
    memcpy(&info->templateSize, ptr, sizeof(info->templateSize));
    reverseBytes(&info->templateSize, sizeof(info->templateSize));
    ptr += sizeof(info->templateSize);
    
    memcpy(&info->databaseSize, ptr, sizeof(info->databaseSize));
    reverseBytes(&info->databaseSize, sizeof(info->databaseSize));
    ptr += sizeof(info->databaseSize);
    
    return confirmCode;
}

FPMStatus FPM::downloadImage(void) 
{
	buffer[0] = FPM_IMGUPLOAD;
    return writeCommandGetResponse(1);
}

bool FPM::readDataPacket(uint8_t * destBuffer, Stream * destStream, uint16_t * readLen, bool * readComplete) 
{
    uint8_t pktId;
    FPMStatus status;
    
    if (destBuffer == NULL && destStream == NULL)
    {
        return false;
    }
    
    /* this will prefer a Stream over a buffer, if the both are provided */
    status = readPacket(destBuffer, destStream, readLen, &pktId);
    
    if (FPM::isErrorCode(status)) {
        FPM_LOGLN_ERROR("readDataPacket: failed with status 0x%X", static_cast<uint16_t>(status));
        return false;
    }
    
    if (pktId == FPM_DATAPACKET || pktId == FPM_ENDDATAPACKET) 
    {        
        *readComplete = (pktId == FPM_ENDDATAPACKET);
        return true;
    }
    
    return false;
}

bool FPM::writeDataPacket(uint8_t * srcBuffer, Stream * srcStream, uint16_t * writeLen, bool writeComplete) 
{
    const uint16_t PACKET_LEN = FPM::packetLengths[static_cast<uint16_t>(sysParams.packetLen)];
    
    if (srcBuffer == NULL && srcStream == NULL)
    {
        return false;
    }
    /* clip the write-length to the current packet length */
    else if (*writeLen > PACKET_LEN)
    {
        FPM_LOGLN_VERBOSE("writeDataPacket: clipping, length %u > packetLen %u", *writeLen, PACKET_LEN);
        *writeLen = PACKET_LEN;
    }
    
    /* prefer a Stream over a buffer, if the both are provided */
    writePacket(srcBuffer, srcStream, writeLen, writeComplete ? FPM_ENDDATAPACKET : FPM_DATAPACKET);
    return true;
}

FPMStatus FPM::downloadTemplate(uint8_t slot) 
{
    buffer[0] = FPM_UPCHAR;
    buffer[1] = slot;
    return writeCommandGetResponse(2);
}

FPMStatus FPM::uploadTemplate(uint8_t slot) 
{
    buffer[0] = FPM_DOWNCHAR;
    buffer[1] = slot;
    
    return writeCommandGetResponse(2);
}
    
FPMStatus FPM::deleteTemplate(uint16_t id, uint16_t howMany) 
{
    buffer[0] = FPM_DELETE;
    buffer[1] = id >> 8; buffer[2] = id & 0xFF;
    buffer[3] = howMany >> 8; buffer[4] = howMany & 0xFF;
    
    return writeCommandGetResponse(5);
}

FPMStatus FPM::emptyDatabase(void) 
{
    buffer[0] = FPM_EMPTYDATABASE;
    
    return writeCommandGetResponse(1);
}

FPMStatus FPM::searchDatabase(uint16_t * finger_id, uint16_t * score, uint8_t slot) 
{
    /* search from ID 0 to 'capacity' */
    buffer[0] = FPM_SEARCH;
    buffer[1] = slot;
    buffer[2] = 0x00; buffer[3] = 0x00;
    buffer[4] = (uint8_t)(sysParams.capacity >> 8); 
    buffer[5] = (uint8_t)(sysParams.capacity & 0xFF);
    
    writePacket(FPM_COMMANDPACKET, buffer, 6);
    
    FPMStatus confirmCode;
    uint16_t readLen = 0;
    
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    if (FPM::isErrorCode(status)) return status;
    if (confirmCode != FPMStatus::OK) return confirmCode;
    if (readLen != 4) return FPMStatus::READ_ERROR;

    *finger_id = buffer[1];
    *finger_id <<= 8;
    *finger_id |= buffer[2];

    *score = buffer[3];
    *score <<= 8;
    *score |= buffer[4];

    return confirmCode;
}

FPMStatus FPM::matchTemplatePair(uint16_t * score) 
{
    buffer[0] = FPM_PAIRMATCH;
    
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    
    FPMStatus confirmCode;
    uint16_t readLen = 0;
    
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    if (FPM::isErrorCode(status)) return status;
    if (confirmCode != FPMStatus::OK) return confirmCode;
    if (readLen != 2) return FPMStatus::READ_ERROR;
    
    *score = buffer[1]; 
    *score <<= 8;
    *score |= buffer[2];

    return confirmCode;
}

FPMStatus FPM::getTemplateCount(uint16_t * templateCount) 
{
    buffer[0] = FPM_TEMPLATECOUNT;
    
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    
    FPMStatus confirmCode;
    uint16_t readLen = 0;
    
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    if (FPM::isErrorCode(status)) return status;
    if (confirmCode != FPMStatus::OK) return confirmCode;
    if (readLen != 2) return FPMStatus::READ_ERROR;
    
    *templateCount = buffer[1];
    *templateCount <<= 8;
    *templateCount |= buffer[2];

    return confirmCode;
}

FPMStatus FPM::getFreeIndex(uint8_t page, int16_t * id) 
{
    buffer[0] = FPM_READTEMPLATEINDEX; 
    buffer[1] = page;
    
    writePacket(FPM_COMMANDPACKET, buffer, 2);
    
    FPMStatus confirmCode; 
    uint16_t readLen = 0;
    
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    if (FPM::isErrorCode(status)) return status;
    if (confirmCode != FPMStatus::OK) return confirmCode;
    
    /* each bit within a byte represents the occupancy status of a slot
     * so each byte == group of 8 slots */
    for (int group_idx = 0; group_idx < readLen; group_idx++) {
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
    
    *id = -1;  // no free space found
    return confirmCode;
}

FPMStatus FPM::getRandomNumber(uint32_t * number) 
{
    buffer[0] = FPM_GETRANDOM;
    
    writePacket(FPM_COMMANDPACKET, buffer, 1);
    
    FPMStatus confirmCode;
    uint16_t readLen = 0;
    
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    if (FPM::isErrorCode(status)) return status;
    if (confirmCode != FPMStatus::OK) return confirmCode;
    if (readLen != 4) return FPMStatus::READ_ERROR;
    
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
    return writeCommandGetResponse(1) == FPMStatus::HANDSHAKE_OK;
}

void FPM::writePacket(uint8_t pktId, uint8_t * srcBuffer, uint16_t writeLen)
{
    writePacket(srcBuffer, NULL, &writeLen, pktId);
}

FPMStatus FPM::writePacket(uint8_t * srcBuffer, Stream * srcStream, uint16_t * writeLen, uint8_t pktId) 
{
    /* Add length of checksum to get the total length */
    uint16_t totalLen = *writeLen + 2;
    
    /* write the header */
    port->write((uint8_t)(FPM_STARTCODE >> 8));
    port->write((uint8_t)FPM_STARTCODE);
    port->write((uint8_t)(address >> 24));
    port->write((uint8_t)(address >> 16));
    port->write((uint8_t)(address >> 8));
    port->write((uint8_t)(address));
    port->write((uint8_t)pktId);
    port->write((uint8_t)(totalLen >> 8));
    port->write((uint8_t)(totalLen));
    
    /* begin calculating the checksum */
    uint16_t sum = (totalLen >> 8) + (totalLen & 0xFF) + pktId;
            
    /* for the payload, read it from the Stream if one has been provided */
    if (srcStream != NULL)
    {
        const uint16_t CHUNK_SIZE = 32;
        uint16_t remaining = *writeLen;
        
        uint32_t lastRead = millis();
        
        /* read from the given Stream in chunks,
         * and write to the sensor simultaneously */
        while ((uint32_t)(millis() - lastRead) < FPM_DEFAULT_TIMEOUT)
        {
            uint16_t toWrite = min(remaining, CHUNK_SIZE);
            
            if (srcStream->available() < toWrite) continue;
            
            lastRead = millis();
            
            srcStream->readBytes(buffer, toWrite);
            port->write(buffer, toWrite);
            
            for (int i = 0; i < toWrite; i++) {
                sum += buffer[i];
            }
            
            remaining -= toWrite;
            
            if (remaining == 0) break;
        }
        
        /* if we actually timed out, just return */
        if (millis() - lastRead >= FPM_DEFAULT_TIMEOUT)
        {
            FPM_LOGLN_ERROR("writePacket: timed out while reading from Stream");
            return FPMStatus::TIMEOUT;
        }
    }
    else
    {
        /* just write the payload straight from the provided buffer */
        port->write(srcBuffer, *writeLen);

        for (int i = 0; i < *writeLen; i++) {
            sum += srcBuffer[i];
        }
    }
    
    /* finally, the checksum */
    port->write((uint8_t)(sum >> 8));
    port->write((uint8_t)sum);
    return FPMStatus::LIB_OK;
}

FPMStatus FPM::readPacket(uint8_t * destBuffer, Stream * destStream, uint16_t * readLen, uint8_t * pktId) 
{
    /* Basic sanity check */
    if (destStream == NULL && readLen == NULL)
    {
        return FPMStatus::INVALID_PARAMS;
    }
    
    FPMState state = FPMState::READ_HEADER;
    
    uint16_t header = 0;
    uint16_t packetLen = 0;
    uint16_t remaining = 0;
    uint16_t chksum = 0;
    const uint16_t CHUNK_SIZE = 32;
    
    uint32_t lastRead = millis();
    
    FPM_LOG_VERBOSE("\r\n");
    
    while ((uint32_t)(millis() - lastRead) < FPM_DEFAULT_TIMEOUT)
    {        
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
                
                FPM_LOGLN_VERBOSE("Found Header");
                
                header = 0;
                state = FPMState::READ_METADATA;
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
                port->readBytes((uint8_t *)&addr, 4);
                reverseBytes(&addr, 4);
                
                if (addr != address) {
                    state = FPMState::READ_HEADER;
                    FPM_LOGLN_ERROR("Wrong address: 0x%X", addr);
                    break;
                }
                
                FPM_LOGLN_VERBOSE("Address: 0x%X", addr);
                
                /* read packet ID */
                *pktId = port->read();
                chksum = *pktId;
                FPM_LOGLN_VERBOSE("PID: 0x%X", *pktId);
                
                /* read and compare length */
                port->readBytes((uint8_t *)&packetLen, 2);
                reverseBytes(&packetLen, 2);
                
                /* ensure packet length is within acceptable bounds */
                if (packetLen <= FPM_CHECKSUM_LENGTH ||
                    packetLen > FPM_MAX_PACKET_LEN + FPM_CHECKSUM_LENGTH ||
                    (destStream == NULL && readLen != NULL && packetLen > (*readLen) + FPM_CHECKSUM_LENGTH)) 
                {
                    state = FPMState::READ_HEADER;
                    FPM_LOGLN_ERROR("Length is invalid or too large: %u", packetLen);
                    break;
                }
                
                FPM_LOGLN_VERBOSE("Length: %u", packetLen - FPM_CHECKSUM_LENGTH);
                
                /* number of bytes left to read, excluding checksum */
                remaining = packetLen - FPM_CHECKSUM_LENGTH;
                
                chksum += packetLen >> 8; chksum += packetLen & 0xFF;
                state = (remaining == 0) ? FPMState::READ_CHECKSUM : FPMState::READ_PAYLOAD;
                break;
            }
            
            case FPMState::READ_PAYLOAD:
            {
                /* wait until we've received the chunk size or everything that's left,
                 * whichever is lesser */
                uint16_t toRead = min(remaining, CHUNK_SIZE);
                if (port->available() < toRead) break;
                
                lastRead = millis();
                
                /* temporary reference to the chunk that will be read soon */
                uint8_t * tmpRef;
                
                /* if a Stream has been provided,
                 * read in the data first, then write it to the Stream */
                if (destStream != NULL) {
                    port->readBytes(buffer, toRead);
                    destStream->write(buffer, toRead);
                    tmpRef = buffer;
                }
                /* otherwise a buffer must have been provided,
                 * so read directly into it */
                else {
                    port->readBytes(destBuffer, toRead);
                    tmpRef = destBuffer;
                    destBuffer += toRead;
                }
                
                /* accumulate checksum */
                for (int i = 0; i < toRead; i++)
                {
                    chksum += tmpRef[i];
                    FPM_LOG_V_VERBOSE("%X ", tmpRef[i]);
                }
                
                FPM_LOG_V_VERBOSE("\r\n");
                
                remaining -= toRead;
                state = (remaining == 0) ? FPMState::READ_CHECKSUM : state;
                break;  
            }
            
            case FPMState::READ_CHECKSUM:
            {
                if (port->available() < FPM_CHECKSUM_LENGTH)
                    break;
                
                lastRead = millis();
                
                uint16_t pktChksum = 0;
                port->readBytes((uint8_t *)&pktChksum, FPM_CHECKSUM_LENGTH);
                reverseBytes(&pktChksum, FPM_CHECKSUM_LENGTH);
                
                if (pktChksum != chksum) {
                    state = FPMState::READ_HEADER;
                    FPM_LOGLN_ERROR("Wrong checksum: 0x%X != 0x%X", pktChksum, chksum);
                    break;
                }
                
                FPM_LOGLN_VERBOSE("Read complete.");
                if (readLen != NULL)    *readLen = packetLen - FPM_CHECKSUM_LENGTH;
                
                return FPMStatus::LIB_OK;
            }
        }
        
        yield();
    }
    
    FPM_LOGLN_ERROR("readPacket timeout.\r\n");
    return FPMStatus::TIMEOUT;
}

FPMStatus FPM::readAckGetResponse(FPMStatus * confirmCode, uint16_t * readLen) 
{   
    uint8_t pktId = 0;
    *readLen = FPM_BUFFER_SZ;
    FPMStatus status = readPacket(buffer, NULL, readLen, &pktId);
    
    /* most likely timed out */
    if (FPM::isErrorCode(status)) return status;
    
    /* wrong pkt id */
    if (pktId != FPM_ACKPACKET) {
        FPM_LOGLN_ERROR("Wrong PID: 0x%X", pktId);
        return FPMStatus::READ_ERROR;
    }
    
    *confirmCode = static_cast<FPMStatus>(buffer[0]);
    
    /* minus confirmation code */
    if (readLen != NULL) *readLen = *readLen - 1;
    return FPMStatus::LIB_OK;
}

FPMStatus FPM::writeCommandGetResponse(uint16_t payloadLen)
{
    writePacket(FPM_COMMANDPACKET, buffer, payloadLen);
    
    FPMStatus confirmCode;
    uint16_t readLen;
    FPMStatus status = readAckGetResponse(&confirmCode, &readLen);
    
    /* if we read an ACK packet successfully,
     * return its confirmation code, otherwise let the caller know why the read failed */
    return (FPM::isErrorCode(status)) ? status : confirmCode;
}

inline bool FPM::isErrorCode(FPMStatus status)
{
    return static_cast<uint16_t>(status) > static_cast<uint16_t>(FPMStatus::LIB_OK) &&
            static_cast<uint16_t>(status) <= static_cast<uint16_t>(FPMStatus::ERROR_END);
}

