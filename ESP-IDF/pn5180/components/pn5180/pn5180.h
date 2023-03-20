// NAME: PN5180.h
//
// DESC: NFC Communication with NXP Semiconductors PN5180 module for Arduino.
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
#ifndef pn5180_h
#define pn5180_h

// PN5180 Commands
#define PN5180_WRITE_REGISTER                     (0x00)
#define PN5180_WRITE_REGISTER_OR_MASK             (0x01)
#define PN5180_WRITE_REGISTER_AND_MASK            (0x02)
#define PN5180_WRITE_REGISTER_MULTIPLE            (0x03)
#define PN5180_READ_REGISTER                      (0x04)
#define PN5180_READ_REGISTER_MULTIPLE             (0x05)
#define PN5180_WRITE_EEPROM                       (0x06)
#define PN5180_READ_EEPROM                        (0x07)
#define PN5180_WRITE_TX_DATA                      (0x08)
#define PN5180_SEND_DATA                          (0x09)
#define PN5180_READ_DATA                          (0x0A)
#define PN5180_SWITCH_MODE                        (0x0B)
#define PN5180_MIFARE_AUTHENTICATE                (0x0C)
#define PN5180_EPC_INVENTORY                      (0x0D)
#define PN5180_EPC_RESUME_INVENTORY               (0x0E)
#define PN5180_EPC_RETRIEVE_INVENTORY_RESULT_SIZE (0x0F)
#define PN5180_EPC_RETRIEVE_INVENTORY_RESULT      (0x10)
#define PN5180_LOAD_RF_CONFIG                     (0x11)
#define PN5180_UPDATE_RF_CONFIG                   (0x12)
#define PN5180_RETRIEVE_RF_CONFIG_SIZE            (0x13)
#define PN5180_RETRIEVE_RF_CONFIG                 (0x14)
#define PN5180_RF_ON                              (0x16)
#define PN5180_RF_OFF                             (0x17)
#define PN5180_CONFIGURE_TESTBUS_DIGITAL          (0x18)
#define PN5180_CONFIGURE_TESTBUS_ANALOG           (0x19)

// PN5180 Registers
#define PN5180_SYSTEM_CONFIG       (0x00)
#define PN5180_IRQ_ENABLE          (0x01)
#define PN5180_IRQ_STATUS          (0x02)
#define PN5180_IRQ_CLEAR           (0x03)
#define PN5180_TRANSCEIVE_CONTROL  (0x04)
#define PN5180_TIMER1_RELOAD       (0x0C)
#define PN5180_TIMER1_CONFIG       (0x0F)
#define PN5180_RX_WAIT_CONFIG      (0x11)
#define PN5180_CRC_RX_CONFIG       (0x12)
#define PN5180_RX_STATUS           (0x13)
#define PN5180_TX_WAIT_CONFIG	     (0x17)
#define PN5180_TX_CONFIG			     (0x18)
#define PN5180_CRC_TX_CONFIG       (0x19)
#define PN5180_RF_STATUS           (0x1D)
#define PN5180_SYSTEM_STATUS       (0x24)
#define PN5180_TEMP_CONTROL        (0x25)
#define PN5180_AGC_REF_CONFIG	     (0x26)


// PN5180 EEPROM Addresses
#define PN5180_DIE_IDENTIFIER      (0x00)
#define PN5180_PRODUCT_VERSION     (0x10)
#define PN5180_FIRMWARE_VERSION    (0x12)
#define PN5180_EEPROM_VERSION      (0x14)
#define PN5180_IRQ_PIN_CONFIG      (0x1A)

typedef enum {
  PN5180_TS_Idle = 0,
  PN5180_TS_WaitTransmit = 1,
  PN5180_TS_Transmitting = 2,
  PN5180_TS_WaitReceive = 3,
  PN5180_TS_WaitForData = 4,
  PN5180_TS_Receiving = 5,
  PN5180_TS_LoopBack = 6,
  PN5180_TS_RESERVED = 7
} PN5180TransceiveStat;

// PN5180 IRQ_STATUS
#define PN5180_RX_IRQ_STAT         	  (1<<0)  // End of RF receiption IRQ
#define PN5180_TX_IRQ_STAT         	  (1<<1)  // End of RF transmission IRQ
#define PN5180_IDLE_IRQ_STAT       	  (1<<2)  // IDLE IRQ
#define PN5180_RFOFF_DET_IRQ_STAT  	  (1<<6)  // RF Field OFF detection IRQ
#define PN5180_RFON_DET_IRQ_STAT   	  (1<<7)  // RF Field ON detection IRQ
#define PN5180_TX_RFOFF_IRQ_STAT   	  (1<<8)  // RF Field OFF in PCD IRQ
#define PN5180_TX_RFON_IRQ_STAT    	  (1<<9)  // RF Field ON in PCD IRQ
#define PN5180_RX_SOF_DET_IRQ_STAT 	  (1<<14) // RF SOF Detection IRQ
#define PN5180_GENERAL_ERROR_IRQ_STAT (1<<17) // General error IRQ
#define PN5180_LPCD_IRQ_STAT 			    (1<<19) // LPCD Detection IRQ

void pn5180_init();
esp_err_t pn5180_command(uint8_t *sendBuffer, size_t sendBufferLen, uint8_t *recvBuffer, size_t recvBufferLen);
//PN5180 direct commands with host interface
/* cmd 0x00 */
//bool pn5180_writeRegister(uint8_t reg, uint32_t value);
/* cmd 0x01 */
//bool pn5180_writeRegisterWithOrMask(uint8_t addr, uint32_t mask);
/* cmd 0x02 */
//bool pn5180_writeRegisterWithAndMask(uint8_t addr, uint32_t mask);

/* cmd 0x04 */
//bool pn5180_readRegister(uint8_t reg, uint32_t *value);

/* cmd 0x06 */
//bool pn5180_writeEEprom(uint8_t addr, uint8_t *buffer, uint8_t len);
/* cmd 0x07 */
//bool pn5180_readEEprom(uint8_t addr, uint8_t *buffer, int len);

/* cmd 0x09 */
//bool pn5180_sendData(uint8_t *data, int len, uint8_t validBits);
/* cmd 0x0a */
//bool pn5180_readData(uint8_t len, uint8_t *buffer);
/* prepare LPCD registers */
//bool pn5180_prepareLPCD();
/* cmd 0x0B */
//bool pn5180_switchToLPCD(uint16_t wakeupCounterInMs);
/* cmd 0x11 */
//bool pn5180_loadRFConfig(uint8_t txConf, uint8_t rxConf);

/* cmd 0x16 */
//bool pn5180_setRF_on();
/* cmd 0x17 */
//bool pn5180_setRF_off();

//uint8_t commandTimeout = 50;
//uint32_t pn5180_getIRQStatus();
//bool pn5180_clearIRQStatus(uint32_t irqMask);

//PN5180TransceiveStat getTransceiveState();
#endif /* PN5180_H */
