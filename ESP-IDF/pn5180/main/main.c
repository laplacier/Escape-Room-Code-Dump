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
    uint8_t uid[8];
    //struct ISO15693UID uid;
    ISO15693ErrorCode_t rc = pn5180_getInventory(uid);
    if (ISO15693_EC_OK != rc) {
      iso15693_printError(rc);
    }
    else{
      iso15693_printUID(uid, 8);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

    uint8_t blockSize, numBlocks;
    rc = pn5180_getSystemInfo(uid, &blockSize, &numBlocks);
    if (ISO15693_EC_OK != rc) {
      iso15693_printError(rc);
    }
    else{
      ESP_LOGI(TAG, "System info retrieved: blockSize=%d, numBlocks=%d", blockSize, numBlocks);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    uint8_t readBuffer[blockSize];
    for (int i=0; i<numBlocks; i++) {
      rc = pn5180_readSingleBlock(uid, i, readBuffer, blockSize);
      if (ISO15693_EC_OK != rc) {
        ESP_LOGE(TAG, "Error in readSingleBlock #%d:", i);
        iso15693_printError(rc);
      }
      else{
        ESP_LOGI(TAG, "Reading block#%d", i);
        iso15693_printGeneric(TAG, readBuffer, blockSize);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}