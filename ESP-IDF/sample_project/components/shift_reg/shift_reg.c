#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "rom/ets_sys.h"
#include "puzzle.h"
#include "shift_reg.h"

TaskHandle_t piso_task_handle;
TaskHandle_t sipo_task_handle;

void shift_init(void) 
{   
    // Create shift register tasks
    xTaskCreatePinnedToCore(piso_task, "PISO", 2048, NULL, GENERIC_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(sipo_task, "SIPO", 2048, NULL, GENERIC_TASK_PRIO, NULL, tskNO_AFFINITY);
    ESP_LOGI("Shift_Reg", "Setup complete");
}

void sipo_task(void *arg){
    while(1){
        vTaskDelay(portMAX_DELAY / portTICK_PERIOD_MS); // Halt forever
    }
}

void piso_task(void *arg){
    while(1){
        vTaskDelay(portMAX_DELAY / portTICK_PERIOD_MS); // Halt forever
    }
}

void pulsePin(uint8_t pinName, uint32_t pulseTime){
  //digitalWrite(pinName, LOW);
  ets_delay_us(pulseTime);
  //digitalWrite(pinName, HIGH);
  ets_delay_us(pulseTime);
}