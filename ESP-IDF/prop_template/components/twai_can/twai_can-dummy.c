#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "twai_can.h"

QueueHandle_t ctrl_task_queue;
QueueHandle_t tx_task_queue;
SemaphoreHandle_t ctrl_task_sem;
SemaphoreHandle_t rx_task_sem;
SemaphoreHandle_t rx_payload_sem;
SemaphoreHandle_t tx_task_sem;

uint8_t rx_payload[8] = {0,0,0,0,0,0,0,0};

void ctrl_task(void *arg){
  while(1)
    vTaskDelay(portMAX_DELAY / portTICK_PERIOD_MS); // Halt forever
}

void rx_task(void *arg){
  while(1)
    vTaskDelay(portMAX_DELAY / portTICK_PERIOD_MS); // Halt forever
}

void tx_task(void *arg){
  while(1)
    vTaskDelay(portMAX_DELAY / portTICK_PERIOD_MS); // Halt forever
}

void fake_bus_task(void *arg){
  while(1)
    vTaskDelay(portMAX_DELAY / portTICK_PERIOD_MS); // Halt forever
}

void twai_can_init(void){
  // Create minimum tasks/queues/semaphores to allow code cohesion 
  ctrl_task_queue = xQueueCreate(1, sizeof(ctrl_task_action_t));
  ctrl_task_sem = xSemaphoreCreateBinary();
  rx_payload_sem = xSemaphoreCreateBinary();
  ESP_LOGI("TWAI_CAN", "Disabled, no setup todo!");
}