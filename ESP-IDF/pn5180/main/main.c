#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "iso15693.h"
#include "pn5180.h"
static const char* TAG = "main.c";
void showIRQStatus(uint32_t irqStatus);
void printUID(const char* tag, uint8_t* uid, uint8_t len);

extern const char afi_string[14][30];
extern const char manufacturerCode[110][100];

//#define WRITE_ENABLED 1
bool flag_written = 0;

void app_main(void)
{
  ESP_LOGI(TAG, "pn5180 example using ISO15493 NFC tags");

  pn5180_init(); // Initialize SPI communication, set up ISO15693, activate RF

  // PN5180 Product version
  uint8_t product[2];
  pn5180_readEEprom(PN5180_PRODUCT_VERSION, product, 2);
  if(0xff == product[1]){
    ESP_LOGE(TAG, "Initialization failed. Reset to restart.");
    while(1) vTaskDelay(portMAX_DELAY);
  }
  ESP_LOGI(TAG,"Product version: %d.%d",product[1],product[0]);

  
  // PN5180 Firmware version
  uint8_t firmware[2];
  pn5180_readEEprom(PN5180_FIRMWARE_VERSION, firmware, 2);
  ESP_LOGI(TAG,"Firmware version: %d.%d",firmware[1],firmware[0]);

  // PN5180 EEPROM version
  uint8_t eeprom[2];
  pn5180_readEEprom(PN5180_EEPROM_VERSION, eeprom, 2);
  ESP_LOGI(TAG,"EEPROM version: %d.%d",eeprom[1],eeprom[0]);

  pn5180_setRF_off();
  ESP_LOGI(TAG, "Starting read cycle...");
  vTaskDelay(pdMS_TO_TICKS(1000));

  ISO15693NFC_t nfc;
  ISO15693Inventory_t inventory;
  inventory.numCard = 1;
  uint32_t pollCount = 1;
  while(1){
    // Multiple inventory
    ESP_LOGI(TAG, "Poll #%ld", pollCount);
    ISO15693ErrorCode_t rc = pn5180_getInventoryMultiple(&inventory);
    if (ISO15693_EC_OK != rc) {
      iso15693_printError(rc);
    }
    else if(!inventory.numCard){
      ESP_LOGI(TAG, "No cards detected.");
    }
    else{
      ESP_LOGI(TAG, "Discovered %d cards.", inventory.numCard);
      for(int i=0; i<inventory.numCard; i++){
        printUID(TAG, inventory.uid_raw[i], sizeof(inventory.uid_raw[i]));
        ESP_LOGI(TAG, "Manufacturer=%s", manufacturerCode[inventory.manufacturer[i]]);
      }
    }
    pollCount++;
  }
  /*while(1){
    // Inventory from NFC tag
    ISO15693ErrorCode_t rc = pn5180_getInventoryMultiple(&nfc);
    if (ISO15693_EC_OK != rc) {
      iso15693_printError(rc);
    }
    else{
      printUID(TAG, nfc.uid_raw, sizeof(nfc.uid_raw));
      ESP_LOGI(TAG, "Manufacturer=%s", manufacturerCode[nfc.manufacturer]);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

    // System information from NFC tag
    rc = pn5180_getSystemInfo(&nfc);
    if (ISO15693_EC_OK != rc) {
      iso15693_printError(rc);
    }
    else{
      ESP_LOGI(TAG, "System info retrieved: DSFID=%d, AFI=%s, blockSize=%d, numBlocks=%d, IC Ref=%d", nfc.dsfid, afi_string[nfc.afi], nfc.blockSize, nfc.numBlocks, nfc.ic_ref);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Read blocks one at a time
    for (int i=0; i<nfc.numBlocks; i++) {
      rc = pn5180_readSingleBlock(&nfc, i);
      if (ISO15693_EC_OK != rc) {
        ESP_LOGE(TAG, "Error in readSingleBlock #%d:", i);
        iso15693_printError(rc);
        break;
      }
      else{
        ESP_LOGI(TAG, "Reading block#%d", i);
        iso15693_printGeneric(TAG, nfc.blockData, nfc.blockSize, i);
      }
    }
    ESP_LOGI(TAG, "Reading multiple blocks #0-%d", nfc.numBlocks-1);
    rc = pn5180_readMultipleBlock(&nfc, 0, nfc.numBlocks);
    if (ISO15693_EC_OK != rc) {
      ESP_LOGE(TAG, "Error in readMultipleBlock #0-%d:", nfc.numBlocks-1);
      iso15693_printError(rc);
    }
    else{
      for(int i=0; i<nfc.numBlocks; i++){
        iso15693_printGeneric(TAG, nfc.blockData, nfc.blockSize, i);
      }
    }

#ifdef WRITE_ENABLED
    // This loop writes to the NFC tag one time per reset
    if(!flag_written){
      uint8_t dataFiller = 0;
      for (int i=0; i<nfc.numBlocks; i++) {
        for (int j=0; j<nfc.blockSize; j++) {
          nfc.blockData[(nfc.blockSize*i) + j] = dataFiller++;
        }
        rc = pn5180_writeSingleBlock(&nfc, i);
        if (ISO15693_EC_OK == rc) {
          ESP_LOGI(TAG, "Wrote block #%d", i);
        }
        else {
          ESP_LOGE(TAG, "Error in writeSingleBlock #%d: ", i);
          iso15693_printError(rc);
          break;
        }
      }
      flag_written = 1;
    }
#endif // WRITE_ENABLED
    vTaskDelay(pdMS_TO_TICKS(5000));
  }*/
}

void printUID(const char* tag, uint8_t* uid, uint8_t len){
  printf("\033[32mI (%ld) %s: UID=", esp_log_timestamp(), tag);
  for(int i=7; i>=0; i--){
    if(uid[i] < 16) printf("0");
    printf("%X", uid[i]);
    if(i > 0) printf(":");
  }
  printf("\n\033[0m");
}
