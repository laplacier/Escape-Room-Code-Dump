// NAME: PN5180.cpp
//
// DESC: Implementation of PN5180 class.
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
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "pn5180.h"

#define ESP32_HOST VSPI_HOST
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_NSS  16
#define PIN_NUM_BUSY 5
#define PIN_NUM_RST  17
spi_device_handle_t nfc;

uint8_t pn5180_txBuffer[260] = {0};
uint8_t pn5180_rxBuffer[508] = {0};

// Prototypes
static void pn5180_reset(void);
esp_err_t pn5180_command(uint8_t *sendBuffer, size_t sendBufferLen, uint8_t *recvBuffer, size_t recvBufferLen);

/*
  * 11.4.1 Physical Host Interface
  * The interface of the PN5180 to a host microcontroller is based on a SPI interface,
  * extended by signal line BUSY. The maximum SPI speed is 7 Mbps and fixed to CPOL
  * = 0 and CPHA = 0.
  */
// Settings for PN5180: 7Mbps, MSB first, SPI_MODE0 (CPOL=0, CPHA=0)

void pn5180_init(void){
  gpio_config_t io_conf = {};

  // NSS and Reset are outputs
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  io_conf.pin_bit_mask = 0;
  io_conf.pin_bit_mask |= (1ULL << PIN_NUM_NSS);
  io_conf.pin_bit_mask |= (1ULL << PIN_NUM_RST);
  gpio_config(&io_conf);

  // BUSY is input
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = 0;
  io_conf.pin_bit_mask |= (1ULL << PIN_NUM_BUSY);
  gpio_config(&io_conf);

  gpio_set_level(PIN_NUM_NSS, 1); // Deselect device
  gpio_set_level(PIN_NUM_RST, 1); // Prevent reset

  // Configure physical bus settings for pn5180
  ESP_LOGI("NFC", "Initializing bus SPI...");
  spi_bus_config_t pn5180_buscfg={
      .miso_io_num = PIN_NUM_MISO,
      .mosi_io_num = PIN_NUM_MOSI,
      .sclk_io_num = PIN_NUM_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 512,
  };

  // Configure software settings for pn5180
  spi_device_interface_config_t pn5180_devcfg={
      .clock_speed_hz = 4000000,
      .mode = 0,
      .spics_io_num = PIN_NUM_NSS,
      .queue_size = 4,
      .pre_cb = NULL,
  };

  // Apply settings
  ESP_ERROR_CHECK(spi_bus_initialize(ESP32_HOST, &pn5180_buscfg, 1));
  ESP_ERROR_CHECK(spi_bus_add_device(ESP32_HOST, &pn5180_devcfg, &nfc));
  ESP_LOGI("NFC", "Initialized");

  // Reset device
  pn5180_reset();
}

/*
 * A Host Interface Command consists of either 1 or 2 SPI frames depending whether the
 * host wants to write or read data from the PN5180. An SPI Frame consists of multiple
 * bytes.
 * All commands are packed into one SPI Frame. An SPI Frame consists of multiple bytes.
 * No NSS toggles allowed during sending of an SPI frame.
 * For all 4 byte command parameter transfers (e.g. register values), the payload
 * parameters passed follow the little endian approach (Least Significant Byte first).
 * The BUSY line is used to indicate that the system is BUSY and cannot receive any data
 * from a host. Recommendation for the BUSY line handling by the host:
 * 1. Assert NSS to Low
 * 2. Perform Data Exchange
 * 3. Wait until BUSY is high
 * 4. Deassert NSS
 * 5. Wait until BUSY is low
 * If there is a parameter error, the IRQ is set to ACTIVE and a GENERAL_ERROR_IRQ is set.
 */
/*phStatus_t pn5180_txn(void     *pDataParams,
                             uint16_t  wOption,
                             uint8_t  *pTxBuffer,
                             uint16_t  wTxLength,
                             uint16_t  wRxBufSize,
                             uint8_t  *pRxBuffer,
                             uint16_t *pRxLength) {
    uint16_t len = wTxLength ? wTxLength : wRxBufSize; 
    ESP_LOGD(TAG, "SPI transaction: write %d read %d options 0x%08x",
             wTxLength, wRxBufSize, wOption);

    if(wTxLength && pTxBuffer != NULL) {
        ESP_LOGD(TAG, "Write data:");
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, pTxBuffer, wTxLength, ESP_LOG_DEBUG);

        // Do the write transaction.
        ESP_ERROR_CHECK( dal_spi_transact(g_spi_dev,
                                          pTxBuffer,
                                          NULL,
                                          wTxLength) );
    }

    if(wRxBufSize && pRxBuffer != NULL) {
        ESP_LOGD(TAG, "Initing %d bytes of read buffer %p", wRxBufSize, pRxBuffer);
        // Fill the read buffer, which will be outgoing, with FF.
        //memset(pRxBuffer, 0xff, wRxBufSize);

        ESP_LOGD(TAG, "Reading %d bytes", wRxBufSize);
        // Do the read transaction.
        ESP_ERROR_CHECK( dal_spi_transact(g_spi_dev,
                                          pRxBuffer,
                                          pRxBuffer,
                                          wRxBufSize) );

        ESP_LOGI(TAG, "Read data:\n");
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, pRxBuffer, wTxLength, ESP_LOG_INFO);
        *pRxLength = len;
    }

    return PH_DRIVER_SUCCESS;
}

esp_err_t dal_spi_transact(spi_device_handle_t dev,
                           const void *tx,
                           void       *rx,
                           int        len) {
    spi_transaction_t txn = {
        .length = len * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    return spi_device_transmit(dev, &txn);
}*/

esp_err_t pn5180_command(uint8_t *sendBuffer, size_t sendBufferLen, uint8_t *recvBuffer, size_t recvBufferLen) {
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  ESP_LOGI("NFC", "Begin, wait for busy...");
  // 0. Wait until BUSY is low
  while (gpio_get_level(PIN_NUM_BUSY)){
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  //////////////////
  // Send command //
  //////////////////
  t.length = sendBufferLen * 8;
  t.tx_buffer = sendBuffer;
  t.rx_buffer = NULL;
  // Write command
  ESP_LOGI("NFC", "Sending command...");
  ESP_ERROR_CHECK(spi_device_transmit(nfc, &t));
  // Finish if write-only command
  if ((0 == recvBuffer) || (0 == recvBufferLen)) return ESP_OK;

  //////////////////////
  // Receive Response //
  //////////////////////
  memset(recvBuffer, 0xFF, recvBufferLen);
  t.length = recvBufferLen * 8;
  t.tx_buffer = recvBuffer;
  t.rxlength = recvBufferLen * 8;
  t.rx_buffer = recvBuffer;
  // 2. Perform data exchange
  ESP_LOGI("NFC", "Receiving data... txlen=%p rxlen=%p",t.tx_buffer,t.rx_buffer);
  //ESP_ERROR_CHECK(spi_device_transmit(nfc, &t));
  spi_device_transmit(nfc, &t);
  ESP_LOGI("NFC", "Read data:\n");
  ESP_LOG_BUFFER_HEX_LEVEL("NFC", recvBuffer, recvBufferLen, ESP_LOG_INFO);
  return ESP_OK;
}

/*
 * Reset NFC device
 */
static void pn5180_reset(void) {
  gpio_set_level(PIN_NUM_RST, 0);  // at least 10us required
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(PIN_NUM_RST, 1); // 2ms to ramp up required
  vTaskDelay(pdMS_TO_TICKS(50));
}

#ifdef FALSE
/*
 * WRITE_REGISTER - 0x00
 * This command is used to write a 32-bit value (little endian) to a configuration register.
 * The address of the register must exist. If the condition is not fulfilled, an exception is
 * raised.
 */
bool pn5180_writeRegister(uint8_t reg, uint32_t value) {
  uint8_t *p = (uint8_t*)&value;

  /*
  For all 4 byte command parameter transfers (e.g. register values), the payload
  parameters passed follow the little endian approach (Least Significant Byte first).
   */
  uint8_t buf[6] = { PN5180_WRITE_REGISTER, reg, p[0], p[1], p[2], p[3] };

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  transceiveCommand(buf, 6);
  PN5180_SPI.endTransaction();

  return true;

  uint8_t tx_data[2] = { reg, value };
    spi_transaction_t t = {
        .tx_buffer = tx_data,
        .length = 2 * 8
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(nfc, &t));
}

/*
 * WRITE_REGISTER_OR_MASK - 0x01
 * This command modifies the content of a register using a logical OR operation. The
 * content of the register is read and a logical OR operation is performed with the provided
 * mask. The modified content is written back to the register.
 * The address of the register must exist. If the condition is not fulfilled, an exception is
 * raised.
 */
bool pn5180_writeRegisterWithOrMask(uint8_t reg, uint32_t mask) {
  uint8_t *p = (uint8_t*)&mask;

  uint8_t buf[6] = { PN5180_WRITE_REGISTER_OR_MASK, reg, p[0], p[1], p[2], p[3] };

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  transceiveCommand(buf, 6);
  PN5180_SPI.endTransaction();

  return true;
}

/*
 * WRITE _REGISTER_AND_MASK - 0x02
 * This command modifies the content of a register using a logical AND operation. The
 * content of the register is read and a logical AND operation is performed with the provided
 * mask. The modified content is written back to the register.
 * The address of the register must exist. If the condition is not fulfilled, an exception is
 * raised.
 */
bool pn5180_writeRegisterWithAndMask(uint8_t reg, uint32_t mask) {
  uint8_t *p = (uint8_t*)&mask;

  uint8_t buf[6] = { PN5180_WRITE_REGISTER_AND_MASK, reg, p[0], p[1], p[2], p[3] };

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  transceiveCommand(buf, 6);
  PN5180_SPI.endTransaction();

  return true;
}

/*
 * READ_REGISTER - 0x04
 * This command is used to read the content of a configuration register. The content of the
 * register is returned in the 4 byte response.
 * The address of the register must exist. If the condition is not fulfilled, an exception is
 * raised.
 */
bool pn5180_readRegister(uint8_t reg, uint32_t *value) {

  uint8_t cmd[2] = { PN5180_READ_REGISTER, reg };

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  transceiveCommand(cmd, 2, (uint8_t*)value, 4);
  PN5180_SPI.endTransaction();

  PN5180DEBUG(F("Register value=0x"));
  PN5180DEBUG(formatHex(*value));
  PN5180DEBUG("\n");

  return true;
}

/*
 * WRITE_EEPROM - 0x06
 */
bool pn5180_writeEEprom(uint8_t addr, uint8_t *buffer, uint8_t len) {
	uint8_t cmd[len + 2];
	cmd[0] = PN5180_WRITE_EEPROM;
	cmd[1] = addr;
	for (int i = 0; i < len; i++) cmd[2 + i] = buffer[i];
	PN5180_SPI.beginTransaction(SPI_SETTINGS);
	transceiveCommand(cmd, len + 2);
	PN5180_SPI.endTransaction();
	return true;
}

/*
 * READ_EEPROM - 0x07
 * This command is used to read data from EEPROM memory area. The field 'Address'
 * indicates the start address of the read operation. The field Length indicates the number
 * of bytes to read. The response contains the data read from EEPROM (content of the
 * EEPROM); The data is read in sequentially increasing order starting with the given
 * address.
 * EEPROM Address must be in the range from 0 to 254, inclusive. Read operation must
 * not go beyond EEPROM address 254. If the condition is not fulfilled, an exception is
 * raised.
 */
bool pn5180_readEEprom(uint8_t addr, uint8_t *buffer, int len) {
  if ((addr > 254) || ((addr+len) > 254)) {
    PN5180DEBUG(F("ERROR: Reading beyond addr 254!\n"));
    return false;
  }

  PN5180DEBUG(F("Reading EEPROM at 0x"));
  PN5180DEBUG(formatHex(addr));
  PN5180DEBUG(F(", size="));
  PN5180DEBUG(len);
  PN5180DEBUG(F("...\n"));

  uint8_t cmd[3] = { PN5180_READ_EEPROM, addr, uint8_t(len) };

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  transceiveCommand(cmd, 3, buffer, len);
  PN5180_SPI.endTransaction();

#ifdef DEBUG
  PN5180DEBUG(F("EEPROM values: "));
  for (int i=0; i<len; i++) {
    PN5180DEBUG(formatHex(buffer[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif

  return true;
}


/*
 * SEND_DATA - 0x09
 * This command writes data to the RF transmission buffer and starts the RF transmission.
 * The parameter ‘Number of valid bits in last Byte’ indicates the exact number of bits to be
 * transmitted for the last byte (for non-byte aligned frames).
 * Precondition: Host shall configure the Transceiver by setting the register
 * SYSTEM_CONFIG.COMMAND to 0x3 before using the SEND_DATA command, as
 * the command SEND_DATA is only writing data to the transmission buffer and starts the
 * transmission but does not perform any configuration.
 * The size of ‘Tx Data’ field must be in the range from 0 to 260, inclusive (the 0 byte length
 * allows a symbol only transmission when the TX_DATA_ENABLE is cleared).‘Number of
 * valid bits in last Byte’ field must be in the range from 0 to 7. The command must not be
 * called during an ongoing RF transmission. Transceiver must be in ‘WaitTransmit’ state
 * with ‘Transceive’ command set. If the condition is not fulfilled, an exception is raised.
 */
bool pn5180_sendData(uint8_t *data, int len, uint8_t validBits) {
  if (len > 260) {
    return false;
  }

  uint8_t buffer[len+2];
  buffer[0] = PN5180_SEND_DATA;
  buffer[1] = validBits; // number of valid bits of last byte are transmitted (0 = all bits are transmitted)
  for (int i=0; i<len; i++) {
    buffer[2+i] = data[i];
  }

  writeRegisterWithAndMask(SYSTEM_CONFIG, 0xfffffff8);  // Idle/StopCom Command
  writeRegisterWithOrMask(SYSTEM_CONFIG, 0x00000003);   // Transceive Command
  /*
   * Transceive command; initiates a transceive cycle.
   * Note: Depending on the value of the Initiator bit, a
   * transmission is started or the receiver is enabled
   * Note: The transceive command does not finish
   * automatically. It stays in the transceive cycle until
   * stopped via the IDLE/StopCom command
   */

  PN5180TransceiveStat transceiveState = getTransceiveState();
  if (PN5180_TS_WaitTransmit != transceiveState) {
    return false;
  }

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  bool success = transceiveCommand(buffer, len+2);
  PN5180_SPI.endTransaction();

  return success;
}

/*
 * READ_DATA - 0x0A
 * This command reads data from the RF reception buffer, after a successful reception.
 * The RX_STATUS register contains the information to verify if the reception had been
 * successful. The data is available within the response of the command. The host controls
 * the number of bytes to be read via the SPI interface.
 * The RF data had been successfully received. In case the instruction is executed without
 * preceding an RF data reception, no exception is raised but the data read back from the
 * reception buffer is invalid. If the condition is not fulfilled, an exception is raised.
 */
uint8_t * pn5180_readData(int len) {
  if (len > 508) {
    return 0L;
  }

  uint8_t cmd[2] = { PN5180_READ_DATA, 0x00 };

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  transceiveCommand(cmd, 2, readBuffer, len);
  PN5180_SPI.endTransaction();

  return readBuffer;
}

bool pn5180_readData(uint8_t len, uint8_t *buffer) {
	if (len > 508) {
		return false;
	}
	uint8_t cmd[2] = { PN5180_READ_DATA, 0x00 };
	PN5180_SPI.beginTransaction(SPI_SETTINGS);
	bool success = transceiveCommand(cmd, 2, buffer, len);
	PN5180_SPI.endTransaction();
	return success;
}

/* prepare LPCD registers */
bool pn5180_prepareLPCD() {
  //=======================================LPCD CONFIG================================================================================

  uint8_t data[255];
  uint8_t response[256];
    //1. Set Fieldon time                                           LPCD_FIELD_ON_TIME (0x36)
  uint8_t fieldOn = 0xF0;//0x## -> ##(base 10) x 8μs + 62 μs
  data[0] = fieldOn;
  pn5180_writeEEprom(0x36, data, 1);
  pn5180_readEEprom(0x36, response, 1);
  fieldOn = response[0];

    //2. Set threshold level                                         AGC_LPCD_THRESHOLD @ EEPROM 0x37
  uint8_t threshold = 0x03;
  data[0] = threshold;
  pn5180_writeEEprom(0x37, data, 1);
  pn5180_readEEprom(0x37, response, 1);
  threshold = response[0];

  //3. Select LPCD mode                                               LPCD_REFVAL_GPO_CONTROL (0x38)
  uint8_t lpcdMode = 0x01; // 1 = LPCD SELF CALIBRATION 
                           // 0 = LPCD AUTO CALIBRATION (this mode does not work, should look more into it, no reason why it shouldn't work)
  data[0] = lpcdMode;
  pn5180_writeEEprom(0x38, data, 1);
  pn5180_readEEprom(0x38, response, 1);
  lpcdMode = response[0];
  
  // LPCD_GPO_TOGGLE_BEFORE_FIELD_ON (0x39)
  uint8_t beforeFieldOn = 0xF0; 
  data[0] = beforeFieldOn;
  pn5180_writeEEprom(0x39, data, 1);
  pn5180_readEEprom(0x39, response, 1);
  beforeFieldOn = response[0];
  
  // LPCD_GPO_TOGGLE_AFTER_FIELD_ON (0x3A)
  uint8_t afterFieldOn = 0xF0; 
  data[0] = afterFieldOn;
  pn5180_writeEEprom(0x3A, data, 1);
  pn5180_readEEprom(0x3A, response, 1);
  afterFieldOn = response[0];
  delay(100);
  return true;
}

/* switch the mode to LPCD (low power card detection)
 * Parameter 'wakeupCounterInMs' must be in the range from 0x0 - 0xA82
 * max. wake-up time is 2960 ms.
 */
bool pn5180_switchToLPCD(uint16_t wakeupCounterInMs) {
  // clear all IRQ flags
  pn5180_clearIRQStatus(0xffffffff); 
  // enable only LPCD and general error IRQ
  pn5180_writeRegister(IRQ_ENABLE, LPCD_IRQ_STAT | GENERAL_ERROR_IRQ_STAT);  
  // switch mode to LPCD 
  uint8_t cmd[4] = { PN5180_SWITCH_MODE, 0x01, (uint8_t)(wakeupCounterInMs & 0xFF), (uint8_t)((wakeupCounterInMs >> 8U) & 0xFF) };
  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  bool success = transceiveCommand(cmd, sizeof(cmd));
  PN5180_SPI.endTransaction();
  return success;
}

/*
 * LOAD_RF_CONFIG - 0x11
 * Parameter 'Transmitter Configuration' must be in the range from 0x0 - 0x1C, inclusive. If
 * the transmitter parameter is 0xFF, transmitter configuration is not changed.
 * Field 'Receiver Configuration' must be in the range from 0x80 - 0x9C, inclusive. If the
 * receiver parameter is 0xFF, the receiver configuration is not changed. If the condition is
 * not fulfilled, an exception is raised.
 * The transmitter and receiver configuration shall always be configured for the same
 * transmission/reception speed. No error is returned in case this condition is not taken into
 * account.
 *
 * Transmitter: RF   Protocol          Speed     Receiver: RF    Protocol    Speed
 * configuration                       (kbit/s)  configuration               (kbit/s)
 * byte (hex)                                    byte (hex)
 * ----------------------------------------------------------------------------------------------
 * ->0D              ISO 15693 ASK100  26        8D              ISO 15693   26
 *   0E              ISO 15693 ASK10   26        8E              ISO 15693   53
 */
bool pn5180_loadRFConfig(uint8_t txConf, uint8_t rxConf) {

  uint8_t cmd[3] = { PN5180_LOAD_RF_CONFIG, txConf, rxConf };

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  transceiveCommand(cmd, 3);
  PN5180_SPI.endTransaction();

  return true;
}

/*
 * RF_ON - 0x16
 * This command is used to switch on the internal RF field. If enabled the TX_RFON_IRQ is
 * set after the field is switched on.
 */
bool pn5180_setRF_on() {

  uint8_t cmd[2] = { PN5180_RF_ON, 0x00 };

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  pn5180_transceiveCommand(cmd, 2);
  PN5180_SPI.endTransaction();

  unsigned long startedWaiting = millis();
  while (0 == (TX_RFON_IRQ_STAT & pn5180_getIRQStatus())) {   // wait for RF field to set up (max 500ms)
    if (millis() - startedWaiting > 500) {
	  return false; 
	}
  }; 
  
  pn5180_clearIRQStatus(TX_RFON_IRQ_STAT);
  return true;
}

/*
 * RF_OFF - 0x17
 * This command is used to switch off the internal RF field. If enabled, the TX_RFOFF_IRQ
 * is set after the field is switched off.
 */
bool pn5180_setRF_off() {

  uint8_t cmd[2] { PN5180_RF_OFF, 0x00 };

  PN5180_SPI.beginTransaction(SPI_SETTINGS);
  transceiveCommand(cmd, 2);
  PN5180_SPI.endTransaction();

  unsigned long startedWaiting = millis();
  while (0 == (TX_RFOFF_IRQ_STAT & getIRQStatus())) {   // wait for RF field to shut down
    if (millis() - startedWaiting > 500) {
	  return false; 
	}
  }; 
  pn5180_clearIRQStatus(TX_RFOFF_IRQ_STAT);
  return true;
}

//---------------------------------------------------------------------------------------------

/*
11.4.3.1 A Host Interface Command consists of either 1 or 2 SPI frames depending whether the
host wants to write or read data from the PN5180. An SPI Frame consists of multiple
bytes.

All commands are packed into one SPI Frame. An SPI Frame consists of multiple bytes.
No NSS toggles allowed during sending of an SPI frame.

For all 4 byte command parameter transfers (e.g. register values), the payload
parameters passed follow the little endian approach (Least Significant Byte first).

Direct Instructions are built of a command code (1 Byte) and the instruction parameters
(max. 260 bytes). The actual payload size depends on the instruction used.
Responses to direct instructions contain only a payload field (no header).
All instructions are bound to conditions. If at least one of the conditions is not fulfilled, an exception is
raised. In case of an exception, the IRQ line of PN5180 is asserted and corresponding interrupt
status register contain information on the exception.
*/
#endif

#ifdef FALSE
/*
 * Reset NFC device
 */
void pn5180_reset() {
  digitalWrite(PN5180_RST, LOW);  // at least 10us required
  delay(1);
  digitalWrite(PN5180_RST, HIGH); // 2ms to ramp up required
  delay(5);

  unsigned long startedWaiting = millis();
  while (0 == (IDLE_IRQ_STAT & pn5180_getIRQStatus())) {
	// wait for system to start up (with timeout)
    if (millis() - startedWaiting > commandTimeout) {
		// try again with larger time
		digitalWrite(PN5180_RST, LOW);  
		delay(10);
		digitalWrite(PN5180_RST, HIGH); 
		delay(50);
		return;
	}
  }
}

/**
 * @name  getInterrupt
 * @desc  read interrupt status register and clear interrupt status
 */
uint32_t pn5180_getIRQStatus() {

  uint32_t irqStatus;
  pn5180_readRegister(IRQ_STATUS, &irqStatus);

  return irqStatus;
}

bool pn5180_clearIRQStatus(uint32_t irqMask) {

  return writeRegister(IRQ_CLEAR, irqMask);
}

/*
 * Get TRANSCEIVE_STATE from RF_STATUS register
 */

PN5180TransceiveStat getTransceiveState() {

  uint32_t rfStatus;
  if (!pn5180_readRegister(RF_STATUS, &rfStatus)) {
#ifdef DEBUG
    pn5180_showIRQStatus(pn5180_getIRQStatus());
#endif
    return PN5180TransceiveStat(0);
  }

  /*
   * TRANSCEIVE_STATEs:
   *  0 - idle
   *  1 - wait transmit
   *  2 - transmitting
   *  3 - wait receive
   *  4 - wait for data
   *  5 - receiving
   *  6 - loopback
   *  7 - reserved
   */
  uint8_t state = ((rfStatus >> 24) & 0x07);

  return PN5180TransceiveStat(state);
}
#endif