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

 /*
  * GPIO 0 - Pulled high on dev boards. If pulled low during boot while GPIO 2 is low/NC, will trigger serial bootloader. DO NOT USE.
  * GPIO 1 - USB TX on dev boards, DO NOT USE.
  * GPIO 2 - Pulled low on dev boards. If pulled low/NC during boot while GPIO 0 is low, will trigger serial bootloader. Use caution.
  * GPIO 3 - USB RX on dev boards, DO NOT USE.
  * GPIO 4 - OK
  * GPIO 5 - If pulled high/low on boot, can alter SDIO Slave timing. Use caution.
  * GPIO 6-11 - Connceted to internal FLASH. DO NOT USE. 
  * GPIO 12 - Pulled low by default. If pulled high during boot, device will fail to start. Use caution.
  * GPIO 13-15 - OK
  * GPIO 16 - OK, UART2_RX
  * GPIO 17 - OK, UART2_TX
  * GPIO 18-19 - OK
  * GPIO 20 - Not usable in any board. DO NOT USE.
  * GPIO 21 - OK, I2C_SDA
  * GPIO 22 - OK, I2C_SCL
  * GPIO 23 - OK
  * GPIO 24 - Not usable in any board. DO NOT USE.
  * GPIO 25-27 - OK
  * GPIO 28-31 - Not usable in any board. DO NOT USE.
  * GPIO 32-33 - OK
  * GPIO 34-36 - OK, input only.
  * GPIO 37-38 - Not usable in any board. DO NOT USE.
  * GPIO 39 - OK, input only.
 */

extern SemaphoreHandle_t puzzle_task_sem;
extern QueueHandle_t puzzle_task_queue;
extern SemaphoreHandle_t rx_payload_sem;
extern uint8_t rx_payload[7];

SemaphoreHandle_t gpio_task_sem;
QueueHandle_t gpio_task_queue;
uint64_t mask_protect = 0b1111111111111111111111111110000011110001000100000000111111001011; // Default DNU pins implemented

void GPIO_Init(){
    static const char* TAG = "GPIO";
    if(CONFIG_ENABLE_SOUND){
        ESP_LOGI(TAG, "Protecting sound pins");
        mask_protect |= 1ULL << UART_TX_GPIO;
        mask_protect |= 1ULL << UART_RX_GPIO;
    }
    if(CONFIG_ENABLE_CAN){
        ESP_LOGI(TAG, "Protecting CAN pins");
        mask_protect |= 1ULL << CAN_TX_GPIO;
        mask_protect |= 1ULL << CAN_RX_GPIO;
    }
    if(CONFIG_ENABLE_SHIFT){
        ESP_LOGI(TAG, "Protecting shift register pins");
        mask_protect |= 1ULL << SHIFT_CLOCK_GPIO;
        mask_protect |= 1ULL << PISO_LOAD_GPIO;
        mask_protect |= 1ULL << PISO_DATA_GPIO;
        mask_protect |= 1ULL << SIPO_LATCH_GPIO;
        mask_protect |= 1ULL << SIPO_DATA_GPIO;
    }

    gpio_task_queue = xQueueCreate(10, sizeof(gpio_task_action_t));
    gpio_task_sem = xSemaphoreCreateCounting( 10, 0 ); 
    xTaskCreatePinnedToCore(gpio_task, "GPIO", 2048, NULL, GENERIC_TASK_PRIO, NULL, tskNO_AFFINITY);
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ~(mask_protect);
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Setup complete");
}

void gpio_task(void *arg){
    gpio_task_action_t gpio_action;
    uint64_t gpio_mask = mask_protected; // Mask applied to select GPIO mins to modify
    uint64_t gpio_step = 0;              // Temp variable to process intermediate actions in each command
    uint64_t gpio_states = 0;            // Initial state of pins
    while(1){
        xSemaphoreTake(gpio_task_sem, portMAX_DELAY); // Blocked from executing until puzzle_task gives a semaphore
        xQueueReceive(gpio_task_queue, &gpio_action, pdMS_TO_TICKS(10)); // Pull task from queue
        switch(gpio_action){
            case SET_MASK:
                gpio_mask = 0;
                gpio_step = 0;
                for(int i=0; i<6; i++){
                    gpio_step = rx_payload[i+1]; // Copy mask byte
                    gpio_step << (i * 8); // Shift byte to correct position
                    gpio_mask |= gpio_step; // Insert mask byte into mask
                }
                gpio_mask &= ~(mask_protected); // Eliminate any pins from other components
                ESP_LOGI(TAG, "Set mask");
                break;
            case SET_STATES:
                gpio_step = 0;
                gpio_states &= ~(gpio_mask); // Initialize setting states by clearing masked pin states
                for(int i=0; i<6; i++){
                    gpio_step = rx_payload[i+1]; // Copy a byte of pins to modify
                    gpio_step << (i * 8);        // Shift byte into correct position
                    gpio_step &= ~(gpio_mask);   // Eliminate unmasked pins
                    gpio_states |= gpio_step;    // Apply states to pins
                }
                ESP_LOGI(TAG, "Set output states");
                break;
            case SEND_STATES:                                      // Command: Send states to CAN_TX payload
                xSemaphoreTake(tx_payload_sem, portMAX_DELAY);     // Blocked from executing until tx_payload is available
                gpio_step = gpio_states;
                for(int i=0; i<6; i++){
                    tx_payload[i+2] = (uint8_t)(gpio_step & 0xFF); // Transfer the current state byte to the CAN_TX payload
                    gpio_step >> 8;                                // Remove the transferred byte
                }
                break;
        }
        xSemaphoreGive(rx_payload_sem); // Give control of rx_payload to rx_task
        //vTaskDelay(portMAX_DELAY / portTICK_PERIOD_MS); // Halt forever
    }
}