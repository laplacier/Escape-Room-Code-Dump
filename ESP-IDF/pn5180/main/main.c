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
static char* TAG = "main.c";
void showIRQStatus(uint32_t irqStatus);

void app_main(void)
{
  ESP_LOGI(TAG,"Free heap=%ld bytes", esp_get_free_heap_size());
  ESP_LOGI(TAG,"Free task heap=%d bytes", uxTaskGetStackHighWaterMark(NULL));
  pn5180_init();
  printIRQStatus(pn5180_getIRQStatus());
  vTaskDelay(pdMS_TO_TICKS(1000));
  uint8_t product[2];
  pn5180_readEEprom(PN5180_PRODUCT_VERSION, product, 2);
  ESP_LOGI(TAG,"Product version: %d.%d",product[1],product[0]);
  vTaskDelay(pdMS_TO_TICKS(1000));
  uint8_t firmware[2];
  pn5180_readEEprom(PN5180_FIRMWARE_VERSION, firmware, 2);
  ESP_LOGI(TAG,"Firmware version: %d.%d",firmware[1],firmware[0]);
  vTaskDelay(pdMS_TO_TICKS(1000));

  while(1){
    ISO15693NFC_t nfc;
    // Inventory from NFC tag
    ISO15693ErrorCode_t rc = pn5180_getInventory(&nfc);
    if (ISO15693_EC_OK != rc) {
      iso15693_printError(rc);
    }
    else{
      ESP_LOGI(TAG, "UID=%s, Manufacturer=%s", nfc.uid, nfc.manufacturer);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

    // System information from NFC tag
    rc = pn5180_getSystemInfo(&nfc);
    if (ISO15693_EC_OK != rc) {
      iso15693_printError(rc);
    }
    else{
      ESP_LOGI(TAG, "System info retrieved: DSFID=%d, AFI=%s, blockSize=%d, numBlocks=%d, IC Ref=%d", nfc.dsfid, nfc.afi, nfc.blockSize, nfc.numBlocks, nfc.ic_ref);
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
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}