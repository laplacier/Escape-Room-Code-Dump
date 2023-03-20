#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "pn5180.h"

extern uint8_t pn5180_rxBuffer[508];
void showIRQStatus(uint32_t irqStatus);

void app_main(void)
{
    pn5180_init();

    while(1){
        //uint32_t irq_state = 0;
        //pn5180_command(cmd, 2, (uint8_t*)&irq_state, 4);
        //showIRQStatus(irq_state);
        //vTaskDelay(pdMS_TO_TICKS(1000));
        uint8_t version[2];
        uint8_t cmd[3] = { PN5180_READ_EEPROM, PN5180_PRODUCT_VERSION, sizeof(version) };
        pn5180_command(cmd, 3, version, sizeof(version));
        ESP_LOGI("NFC","Product version: %d.%d",version[1],version[0]);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    //ESP_LOGI("NFC","Firmware Version: %d.%d",pn5180_rxBuffer[1],pn5180_rxBuffer[0]);
}

void showIRQStatus(uint32_t irqStatus) {
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
  ESP_LOGI("NFC","IRQ_Status: %s",states);
}