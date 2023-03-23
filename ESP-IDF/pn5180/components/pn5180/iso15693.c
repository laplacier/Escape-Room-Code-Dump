// NAME: PN5180ISO15693.h
//
// DESC: ISO15693 protocol on NXP Semiconductors PN5180 module for Arduino.
//
// Copyright (c) 2018 by Andreas Trappmann. All rights reserved.
//
// This file is part of the PN5180 library for the Arduino environment.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
//#define DEBUG 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "iso15693.h"
static char* TAG = "iso15693.c";

esp_err_t pn5180_setupRF(void) {
  esp_err_t ret;
  ESP_LOGD(TAG, "Loading RF-configuration...");
  ret = pn5180_loadRFConfig(0x0d, 0x8d);
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "setupRF: Failed to load RF Config");
    return ret;
  }

  ESP_LOGD(TAG, "Turning ON RF field...");
  ret = pn5180_setRF_on();
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "setupRF: Failed to set RF on");
    return ret;
  }

  pn5180_writeRegisterWithAndMask(PN5180_SYSTEM_CONFIG, 0xfffffff8);  // Idle/StopCom Command
  pn5180_writeRegisterWithOrMask(PN5180_SYSTEM_CONFIG, 0x00000003);   // Transceive Command

  return ESP_OK;
}

/*
 * Inventory, code=01
 *
 * Request format: SOF, Req.Flags, Inventory, AFI (opt.), Mask len, Mask value, CRC16, EOF
 * Response format: SOF, Resp.Flags, DSFID, UID, CRC16, EOF
 *
 */
ISO15693ErrorCode_t pn5180_getInventory(ISO15693NFC_t* nfc) {
  //                      Flags,  CMD, maskLen
  uint8_t inventory[3] = { 0x26, 0x01, 0x00 };
  //                         |\- inventory flag + high data rate
  //                         \-- 1 slot: only one card, no AFI field present
  ESP_LOGD(TAG,"getInventory: Get Inventory...");

  for (int i=0; i<8; i++) {
    nfc->uid_raw[i] = 0;  
  }
  
  uint8_t *readBuffer;
  ISO15693ErrorCode_t rc = pn5180_ISO15693Command(inventory, 3, &readBuffer);
  if (ISO15693_EC_OK != rc) return rc;
  if(readBuffer[9] != 224) return ISO15693_EC_NOT_RECOGNIZED; // UIDs always start with E0h (224d)

  // Record raw UID data
  for (int i=0; i<8; i++) {
    nfc->uid_raw[i] = readBuffer[2+i];
  }

  /*
   * https://www.nxp.com/docs/en/data-sheet/SL2S2002_SL2S2102.pdf
   *  
   * The 64-bit unique identifier (UID) is programmed during the production process according
   * to ISO/IEC 15693-3 and cannot be changed afterwards.
   * 
   * UID: AA:BB:CC:DDDDDDDDDD
   * 
   * AA - Always E0
   * BB - Manufacturer Code (0x04 = NXP Semiconductors)
   * CC - Tag Type (0x01 = ICODE SLIX)
   * DDDDDDDDDD - Random ID
   */

  // Determine Manufacturer
  uint8_t lengthMan = sizeof(manufacturerCode[nfc->uid_raw[6]]);
  for(int i=0; i<lengthMan; i++){
    nfc->manufacturer[i] = manufacturerCode[nfc->uid_raw[6]][i];
  }
  nfc->manufacturer[lengthMan] = '\0';

  // Record IC type
  nfc->type = nfc->uid_raw[5];

  // Properly format unique 5 byte UID
  for(int i=0; i<sizeof(nfc->uid); i++){
    nfc->uid[i] = '\0';
  }
  uint8_t hexSeg;
  uint8_t temp;
  uint8_t j;
  char hexChar[3] = "";
  hexChar[2] = '\0';
  for(int i=4; i>=0; i--){
    temp = nfc->uid_raw[i];
    hexChar[0] = 48;
    hexChar[1] = 48;
    j = 1;
    while(temp != 0) {
      hexSeg = temp % 16;
      //To convert integer into character
      if(hexSeg < 10) hexSeg += 48;
      else hexSeg += 55;
      hexChar[j] = (char)hexSeg;
      temp /= 16;
      j--;
    }
    strcat(nfc->uid, hexChar);
    if(i > 0) strcat(nfc->uid,":");
  }

  ESP_LOGD(TAG,"getInventory: Response flags: 0x%X, Data Storage Format ID: 0x%X", readBuffer[0], readBuffer[1]);

  return ISO15693_EC_OK;
}

/*
 * ISO 15693 - Protocol
 *
 * General Request Format:
 *  SOF, Req.Flags, Command code, Parameters, Data, CRC16, EOF
 *
 *  Request Flags:
 *    xxxx.3210
 *         |||\_ Subcarrier flag: 0=single sub-carrier, 1=two sub-carrier
 *         ||\__ Datarate flag: 0=low data rate, 1=high data rate
 *         |\___ Inventory flag: 0=no inventory, 1=inventory
 *         \____ Protocol extension flag: 0=no extension, 1=protocol format is extended
 *
 *  If Inventory flag is set:
 *    7654.xxxx
 *     ||\_ AFI flag: 0=no AFI field present, 1=AFI field is present
 *     |\__ Number of slots flag: 0=16 slots, 1=1 slot
 *     \___ Option flag: 0=default, 1=meaning is defined by command description
 *
 *  If Inventory flag is NOT set:
 *    7654.xxxx
 *     ||\_ Select flag: 0=request shall be executed by any VICC according to Address_flag
 *     ||                1=request shall be executed only by VICC in selected state
 *     |\__ Address flag: 0=request is not addressed. UID field is not present.
 *     |                  1=request is addressed. UID field is present. Only VICC with UID shall answer
 *     \___ Option flag: 0=default, 1=meaning is defined by command description
 *
 * General Response Format:
 *  SOF, Resp.Flags, Parameters, Data, CRC16, EOF
 *
 *  Response Flags:
 *    xxxx.3210
 *         |||\_ Error flag: 0=no error, 1=error detected, see error field
 *         ||\__ RFU: 0
 *         |\___ RFU: 0
 *         \____ Extension flag: 0=no extension, 1=protocol format is extended
 *
 *  If Error flag is set, the following error codes are defined:
 *    01 = The command is not supported, i.e. the request code is not recognized.
 *    02 = The command is not recognized, i.e. a format error occurred.
 *    03 = The option is not supported.
 *    0F = Unknown error.
 *    10 = The specific block is not available.
 *    11 = The specific block is already locked and cannot be locked again.
 *    12 = The specific block is locked and cannot be changed.
 *    13 = The specific block was not successfully programmed.
 *    14 = The specific block was not successfully locked.
 *    A0-DF = Custom command error codes
 *
 *  Function return values:
 *    0 = OK
 *   -1 = No card detected
 *   >0 = Error code
 */
ISO15693ErrorCode_t pn5180_ISO15693Command(uint8_t *cmd, uint16_t cmdLen, uint8_t **resultPtr) {
  ESP_LOGD(TAG,"ISO5693Command: Issue Command 0x%X...", cmd[1]);
  pn5180_sendData(cmd, cmdLen, 0);
  vTaskDelay(pdMS_TO_TICKS(10));

  uint8_t retries = 5;
  uint32_t irqR = pn5180_getIRQStatus();
  while (!(irqR & PN5180_RX_SOF_DET_IRQ_STAT) && retries > 0) {   // wait for RF field to set up (max 500ms)
    vTaskDelay(pdMS_TO_TICKS(10));
	  irqR = pn5180_getIRQStatus();
    retries--;
  }
  if (0 == (irqR & PN5180_RX_SOF_DET_IRQ_STAT)){
    ESP_LOGE(TAG, "ISO15693Command: No RX_SOF_DET IRQ. State=%ld", irqR);
    return EC_NO_CARD;
  }

  retries = 5;
  while (!(irqR & PN5180_RX_IRQ_STAT) && retries > 0) {   // wait for RX end of frame (max 500ms)
    vTaskDelay(pdMS_TO_TICKS(10));
	  retries--;
  }
  if(!(irqR & PN5180_RX_IRQ_STAT)){ 
    ESP_LOGE(TAG, "ISO15693Command: No EOF_RX IRQ. State=%ld", irqR);
    return EC_NO_CARD;
  }
  
  uint32_t rxStatus;
  pn5180_readRegister(PN5180_RX_STATUS, &rxStatus);
  uint16_t len = (uint16_t)(rxStatus & 0x000001ff);
  
  ESP_LOGD(TAG,"ISO5693Command: RX-Status=0x%lX, len=%d", rxStatus, len);

  *resultPtr = pn5180_readData(len);
  if (0L == *resultPtr) {
    ESP_LOGE(TAG,"ISO5693Command: ERROR in readData!");
    return ISO15693_EC_UNKNOWN_ERROR;
  }

  uint32_t irqStatus = pn5180_getIRQStatus();
  if (0 == (PN5180_RX_SOF_DET_IRQ_STAT & irqStatus)) { // no card detected
     pn5180_clearIRQStatus(PN5180_TX_IRQ_STAT | PN5180_IDLE_IRQ_STAT);
     return EC_NO_CARD;
  }

  uint8_t responseFlags = (*resultPtr)[0];
  if (responseFlags & (1<<0)) { // error flag
    uint8_t errorCode = (*resultPtr)[1];
    ESP_LOGE(TAG,"ISO5693Command: ERROR code=%X",errorCode);
    iso15693_printError(errorCode);
    if (errorCode >= 0xA0) { // custom command error codes
      return ISO15693_EC_CUSTOM_CMD_ERROR;
    }
    else return (ISO15693ErrorCode_t)errorCode;
  }

  ESP_LOGD(TAG,"ISO5693Command: Extension flag: %d", (responseFlags & (1<<3)));

  pn5180_clearIRQStatus(PN5180_RX_SOF_DET_IRQ_STAT | PN5180_IDLE_IRQ_STAT | PN5180_TX_IRQ_STAT | PN5180_RX_IRQ_STAT);
  return ISO15693_EC_OK;
}

void iso15693_printError(ISO15693ErrorCode_t errno) {
  char err[50] = "";
  switch (errno) {
    case EC_NO_CARD: strcat(err,"No card detected!"); break;
    case ISO15693_EC_OK: strcat(err,"OK!"); break;
    case ISO15693_EC_NOT_SUPPORTED: strcat(err,"Command is not supported!"); break;
    case ISO15693_EC_NOT_RECOGNIZED: strcat(err,"Command is not recognized!"); break;
    case ISO15693_EC_OPTION_NOT_SUPPORTED: strcat(err,"Option is not supported!"); break;
    case ISO15693_EC_UNKNOWN_ERROR: strcat(err,"Unknown error!"); break;
    case ISO15693_EC_BLOCK_NOT_AVAILABLE: strcat(err,"Specified block is not available!"); break;
    case ISO15693_EC_BLOCK_ALREADY_LOCKED: strcat(err,"Specified block is already locked!"); break;
    case ISO15693_EC_BLOCK_IS_LOCKED: strcat(err,"Specified block is locked and cannot be changed!"); break;
    case ISO15693_EC_BLOCK_NOT_PROGRAMMED: strcat(err,"Specified block was not successfully programmed!"); break;
    case ISO15693_EC_BLOCK_NOT_LOCKED: strcat(err,"Specified block was not successfully locked!"); break;
    default:
      if ((errno >= 0xA0) && (errno <= 0xDF)) {
        strcat(err,"Custom command error code!");
      }
      else strcat(err,"Undefined error code in ISO15693!");
  }
  ESP_LOGE(TAG, "ISO15693 Error: %s", err);
}

/*
 * Read single block, code=20
 *
 * Request format: SOF, Req.Flags, ReadSingleBlock, UID (opt.), BlockNumber, CRC16, EOF
 * Response format:
 *  when ERROR flag is set:
 *    SOF, Resp.Flags, ErrorCode, CRC16, EOF
 *
 *     Response Flags:
  *    xxxx.3xx0
  *         |||\_ Error flag: 0=no error, 1=error detected, see error field
  *         \____ Extension flag: 0=no extension, 1=protocol format is extended
  *
  *  If Error flag is set, the following error codes are defined:
  *    01 = The command is not supported, i.e. the request code is not recognized.
  *    02 = The command is not recognized, i.e. a format error occurred.
  *    03 = The option is not supported.
  *    0F = Unknown error.
  *    10 = The specific block is not available.
  *    11 = The specific block is already locked and cannot be locked again.
  *    12 = The specific block is locked and cannot be changed.
  *    13 = The specific block was not successfully programmed.
  *    14 = The specific block was not successfully locked.
  *    A0-DF = Custom command error codes
 *
 *  when ERROR flag is NOT set:
 *    SOF, Flags, BlockData (len=blockLength), CRC16, EOF
 */
ISO15693ErrorCode_t pn5180_readSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *blockData, uint8_t blockSize) {
  //                              flags, cmd, uid,             blockNo
  uint8_t readSingleBlock[11] = { 0x22, 0x20, 1,2,3,4,5,6,7,8, blockNo }; // UID has LSB first!
  //                                |\- high data rate
  //                                \-- no options, addressed by UID
  for (int i=0; i<8; i++) {
    readSingleBlock[2+i] = uid[i];
  }

  ESP_LOGD(TAG,"readSingleBlock: Read Single Block #%d, size=%d: ", blockNo, blockSize);
  ESP_LOG_BUFFER_HEX_LEVEL(TAG, readSingleBlock, sizeof(readSingleBlock), ESP_LOG_DEBUG);

  uint8_t *resultPtr;
  ISO15693ErrorCode_t rc = pn5180_ISO15693Command(readSingleBlock, 11, &resultPtr);
  if (ISO15693_EC_OK != rc) {
    return rc;
  }

  for (int i=0; i<blockSize; i++) {
    blockData[i] = resultPtr[2+i];
  }

  ESP_LOGD(TAG,"readSingleBlock: Value=");
  ESP_LOG_BUFFER_HEX_LEVEL(TAG, blockData, blockSize, ESP_LOG_DEBUG);

  return ISO15693_EC_OK;
}

/*
 * Write single block, code=21
 *
 * Request format: SOF, Requ.Flags, WriteSingleBlock, UID (opt.), BlockNumber, BlockData (len=blcokLength), CRC16, EOF
 * Response format:
 *  when ERROR flag is set:
 *    SOF, Resp.Flags, ErrorCode, CRC16, EOF
 *
 *     Response Flags:
  *    xxxx.3xx0
  *         |||\_ Error flag: 0=no error, 1=error detected, see error field
  *         \____ Extension flag: 0=no extension, 1=protocol format is extended
  *
  *  If Error flag is set, the following error codes are defined:
  *    01 = The command is not supported, i.e. the request code is not recognized.
  *    02 = The command is not recognized, i.e. a format error occurred.
  *    03 = The option is not supported.
  *    0F = Unknown error.
  *    10 = The specific block is not available.
  *    11 = The specific block is already locked and cannot be locked again.
  *    12 = The specific block is locked and cannot be changed.
  *    13 = The specific block was not successfully programmed.
  *    14 = The specific block was not successfully locked.
  *    A0-DF = Custom command error codes
 *
 *  when ERROR flag is NOT set:
 *    SOF, Resp.Flags, CRC16, EOF
 */
ISO15693ErrorCode_t pn5180_writeSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *blockData, uint8_t blockSize) {
  //                            flags, cmd, uid,             blockNo
  uint8_t writeSingleBlock[] = { 0x22, 0x21, 1,2,3,4,5,6,7,8, blockNo }; // UID has LSB first!
  //                               |\- high data rate
  //                               \-- no options, addressed by UID

  uint8_t writeCmdSize = sizeof(writeSingleBlock) + blockSize;
  uint8_t *writeCmd = (uint8_t*)malloc(writeCmdSize);
  uint8_t pos = 0;
  writeCmd[pos++] = writeSingleBlock[0];
  writeCmd[pos++] = writeSingleBlock[1];
  for (int i=0; i<8; i++) {
    writeCmd[pos++] = uid[i];
  }
  writeCmd[pos++] = blockNo;
  for (int i=0; i<blockSize; i++) {
    writeCmd[pos++] = blockData[i];
  }

  ESP_LOGD(TAG,"writeSingleBlock: Write Single Block #%d, size=%d: ", blockNo, blockSize);
  ESP_LOG_BUFFER_HEX_LEVEL(TAG, writeCmd, writeCmdSize, ESP_LOG_DEBUG);

  uint8_t *resultPtr;
  ISO15693ErrorCode_t rc = pn5180_ISO15693Command(writeCmd, sizeof(writeCmd), &resultPtr);
  if (ISO15693_EC_OK != rc) {
    free(writeCmd);
    return rc;
  }

  free(writeCmd);
  return ISO15693_EC_OK;
}

/*
 * Get System Information, code=2B
 *
 * Request format: SOF, Req.Flags, GetSysInfo, UID (opt.), CRC16, EOF
 * Response format:
 *  when ERROR flag is set:
 *    SOF, Resp.Flags, ErrorCode, CRC16, EOF
 *
 *     Response Flags:
  *    xxxx.3xx0
  *         |||\_ Error flag: 0=no error, 1=error detected, see error field
  *         \____ Extension flag: 0=no extension, 1=protocol format is extended
  *
  *  If Error flag is set, the following error codes are defined:
  *    01 = The command is not supported, i.e. the request code is not recognized.
  *    02 = The command is not recognized, i.e. a format error occurred.
  *    03 = The option is not supported.
  *    0F = Unknown error.
  *    10 = The specific block is not available.
  *    11 = The specific block is already locked and cannot be locked again.
  *    12 = The specific block is locked and cannot be changed.
  *    13 = The specific block was not successfully programmed.
  *    14 = The specific block was not successfully locked.
  *    A0-DF = Custom command error codes
  *
 *  when ERROR flag is NOT set:
 *    SOF, Flags, InfoFlags, UID, DSFID (opt.), AFI (opt.), Other fields (opt.), CRC16, EOF
 *
 *    InfoFlags:
 *    xxxx.3210
 *         |||\_ DSFID: 0=DSFID not supported, DSFID field NOT present; 1=DSFID supported, DSFID field present
 *         ||\__ AFI: 0=AFI not supported, AFI field not present; 1=AFI supported, AFI field present
 *         |\___ VICC memory size:
 *         |        0=Information on VICC memory size is not supported. Memory size field is present. ???
 *         |        1=Information on VICC memory size is supported. Memory size field is present.
 *         \____ IC reference:
 *                  0=Information on IC reference is not supported. IC reference field is not present.
 *                  1=Information on IC reference is supported. IC reference field is not present.
 *
 *    VICC memory size:
 *      xxxb.bbbb nnnn.nnnn
 *        bbbbb - Block size is expressed in number of bytes, on 5 bits, allowing to specify up to 32 bytes i.e. 256 bits.
 *        nnnn.nnnn - Number of blocks is on 8 bits, allowing to specify up to 256 blocks.
 *
 *    IC reference: The IC reference is on 8 bits and its meaning is defined by the IC manufacturer.
 */
ISO15693ErrorCode_t pn5180_getSystemInfo(uint8_t *uid, uint8_t *blockSize, uint8_t *numBlocks) {
  //esp_log_level_set(TAG, ESP_LOG_DEBUG);
  uint8_t sysInfo[] = { 0x22, 0x2b, 1,2,3,4,5,6,7,8 };  // UID has LSB first!
  for (int i=0; i<8; i++) {
    sysInfo[2+i] = uid[i];
  }

  ESP_LOGD(TAG,"getSystemInfo: Get System Information");
  ESP_LOG_BUFFER_HEX_LEVEL(TAG, sysInfo, sizeof(sysInfo), ESP_LOG_DEBUG);

  uint8_t *readBuffer;
  ISO15693ErrorCode_t rc = pn5180_ISO15693Command(sysInfo, 10, &readBuffer);
  if (ISO15693_EC_OK != rc) {
    return rc;
  }

  uint64_t printUID = 0;
  for (int i=0; i<8; i++) {
    uid[i] = readBuffer[2+i];
    printUID <<= 8;
    printUID |= readBuffer[2+i];
  }
  
  ESP_LOGD(TAG, "getSystemInfo: UID=%llX", printUID);

  uint8_t *p = &readBuffer[10];
  uint8_t infoFlags = readBuffer[1];

  if (infoFlags & 0x01) { // DSFID flag
    ESP_LOGD(TAG, "getSystemInfo: DSFID=%X", (uint8_t)(*p++)); // Data storage format identifier
  }
  else ESP_LOGD(TAG,"getSystemInfo: No DSFID");  
  
  if (infoFlags & 0x02) { // AFI flag
    uint8_t afi = *p++;
    char afi_string[30] = "";
    switch (afi >> 4) {
      case 0: strcat(afi_string,"All families"); break;
      case 1: strcat(afi_string,"Transport"); break;
      case 2: strcat(afi_string,"Financial"); break;
      case 3: strcat(afi_string,"Identification"); break;
      case 4: strcat(afi_string,"Telecommunication"); break;
      case 5: strcat(afi_string,"Medical"); break;
      case 6: strcat(afi_string,"Multimedia"); break;
      case 7: strcat(afi_string,"Gaming"); break;
      case 8: strcat(afi_string,"Data storage"); break;
      case 9: strcat(afi_string,"Item management"); break;
      case 10: strcat(afi_string,"Express parcels"); break;
      case 11: strcat(afi_string,"Postal services"); break;
      case 12: strcat(afi_string,"Airline bags"); break;
      default: strcat(afi_string,"Unknown"); break;
    }
    ESP_LOGD(TAG,"getSystemInfo: AFI=%X - %s", afi, afi_string);  // Application family identifier
  }
  else ESP_LOGD(TAG,"getSystemInfo: No AFI");

  if (infoFlags & 0x04) { // VICC Memory size
    *numBlocks = *p++;
    *blockSize = *p++;
    *blockSize = (*blockSize) & 0x1f;

    *blockSize = *blockSize + 1; // range: 1-32
    *numBlocks = *numBlocks + 1; // range: 1-256

    ESP_LOGD(TAG, "getSystemInfo: VICC MemSize=%d BlockSize=%d NumBlocks=%d", (uint16_t)(*blockSize) * (*numBlocks), *blockSize, *numBlocks);
  }
  else ESP_LOGD(TAG, "getSystemInfo: No VICC memory size");
   
  if (infoFlags & 0x08) { // IC reference
    ESP_LOGD(TAG, "getSystemInfo: IC Ref=%X", (uint8_t)(*p++));
  }
  else ESP_LOGD(TAG,"getSystemInfo: No IC ref");
  //esp_log_level_set(TAG, ESP_LOG_INFO);

  return ISO15693_EC_OK;
}


// ICODE SLIX specific commands

/*
 * The GET RANDOM NUMBER command is required to receive a random number from the label IC. 
 * The passwords that will be transmitted with the SET PASSWORD,ENABLEPRIVACY and DESTROY commands 
 * have to be calculated with the password and the random number (see Section 9.5.3.2 "SET PASSWORD")
 */
ISO15693ErrorCode_t pn5180_getRandomNumber(uint8_t *randomData) {
  uint8_t getrandom[3] = {0x02, 0xB2, 0x04};
  uint8_t *readBuffer;
  ISO15693ErrorCode_t rc = pn5180_ISO15693Command(getrandom, 3, &readBuffer);
  if (rc == ISO15693_EC_OK) {
    randomData[0] = readBuffer[1];
    randomData[1] = readBuffer[2];
  }
  return rc;
}

/*
 * The SET PASSWORD command enables the different passwords to be transmitted to the label 
 * to access the different protected functionalities of the following commands. 
 * The SET PASSWORD command has to be executed just once for the related passwords if the label is powered
 */
ISO15693ErrorCode_t pn5180_setPassword(uint8_t identifier, uint8_t *password, uint8_t *random) {
  uint8_t setPassword[8] = {0x02, 0xB3, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00};
  uint8_t *readBuffer;
  setPassword[3] = identifier;
  setPassword[4] = password[0] ^ random[0];
  setPassword[5] = password[1] ^ random[1];
  setPassword[6] = password[2] ^ random[0];
  setPassword[7] = password[3] ^ random[1];
  ISO15693ErrorCode_t rc = pn5180_ISO15693Command(setPassword, 8, &readBuffer);
  return rc;
}

/*
 * The ENABLE PRIVACY command enables the ICODE SLIX2 Label IC to be set to
 * Privacy mode if the Privacy password is correct. The ICODE SLIX2 will not respond to
 * any command except GET RANDOM NUMBER and SET PASSWORD
 */
ISO15693ErrorCode_t pn5180_enablePrivacy(uint8_t *password, uint8_t *random) {
  uint8_t setPrivacy[7] = {0x02, 0xBA, 0x04, 0x00, 0x00, 0x00, 0x00};
  uint8_t *readBuffer;
  setPrivacy[3] = password[0] ^ random[0];
  setPrivacy[4] = password[1] ^ random[1];
  setPrivacy[5] = password[2] ^ random[0];
  setPrivacy[6] = password[3] ^ random[1];
  ISO15693ErrorCode_t rc = pn5180_ISO15693Command(setPrivacy, 7, &readBuffer);
  return rc;
}


// disable privacy mode for ICODE SLIX2 tag with given password
ISO15693ErrorCode_t pn5180_disablePrivacyMode(uint8_t *password) {
  // get a random number from the tag
  uint8_t random[]= {0x00, 0x00};
  ISO15693ErrorCode_t rc = pn5180_getRandomNumber(random);
  if (rc != ISO15693_EC_OK) {
    return rc;
  }
  
  // set password to disable privacy mode 
  rc = pn5180_setPassword(0x04, password, random);
  return rc; 
}

// enable privacy mode for ICODE SLIX2 tag with given password 
ISO15693ErrorCode_t pn5180_enablePrivacyMode(uint8_t *password) {
  // get a random number from the tag
  uint8_t random[]= {0x00, 0x00};
  ISO15693ErrorCode_t rc = pn5180_getRandomNumber(random);
  if (rc != ISO15693_EC_OK) {
    return rc;
  }
  
  // enable privacy command to lock the tag
  rc = pn5180_enablePrivacy(password, random);
  return rc; 
}

void iso15693_printGeneric(const char* tag, uint8_t* dataBuf, uint8_t blockSize){
  if(ESP_LOG_INFO <= esp_log_level_get(tag)){
    // Hex print
    printf("\033[32mI (%ld) %s: ", esp_log_timestamp(), tag);
    for (int i=0; i<blockSize; i++) {
      if(dataBuf[i] < 16) printf("0");
      printf("%X", dataBuf[i]);
      if(i < blockSize - 1) printf(":");
    }
    
    printf(" ");
    
    // String print
    for (int i=0; i<blockSize; i++) {
      char c = dataBuf[i];
      if (isprint(c)) {
        printf("%c",c);
      }
      else printf(".");
    }
    printf("\n\033[0m");
  }
}

