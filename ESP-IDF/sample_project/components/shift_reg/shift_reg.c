#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "puzzle.h"
#include "twai_can.h"
#include "shift_reg.h"
#include "sound.h"
#include "gpio_prop.h"

extern SemaphoreHandle_t puzzle_task_sem;
extern QueueHandle_t puzzle_task_queue;
extern SemaphoreHandle_t tx_payload_sem;
extern SemaphoreHandle_t rx_payload_sem;
extern uint8_t tx_payload[8];
extern uint8_t rx_payload[8];

TaskHandle_t sipo_task_handle;
SemaphoreHandle_t sipo_task_sem;
QueueHandle_t sipo_task_queue;

uint8_t dataOut[NUM_SIPO]; // Data sent to SIPO registers
uint8_t maskSIPO[NUM_SIPO];
uint8_t dataIn[NUM_PISO]; // Data read from PISO registers

void shift_init(void){   
    // Create shift register tasks, queues, semaphores
    sipo_task_queue = xQueueCreate(10, sizeof(sipo_task_action_t));
    sipo_task_sem = xSemaphoreCreateCounting( 10, 0 );
    xTaskCreatePinnedToCore(sipo_task, "SIPO", 2048, NULL, GENERIC_TASK_PRIO, NULL, tskNO_AFFINITY);
    ESP_LOGI("Shift_Reg", "Setup complete");
}

void pulsePin(uint8_t pinName, uint32_t pulseTime){
  gpio_write(pinName, 0);
  esp_rom_delay_us(pulseTime);
  gpio_write(pinName, 1);
  esp_rom_delay_us(pulseTime);
}

void piso_update(void){
    gpio_write(SHIFT_CLOCK_GPIO, 1);               // Data is shifted when clock pin changes from LOW to HIGH, so ensure it starts HIGH
    pulsePin(PISO_LOAD_GPIO, 5);                             // Pulse load pin to snapshot all PISO pin states and start at the beginning
    for(int i=0; i<NUM_PISO; i++){                     // For each PISO register...
        for(int j=7; j>=0; j--){                        // For each bit in the PISO register...
        bitWrite(dataIn[i], j, gpio_read(PISO_DATA_GPIO)); // Write the current bit to the corresponding bit in the input variable
        pulsePin(SHIFT_CLOCK_GPIO, 5);                        // Pulse the clock to shift the next bit in from the PISO register
        }
    }
}

void sipo_write(uint8_t piso_num, uint8_t pin, bool val){
    if(piso_num > NUM_PISO){
        ESP_LOGE("Shift_Reg", "Attempted to write to PISO that does not exist");
    }
    else{
        bitWrite(dataOut[piso_num], pin, val);
        sipo_update();
    }
}

void sipo_update(void){
    for(int i=NUM_SIPO-1; i>=0; i--){
        for(int j=7; j>=0; j--){
            gpio_write(SIPO_DATA_GPIO, bitRead(dataOut[i], j));
            pulsePin(SHIFT_CLOCK_GPIO, 5);
        }
    }
    pulsePin(SIPO_LATCH_GPIO, 5);
}

void sipo_task(void *arg){
    sipo_task_action_t sipo_action;
    static const char* TAG = "Shift_SIPO";
    while(1){
        if(xSemaphoreTake(sipo_task_sem, pdMS_TO_TICKS(10)) == pdTRUE){ // Blocked from executing until puzzle_task gives a semaphore
            xQueueReceive(sipo_task_queue, &sipo_action, portMAX_DELAY); // Pull task from queue
            switch(sipo_action){
                case SET_SIPO_MASK:
                    for(int i=0; i<NUM_SIPO; i++){
                        maskSIPO[i] = rx_payload[i+1]; // Copy byte of mask to now empty byte in var
                    }
                    ESP_LOGI(TAG, "Set mask");
                    xSemaphoreGive(rx_payload_sem); // Give control of rx_payload to rx_task
                    break;
                case SET_SIPO_STATES:
                    for(int i=0; i<NUM_SIPO; i++){
                        dataOut[i] &= ~(maskSIPO[i]);
                        dataOut[i] |= (rx_payload[i+1] & maskSIPO[i]);
                    }
                    sipo_update();
                    ESP_LOGI(TAG, "Set output states");
                    xSemaphoreGive(rx_payload_sem); // Give control of rx_payload to rx_task
                    break;
                case SEND_SIPO_STATES:                                      // Command: Send states to CAN_TX payload
                    xSemaphoreTake(tx_payload_sem, portMAX_DELAY);     // Blocked from continuing until tx_payload is available
                    tx_payload[1] = 5;
                    for(int i=0; i<NUM_SIPO; i++){
                        tx_payload[i+2] = dataOut[i]; // Transfer the current state bytes to the CAN_TX payload
                    }
                    break;
                case SEND_PISO_STATES:                                      // Command: Send states to CAN_TX payload
                    xSemaphoreTake(tx_payload_sem, portMAX_DELAY);     // Blocked from continuing until tx_payload is available
                    tx_payload[1] = 6;
                    for(int i=0; i<NUM_PISO; i++){
                        tx_payload[i+2] = dataIn[i]; // Transfer the current state bytes to the CAN_TX payload
                    }
                    break;
            }
        }
        
    }
}