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
#include "iso15693.h"
#include "pn5180.h"

#define ESP32_HOST VSPI_HOST
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_NSS  16
#define PIN_NUM_BUSY 5
#define PIN_NUM_RST  17
spi_device_handle_t nfc;

static char* TAG = "pn5180.c";
uint8_t readBuffer[508];

////////////////////////
// Private Prototypes //
////////////////////////
/**
 * @brief  This command, 0x06, is used to write up to 255 bytes to the EEPROM. The field ‘EEPROM content’ contains the data to be written to EEPROM starting at the address given by byte ‘EEPROM Address’. The data is written in sequential order.
 * 
 * @param  txBuffer MOSI content.
 * @param  txBufferLen Length of MOSI content.
 * @param  rxBuffer MISO content.
 * @param  rxBufferLen Length of MISO content.
 *
 * @return
 *     - ESP_OK  Success
 *     - ESP_ERR_TIMEOUT  BUSY signal line failed to return low
 *
 */
static esp_err_t pn5180_command(uint8_t *sendBuffer, size_t sendBufferLen, uint8_t *recvBuffer, size_t recvBufferLen);
static esp_err_t pn5180_txn(spi_device_handle_t dev, const void *tx, int txLen, void *rx, int rxLen);
static esp_err_t pn5180_busy_wait(uint32_t timeout);

///////////////
// Functions //
///////////////
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
  ESP_LOGD(TAG, "init: Initializing bus SPI...");
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
      .clock_speed_hz = 7000000,
      .mode = 0,
      .spics_io_num = PIN_NUM_NSS,
      .queue_size = 4,
      .pre_cb = NULL,
  };

  // Apply settings
  ESP_ERROR_CHECK(spi_bus_initialize(ESP32_HOST, &pn5180_buscfg, 1));
  ESP_ERROR_CHECK(spi_bus_add_device(ESP32_HOST, &pn5180_devcfg, &nfc));
  ESP_LOGI(TAG, "init: Bus SPI Initialized");

  // Reset device
  pn5180_reset();

  // Configure and turn on RF
  ESP_ERROR_CHECK(pn5180_setupRF());
}

static esp_err_t pn5180_command(uint8_t *sendBuffer, size_t sendBufferLen, uint8_t *recvBuffer, size_t recvBufferLen) {
  esp_err_t ret;
  ////////////////////
  // Initialization //
  ////////////////////
  ESP_LOGD(TAG, "command: Write, wait for busy...");
  ret = pn5180_busy_wait(1000); // 1.
  if(ret == ESP_ERR_TIMEOUT){
    ESP_LOGE(TAG, "command: BUSY signal line timeout");
    return ret;
  }
  ESP_LOGD(TAG, "command: SPI transaction: write %d read %d", sendBufferLen, recvBufferLen);
  
  //////////////////
  // Send command //
  //////////////////
  ESP_LOGD(TAG, "command: Write data:");
  ESP_LOG_BUFFER_HEX_LEVEL(TAG, sendBuffer, sendBufferLen, ESP_LOG_DEBUG);
  ret = pn5180_txn(nfc, sendBuffer, sendBufferLen, NULL, 0); // 2. 3. 4.
  ESP_ERROR_CHECK(ret);

  // Finish if write-only command
  if ((0 == recvBuffer) || (0 == recvBufferLen)) return ret;

  ESP_LOGD(TAG, "command: Read, wait for busy...");
  ret = pn5180_busy_wait(1000); // 5.
  if(ret == ESP_ERR_TIMEOUT){
    ESP_LOGE(TAG, "command: BUSY signal line timeout");
    return ret;
  }
  memset(recvBuffer, 0xFF, recvBufferLen);

  //////////////////////
  // Receive Response //
  //////////////////////
  ret = pn5180_txn(nfc, recvBuffer, recvBufferLen, recvBuffer, recvBufferLen); // 6. 7. 8.
  ESP_ERROR_CHECK(ret);

  ESP_LOGD(TAG, "command: Read data:\n");
  ESP_LOG_BUFFER_HEX_LEVEL(TAG, recvBuffer, recvBufferLen, ESP_LOG_DEBUG);
  
  return ret;
}

/*
 * The BUSY signal is used to indicate that the PN5180 is not able to send or receive data
 * over the SPI interface
 */
static esp_err_t pn5180_busy_wait(uint32_t timeout){
  uint32_t retries = timeout / 10;
  while (gpio_get_level(PIN_NUM_BUSY) && retries > 0){
    vTaskDelay(pdMS_TO_TICKS(10));
    retries--;
  }
  if(gpio_get_level(PIN_NUM_BUSY)){
    ESP_LOGE(TAG, "busy_wait: Timeout waiting for BUSY pin LOW");
    return ESP_ERR_TIMEOUT;
  }
  return ESP_OK;
}

esp_err_t pn5180_reset(void) {
  uint32_t retries = 10;
  gpio_set_level(PIN_NUM_RST, 0);
  vTaskDelay(pdMS_TO_TICKS(1));
  gpio_set_level(PIN_NUM_RST, 1);
  while((PN5180_IDLE_IRQ_STAT & pn5180_getIRQStatus()) == 0 && retries > 0){
    vTaskDelay(pdMS_TO_TICKS(10));
    retries--;
  }
  if((PN5180_IDLE_IRQ_STAT & pn5180_getIRQStatus()) == 0){
    ESP_LOGE(TAG, "reset: Timeout waiting for IRQ state IDLE");
    return ESP_ERR_TIMEOUT;
  }
  return ESP_OK;
}

static esp_err_t pn5180_txn(spi_device_handle_t dev, const void *tx, int txLen, void *rx, int rxLen) {
    spi_transaction_t txn = {
        .length = txLen * 8,
        .tx_buffer = tx,
        .rxlength = rxLen * 8,
        .rx_buffer = rx,
    };
    return spi_device_transmit(dev, &txn);
}

/*
 * WRITE_REGISTER - 0x00
 */
esp_err_t pn5180_writeRegister(uint8_t reg, uint32_t value) {
  uint8_t *p = (uint8_t*)&value;
  uint8_t buf[6] = { PN5180_WRITE_REGISTER, reg, p[0], p[1], p[2], p[3] };
  return pn5180_command(buf, 6, 0, 0);
}

/*
 * WRITE_REGISTER_OR_MASK - 0x01
 */
esp_err_t pn5180_writeRegisterWithOrMask(uint8_t reg, uint32_t mask) {
  uint8_t *p = (uint8_t*)&mask;
  uint8_t buf[6] = { PN5180_WRITE_REGISTER_OR_MASK, reg, p[0], p[1], p[2], p[3] };
  return pn5180_command(buf, 6, 0, 0);
}

/*
 * WRITE_REGISTER_AND_MASK - 0x02
 */
esp_err_t pn5180_writeRegisterWithAndMask(uint8_t reg, uint32_t mask) {
  uint8_t *p = (uint8_t*)&mask;
  uint8_t buf[6] = { PN5180_WRITE_REGISTER_AND_MASK, reg, p[0], p[1], p[2], p[3] };
  return pn5180_command(buf, 6, 0, 0);
}

/*
 * READ_REGISTER - 0x04
 */
esp_err_t pn5180_readRegister(uint8_t reg, uint32_t *value) {
  uint8_t cmd[2] = { PN5180_READ_REGISTER, reg };
  return pn5180_command(cmd, 2, (uint8_t*)value, 4);
}

/*
 * WRITE_EEPROM - 0x06
 */
esp_err_t pn5180_writeEEprom(uint8_t addr, uint8_t *buffer, uint16_t len) {
  if ((addr > 254) || ((addr+len) > 254)) {
    ESP_LOGE(TAG, "writeEEprom: Size of tx buffer exceeds 253 bytes");
    return ESP_ERR_INVALID_SIZE;
  }
	uint8_t cmd[len + 2];
	cmd[0] = PN5180_WRITE_EEPROM;
	cmd[1] = addr;
	for (int i = 0; i < len; i++) cmd[2 + i] = buffer[i];
	return pn5180_command(cmd, len + 2, 0, 0);
}

/*
 * READ_EEPROM - 0x07
 */
esp_err_t pn5180_readEEprom(uint8_t addr, uint8_t *buffer, uint16_t len) {
  if ((addr > 254) || ((addr+len) > 254)) {
    ESP_LOGE(TAG, "readEEprom: Size of rx buffer exceeds 253 bytes");
    return ESP_ERR_INVALID_SIZE;
  }
  uint8_t cmd[3] = { PN5180_READ_EEPROM, addr, (uint8_t)(len) };
  return pn5180_command(cmd, 3, buffer, len);
}

uint32_t pn5180_getIRQStatus() {
  uint32_t irqStatus;
  pn5180_readRegister(PN5180_IRQ_STATUS, &irqStatus);
  return irqStatus;
}

esp_err_t pn5180_clearIRQStatus(uint32_t irqMask) {
  return pn5180_writeRegister(PN5180_IRQ_CLEAR, irqMask);
}

/*
 * LOAD_RF_CONFIG - 0x11
 */
esp_err_t pn5180_loadRFConfig(uint8_t txConf, uint8_t rxConf) {
  uint8_t cmd[3] = { PN5180_LOAD_RF_CONFIG, txConf, rxConf };
  return pn5180_command(cmd, 3, 0, 0);
}

/*
 * RF_ON - 0x16
 */
esp_err_t pn5180_setRF_on() {
  uint8_t cmd[2] = { PN5180_RF_ON, 0x00 };
  pn5180_command(cmd, 2, 0, 0);

  uint8_t retries = 50;
  while (0 == (PN5180_TX_RFON_IRQ_STAT & pn5180_getIRQStatus()) && retries > 0) {   // wait for RF field to set up (max 500ms)
    vTaskDelay(pdMS_TO_TICKS(10));
	  retries--;
  }
  ESP_LOGD(TAG, "setRF_on: IRQ State after set - %ld", pn5180_getIRQStatus());
  if(0 == (PN5180_TX_RFON_IRQ_STAT & pn5180_getIRQStatus())){
    ESP_LOGE(TAG, "setRF_on: Failed to detect IRQ state TX_RFON");
    return ESP_FAIL;
  }

  pn5180_clearIRQStatus(PN5180_TX_RFON_IRQ_STAT);
  return ESP_OK;
}

/*
 * RF_OFF - 0x17
 */
esp_err_t pn5180_setRF_off() {

  uint8_t cmd[2] = { PN5180_RF_OFF, 0x00 };
  pn5180_command(cmd, 2, 0, 0);

  uint8_t retries = 50;
  while (0 == (PN5180_TX_RFOFF_IRQ_STAT & pn5180_getIRQStatus()) && retries > 0) {   // wait for RF field to set up (max 500ms)
    vTaskDelay(pdMS_TO_TICKS(10));
	  retries--;
  }
  if(0 == (PN5180_TX_RFOFF_IRQ_STAT & pn5180_getIRQStatus())) return ESP_FAIL;

  pn5180_clearIRQStatus(PN5180_TX_RFOFF_IRQ_STAT);
  return ESP_OK;
}; 

/*
 * SEND_DATA - 0x09
 */
esp_err_t pn5180_sendData(uint8_t *data, uint16_t len, uint8_t validBits) {
  if (len > 260){
    ESP_LOGE(TAG, "sendData: Length of data exceeds 260 bytes");
    return ESP_ERR_INVALID_SIZE;
  }

  uint8_t buffer[len+2];
  buffer[0] = PN5180_SEND_DATA;
  buffer[1] = validBits; // number of valid bits of last byte are transmitted (0 = all bits are transmitted)
  for (int i=0; i<len; i++) {
    buffer[2+i] = data[i];
  }

  pn5180_writeRegisterWithAndMask(PN5180_SYSTEM_CONFIG, 0xfffffff8);  // Idle/StopCom Command
  pn5180_writeRegisterWithOrMask(PN5180_SYSTEM_CONFIG, 0x00000003);   // Transceive Command
  vTaskDelay(pdMS_TO_TICKS(10));
  /*
   * Transceive command; initiates a transceive cycle.
   * Note: Depending on the value of the Initiator bit, a
   * transmission is started or the receiver is enabled
   * Note: The transceive command does not finish
   * automatically. It stays in the transceive cycle until
   * stopped via the IDLE/StopCom command
   */

  uint32_t rfStatus;
  if (pn5180_readRegister(PN5180_RF_STATUS, &rfStatus) != ESP_OK) {
    ESP_LOGE(TAG, "sendData: Failed to read RF_STATUS register.");
    return ESP_ERR_INVALID_RESPONSE;
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
  ESP_LOGD(TAG, "sendData: rfStatus=%ld", rfStatus);
  PN5180TransceiveState_t state = ((rfStatus >> 24) & 0x07);

  ESP_LOGD(TAG,"sendData: state=%d",(uint8_t)(state));
  if (PN5180_TS_WaitTransmit != state){
    ESP_LOGE(TAG, "sendData: TransceiveState not WaitTransmit");
    return ESP_ERR_INVALID_STATE;
  }

  return pn5180_command(buffer, len+2, 0, 0);
}

/*
 * READ_DATA - 0x0A
 */
uint8_t* pn5180_readData(int len) {
  if (len > 508) return 0L;

  uint8_t cmd[2] = { PN5180_READ_DATA, 0x00 };
  pn5180_command(cmd, 2, readBuffer, len);

  return readBuffer;
}

#ifdef FALSE
/*
 * Get TRANSCEIVE_STATE from RF_STATUS register
 */

PN5180TransceiveState_t getTransceiveState() {
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
  uint32_t rfStatus;
  if (!pn5180_readRegister(PN5180_RF_STATUS, &rfStatus)) {
    return PN5180_TS_Idle;
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
  ESP_LOGI(TAG, "getTransceiveState: rfStatus=%ld", rfStatus);
  uint8_t state = ((rfStatus >> 24) & 0x07);
  return state;
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

esp_err_t pn5180_readData(uint8_t len, uint8_t *buffer) {
	if (len > 508) {
		return ESP_ERR_INVALID_SIZE;
	}
	uint8_t cmd[2] = { PN5180_READ_DATA, 0x00 };
	return pn5180_command(cmd, 2, buffer, len);;
}
#endif

/* prepare LPCD registers */
esp_err_t pn5180_prepareLPCD() {
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
  vTaskDelay(pdMS_TO_TICKS(100));
  return ESP_OK;
}

/* switch the mode to LPCD (low power card detection)
 * Parameter 'wakeupCounterInMs' must be in the range from 0x0 - 0xA82
 * max. wake-up time is 2960 ms.
 */
esp_err_t pn5180_switchToLPCD(uint16_t wakeupCounterInMs) {
  // clear all IRQ flags
  pn5180_clearIRQStatus(0xffffffff); 
  // enable only LPCD and general error IRQ
  pn5180_writeRegister(PN5180_IRQ_ENABLE, PN5180_LPCD_IRQ_STAT | PN5180_GENERAL_ERROR_IRQ_STAT);  
  // switch mode to LPCD 
  uint8_t cmd[4] = { PN5180_SWITCH_MODE, 0x01, (uint8_t)(wakeupCounterInMs & 0xFF), (uint8_t)((wakeupCounterInMs >> 8U) & 0xFF) };
  return pn5180_command(cmd, sizeof(cmd), 0, 0);
}

void printIRQStatus(uint32_t irqStatus) {
  char states[255] = "";
  if (irqStatus & (1<< 0)) strcat(states,"RX ");
  if (irqStatus & (1<< 1)) strcat(states,"TX ");
  if (irqStatus & (1<< 2)) strcat(states,"IDLE ");
  if (irqStatus & (1<< 3)) strcat(states,"MODE_DETECTED ");
  if (irqStatus & (1<< 4)) strcat(states,"CARD_ACTIVATED ");
  if (irqStatus & (1<< 5)) strcat(states,"STATE_CHANGE ");
  if (irqStatus & (1<< 6)) strcat(states,"RFOFF_DET ");
  if (irqStatus & (1<< 7)) strcat(states,"RFON_DET ");
  if (irqStatus & (1<< 8)) strcat(states,"TX_RFOFF ");
  if (irqStatus & (1<< 9)) strcat(states,"TX_RFON ");
  if (irqStatus & (1<<10)) strcat(states,"RF_ACTIVE_ERROR ");
  if (irqStatus & (1<<11)) strcat(states,"TIMER0 ");
  if (irqStatus & (1<<12)) strcat(states,"TIMER1 ");
  if (irqStatus & (1<<13)) strcat(states,"TIMER2 ");
  if (irqStatus & (1<<14)) strcat(states,"RX_SOF_DET ");
  if (irqStatus & (1<<15)) strcat(states,"RX_SC_DET ");
  if (irqStatus & (1<<16)) strcat(states,"TEMPSENS_ERROR ");
  if (irqStatus & (1<<17)) strcat(states,"GENERAL_ERROR ");
  if (irqStatus & (1<<18)) strcat(states,"HV_ERROR ");
  if (irqStatus & (1<<19)) strcat(states,"LPCD ");
  ESP_LOGI(TAG,"IRQ_Status: %s",states);
}