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

SemaphoreHandle_t puzzle_task_sem;
QueueHandle_t puzzle_task_queue;

extern QueueHandle_t ctrl_task_queue;
extern SemaphoreHandle_t ctrl_task_sem;
extern SemaphoreHandle_t rx_payload_sem;
extern TaskHandle_t sound_task_handle;
extern uint8_t rx_payload[7];

void puzzle_init(void){
    puzzle_task_queue = xQueueCreate(1, sizeof(puzzle_task_action_t));
    puzzle_task_sem = xSemaphoreCreateCounting( 10, 0 );
    xTaskCreatePinnedToCore(puzzle_task, "Puzzle", 4096, NULL, PUZZLE_TASK_PRIO, NULL, tskNO_AFFINITY);
    ESP_LOGI("Puzzle", "Setup complete");
}

void puzzle_task(void *arg){
    puzzle_task_action_t action;
    static const char* TAG = "Puzzle";
    static const char* cmd[4]= {"STATE","GPIO_MASK","GPIO","PLAY_SOUND"};
    uint8_t game_state = 0;
    uint8_t gpio_mask[6] = {0,0,0,0,0,0};
    uint8_t gpio_mask_protected[6] = {0,0,0,0,0,0};
    uint8_t gpio_states[6] = {0,0,0,0,0,0};
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
        if(xSemaphoreTake(puzzle_task_sem, pdMS_TO_TICKS(10)) == pdTRUE){ // Blocked from executing until ctrl_task gives semaphore
            xQueueReceive(puzzle_task_queue, &action, pdMS_TO_TICKS(10)); // Pull task from queue
            if(action == CMD){ // Received a forced state change from the CAN bus
                ESP_LOGI(TAG, "Command received from CAN bus: %s",cmd[rx_payload[0]]);
                switch(rx_payload[0]){
                    case 0: // Change the game state
                        if(rx_payload[1] == game_state){ // Already in the requested state
                            ESP_LOGI(TAG, "Game already in requested state!");
                        }
                        else{ // Otherwise, change to requested game state
                            game_state = rx_payload[1];
                        }
                        break;
                    case 1: // GPIO_Mask of pins to modify
                        for(int i=0; i<6; i++){
                            gpio_mask[i] = rx_payload[i+1];
                        }
                        break;
                    case 2: // GPIO states of pins to modify
                        for(int i=0; i<6; i++){
                            rx_payload[i+1] &= gpio_mask[i];   // Read the requested pins to modify
                            gpio_states[i] &= ~(gpio_mask[i]); // Clear the pins to modify
                            gpio_states[i] |= rx_payload[i+1];   // Set the pins to their requested states
                        }
                        ESP_LOGI(TAG, "New pin states: 0x %x %x %x %x %x %x",gpio_states[0],gpio_states[1],gpio_states[2],gpio_states[3],gpio_states[4],gpio_states[5]);
                        break;
                    case 3: // Play music
                        xTaskNotify(sound_task_handle,rx_payload[1],eSetValueWithOverwrite);
                        break;
                    default:
                        ESP_LOGI(TAG, "Unknown command");
                }
                xSemaphoreGive(rx_payload_sem); // Give control of rx_payload to rx_task
            }
        }
        // The actual puzzle goes below
    }
}

void GPIO_Init(){
    static const char* TAG = "GPIO";
    uint64_t mask_protect = 0;
    if(CONFIG_ENABLE_SOUND){
        ESP_LOGI(TAG, "Protecting sound pins");
        mask_protect |= 1ULL << UART_TX_GPIO;
        mask_protect |= 1ULL << UART_RX_GPIO;
    }
    if(CONFIG_ENABLE_SHIFT){
        ESP_LOGI(TAG, "Protecting shift register pins");
        mask_protect |= 1ULL << UART_TX_GPIO;
        mask_protect |= 1ULL << UART_RX_GPIO;
    }
    if(CONFIG_ENABLE_CAN){
        ESP_LOGI(TAG, "Protecting CAN pins");
        mask_protect |= 1ULL << SHIFT_CLOCK_GPIO;
        mask_protect |= 1ULL << PISO_LOAD_GPIO;
        mask_protect |= 1ULL << PISO_DATA_GPIO;
        mask_protect |= 1ULL << SIPO_LATCH_GPIO;
        mask_protect |= 1ULL << SIPO_DATA_GPIO;
    }

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}