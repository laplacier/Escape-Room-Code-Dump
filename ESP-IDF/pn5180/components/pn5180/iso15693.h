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
#ifndef iso15693_h
#define iso15693_h

#include "pn5180.h"

typedef enum {
  EC_NO_CARD = -1,
  ISO15693_EC_OK = 0,
  ISO15693_EC_NOT_SUPPORTED = 0x01,
  ISO15693_EC_NOT_RECOGNIZED = 0x02,
  ISO15693_EC_OPTION_NOT_SUPPORTED = 0x03,
  ISO15693_EC_UNKNOWN_ERROR = 0x0f,
  ISO15693_EC_BLOCK_NOT_AVAILABLE = 0x10,
  ISO15693_EC_BLOCK_ALREADY_LOCKED = 0x11,
  ISO15693_EC_BLOCK_IS_LOCKED = 0x12,
  ISO15693_EC_BLOCK_NOT_PROGRAMMED = 0x13,
  ISO15693_EC_BLOCK_NOT_LOCKED = 0x14,
  ISO15693_EC_CUSTOM_CMD_ERROR = 0xA0
} ISO15693ErrorCode_t;

typedef struct {
  char manufacturer[100];
  uint8_t type;
  char uid[30];
  uint8_t uid_raw[8];
  uint8_t dsfid;
  char afi[30];
  uint8_t ic_ref;
  // The physical memory of an ISO15693 VICC is organized in the form of blocks or pages of fixed size. Up to 256 blocks can be addressed and a block size can be up to 32 bytes.
  uint8_t numBlocks;
  uint16_t blockSize;
  uint8_t* blockData;
} ISO15693NFC_t;

ISO15693ErrorCode_t pn5180_ISO15693Command(uint8_t *cmd, uint16_t cmdLen, uint8_t **resultPtr);
ISO15693ErrorCode_t pn5180_getInventory(ISO15693NFC_t* nfc);
ISO15693ErrorCode_t pn5180_readSingleBlock(ISO15693NFC_t* nfc, uint8_t blockNo);
ISO15693ErrorCode_t pn5180_writeSingleBlock(ISO15693NFC_t* nfc, uint8_t blockNo);
ISO15693ErrorCode_t pn5180_getSystemInfo(ISO15693NFC_t* nfc);
// ICODE SLIX2 specific commands, see https://www.nxp.com/docs/en/data-sheet/SL2S2602.pdf
ISO15693ErrorCode_t pn5180_getRandomNumber(uint8_t *randomData);
ISO15693ErrorCode_t pn5180_setPassword(uint8_t identifier, uint8_t *password, uint8_t *random);
ISO15693ErrorCode_t pn5180_enablePrivacy(uint8_t *password, uint8_t *random);
// helpers
ISO15693ErrorCode_t pn5180_enablePrivacyMode(uint8_t *password);
ISO15693ErrorCode_t pn5180_disablePrivacyMode(uint8_t *password); 
esp_err_t pn5180_setupRF(void);
void iso15693_printError(ISO15693ErrorCode_t errno);  
void iso15693_printGeneric(const char* tag, uint8_t* dataBuf, uint8_t blockSize, uint8_t blockNum);

// Publicly available from https://www.kartenbezogene-identifier.de/de/chiphersteller-kennungen.html
static const char manufacturerCode[][100] = {
  "Unknown",
  "Motorola (UK)",
  "STMicroelectronics SA (FR)",
  "Hitachi Ltd (JP)",
  "NXP Semiconductors (DE)",
  "Infineon Technologies AG (DE)",
  "Cylink (US)",
  "Texas Instruments (FR)",
  "Fujitsu Limited (JP)",
  "Matsushita Electronics Corporation, Semiconductor Company (JP)",
  "NEC (JP)",
  "Oki Electric Industry Co Ltd (JP)",
  "Toshiba Corp (JP)",
  "Mitsubishi Electric Corp (JP)",
  "Samsung Electronics Co Ltd (KR)",
  "Hynix (KR)",
  "LG-Semiconductors Co Ltd (KR)",
  "Emosyn-EM Microelectronics (US)",
  "INSIDE Technology (FR)",
  "ORGA Kartensysteme GmbH (DE)",
  "Sharp Corporation (JP)",
  "ATMEL (FR)",
  "EM Microelectronic-Marin (CH)",
  "SMARTRAC TECHNOLOGY GmbH (DE)",
  "ZMD AG (DE)",
  "XICOR Inc (US)",
  "Sony Corporation (JP)",
  "Malaysia Microelectronic Solutions Sdn Bhd (MY)",
  "Emosyn (US)",
  "Shanghai Fudan Microelectronics Co Ltd (CN)",
  "Magellan Technology Pty Limited (AU)",
  "Melexis NV BO (CH)",
  "Renesas Technology Corp (JP)",
  "TAGSYS (FR)",
  "Transcore (US)",
  "Shanghai Belling Corp Ltd (CN)",
  "Masktech Germany GmbH (DE)",
  "Innovision Research and Technology Plc (UK)",
  "Hitachi ULSI Systems Co Ltd (JP)",
  "Yubico AB (SE)",
  "Ricoh (JP)",
  "ASK (FR)",
  "Unicore Microsystems LLC (RU)",
  "Dallas semiconductor/Maxim (US)",
  "Impinj Inc (US)",
  "RightPlug Alliance (US)",
  "Broadcom Corporation (US)",
  "MStar Semiconductor Inc (TW)",
  "BeeDar Technology Inc (US)",
  "RFIDsec (DK)",
  "Schweizer Electronic AG (DE)",
  "AMIC Technology Corp (TW)",
  "Mikron JSC (RU)",
  "Fraunhofer Institute for Photonic Microsystems (DE)",
  "IDS Microship AG (CH)",
  "Kovio (US)",
  "HMT Microelectronic Ltd (CH)",
  "Silicon Craft Technology (TH)",
  "Advanced Film Device Inc. (JP)",
  "Nitecrest Ltd (UK)",
  "Verayo Inc. (US)",
  "HID Global (US)",
  "Productivity Engineering Gmbh (DE)",
  "Austriamicrosystems AG (reserved) (AT)",
  "Gemalto SA (FR)",
  "Renesas Electronics Corporation (JP)",
  "3Alogics Inc (KR)",
  "Top TroniQ Asia Limited (Hong Kong)",
  "GenTag Inc (USA)",
  "Invengo Information Technology Co. Ltd (CN)",
  "Guangzhou Sysur Microelectronics, Inc (CN)",
  "CEITEC S.A. (BR)",
  "Shanghai Quanray Electronics Co. Ltd. (CN)",
  "MediaTek Inc (TW)",
  "Angstrem PJSC (RU)",
  "Celisic Semiconductor (Hong Kong) Limited (CN)",
  "LEGIC Identsystems AG (CH)",
  "Balluff GmbH (DE)",
  "Oberthur Technologies (FR)",
  "Silterra Malaysia Sdn. Bhd. (MY)",
  "DELTA Danish Electronics, Light & Acoustics (DK)",
  "Giesecke & Devrient GmbH (DE)",
  "Shenzhen China Vision Microelectronics Co., Ltd. (CN)",
  "Shanghai Feiju Microelectronics Co. Ltd. (CN)",
  "Intel Corporation (US)",
  "Microsensys GmbH (DE)",
  "Sonix Technology Co., Ltd. (TW)",
  "Qualcomm Technologies Inc (US)",
  "Realtek Semiconductor Corp (TW)",
  "Freevision Technologies Co. Ltd (CN)",
  "Giantec Semiconductor Inc. (CN)",
  "JSC Angstrem-T (RU)",
  "STARCHIP France",
  "SPIRTECH (FR)",
  "GANTNER Electronic GmbH (AT)",
  "Nordic Semiconductor (NO)",
  "Verisiti Inc (US)",
  "Wearlinks Technology Inc. (CN)",
  "Userstar Information Systems Co., Ltd (TW)",
  "Pragmatic Printing Ltd. (UK)",
  "Associacao do Laboratorio de Sistemas Integraveis Tecnologico - LSI-TEC (BR)",
  "Tendyron Corporation (CN)",
  "MUTO Smart Co., Ltd.(KR)",
  "ON Semiconductor (US)",
  "TÜBİTAK BİLGEM (TR)",
  "Huada Semiconductor Co., Ltd (CN)",
  "SEVENEY (FR)",
  "ISSM (FR)",
  "Wisesec Ltd (IL)",
  "Holtek (TW)"
};

static const char afi_string[][30] = {
  "All families",
  "Transport",
  "Financial",
  "Identification",
  "Telecommunication",
  "Medical",
  "Multimedia",
  "Gaming",
  "Data storage",
  "Item management",
  "Express parcels",
  "Postal services",
  "Airline bags"
};
#endif /* PN5180ISO15693_H */
