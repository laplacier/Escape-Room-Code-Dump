#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "esp_rom_sys.h"
#include "puzzle.h"
#include "twai_can.h"
#include "sound.h"
#include "shift_reg.h"
#include "gpio_prop.h"

//---------External Global Variables-----------//

//twai_can
extern QueueHandle_t ctrl_task_queue;
extern SemaphoreHandle_t ctrl_task_sem;
extern SemaphoreHandle_t rx_payload_sem;
extern uint8_t rx_payload[8];
extern uint8_t tx_payload[8];

//sound
extern TaskHandle_t sound_task_handle;

//gpio
extern QueueHandle_t gpio_task_queue;
extern SemaphoreHandle_t gpio_task_sem;

//--------------Global Variables---------------//
SemaphoreHandle_t puzzle_task_sem;
QueueHandle_t puzzle_task_queue;
uint8_t game_state = 0;

void puzzle_init(void){
    puzzle_task_queue = xQueueCreate(1, sizeof(puzzle_task_action_t));
    puzzle_task_sem = xSemaphoreCreateCounting( 10, 0 );
    xTaskCreatePinnedToCore(puzzle_task, "Puzzle", 4096, NULL, PUZZLE_TASK_PRIO, NULL, tskNO_AFFINITY);
    gpio_mode(27,INPUT_PULLUP,0);
    ESP_LOGI("Puzzle", "Setup complete");
}

void puzzle_task(void *arg){
    static const char* TAG = "Puzzle";
    bool switch_state = 1;
    bool switch_old = 1;
    /*struct state {
        uint8_t val;
        const char *msg;
    };
    struct state states[] = {
        {0x00, "Idle/Inactive"},
        {0x01, "Stage 1"},
        {0x02, "Stage 2"},
        {0xFE, "Reset"},
        {0xFF, "Solved"}
    };*/
    for(;;){
        CAN_Receive(10);
        // The actual puzzle goes below
        switch_state = gpio_read(27);
        if(switch_state != switch_old){
            ESP_LOGI(TAG, "GPIO 27 %d", switch_state);
            switch_old = switch_state;
        }
    }
}

bool CAN_Receive(uint32_t delay){
    static const char* TAG = "Puzzle";
    static const char* cmd[4]= {"STATE","GPIO_MASK","GPIO","PLAY_SOUND"};
    puzzle_task_action_t puzzle_action;
    gpio_task_action_t gpio_action;
    if(xSemaphoreTake(puzzle_task_sem, pdMS_TO_TICKS(delay)) == pdTRUE){ // Blocked from executing until ctrl_task gives semaphore
        xQueueReceive(puzzle_task_queue, &puzzle_action, pdMS_TO_TICKS(delay)); // Pull task from queue
        if(puzzle_action == CMD){ // Received a forced state change from the CAN bus
            ESP_LOGI(TAG, "Command received from CAN bus: %s",cmd[rx_payload[0]]);
            switch(rx_payload[0]){
                case 0: // Change the game state
                    if(rx_payload[1] == game_state){ // Already in the requested state
                        ESP_LOGI(TAG, "Game already in requested state!");
                        xSemaphoreGive(rx_payload_sem); // Give control of rx_payload to rx_task
                    }
                    else{ // Otherwise, change to requested game state
                        game_state = rx_payload[1];
                        xSemaphoreGive(rx_payload_sem); // Give control of rx_payload to rx_task
                    }
                    break;
                case 1: // GPIO_Mask of pins to modify
                    gpio_action = SET_GPIO_MASK;
                    xQueueSend(gpio_task_queue, &gpio_action, portMAX_DELAY);
                    xSemaphoreGive(gpio_task_sem);
                    ESP_LOGI(TAG, "Sent mask to GPIO");
                    break;
                case 2: // GPIO states of pins to modify
                    gpio_action = SET_GPIO_STATES;
                    xQueueSend(gpio_task_queue, &gpio_action, portMAX_DELAY);
                    xSemaphoreGive(gpio_task_sem);
                    ESP_LOGI(TAG, "Sent states to GPIO");
                    break;
                case 3: // Play music
                    xTaskNotify(sound_task_handle,rx_payload[1],eSetValueWithOverwrite);
                    xSemaphoreGive(rx_payload_sem); // Give control of rx_payload to rx_task
                    break;
                default:
                    ESP_LOGI(TAG, "Unknown command");
            }
        }
        return 1;
    }
    else{
        return 0;
    }
}