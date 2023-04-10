#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

TaskHandle_t sound_task_handle;

void sendAudioCommand(uint8_t command, uint16_t parameter){
}

void sound_task(void *arg){
  while(1)
    vTaskDelay(portMAX_DELAY / portTICK_PERIOD_MS); // Halt forever
}

void sound_init(void) 
{
  // Create minimum tasks/queues/semaphores to allow code cohesion 
  xTaskCreatePinnedToCore(sound_task, "sound", 2048, NULL, 0, &sound_task_handle, tskNO_AFFINITY);
  ESP_LOGI("Sound", "Disabled, no setup todo!");
}