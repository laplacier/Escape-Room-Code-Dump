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

//-----------------Prototypes------------------//
static void puzzle_task(void *arg);
static bool CAN_Receive(uint32_t delay);
static void puzzle_main(void *arg);

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

//shift_reg
extern uint8_t dataIn[NUM_PISO]; // Data read from PISO registers

//nfc
extern ISO15693NFC_t nfc; // Struct holding data from nfc tags

//--------------Global Variables---------------//
SemaphoreHandle_t puzzle_task_sem;
QueueHandle_t puzzle_task_queue;
uint8_t game_state = 0;
uint8_t piso_old[NUM_PISO];

/*
 *
 */
static void puzzle_main(void *arg){
  // Setup
  static const char* TAG = "User";
  for(int i=0; i<NUM_PISO; i++){
    piso_old[i] = dataIn[i];
  }

  // Loop forever
  while(1){
    for(int i=0; i<8; i++){
      if(bitRead(dataIn[0],i) != bitRead(piso_old[0],i)){
        ESP_LOGI(TAG, "PISO#0 Pin %d: %d",i,bitRead(dataIn[0],i));
      }
    }
    piso_old[0] = dataIn[0];

    shift_write(1,1);
    shift_show();
    vTaskDelay(pdMS_TO_TICKS(1000));
    shift_write(1,0);
    shift_show();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/*
 * Helper functions - DO NOT EDIT
 * Functions required for flow control between other components
 */
void puzzle_init(void){
  puzzle_task_queue = xQueueCreate(1, sizeof(puzzle_task_action_t));
  puzzle_task_sem = xSemaphoreCreateCounting( 10, 0 );
  xTaskCreatePinnedToCore(puzzle_task, "Puzzle", 4096, NULL, PUZZLE_TASK_PRIO, NULL, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(puzzle_main, "User", 4096, NULL, PUZZLE_TASK_PRIO, NULL, tskNO_AFFINITY);
  ESP_LOGI("Puzzle", "Setup complete");
}

static void puzzle_task(void *arg){
  while(1){
    CAN_Receive(10);
  }
}

static bool CAN_Receive(uint32_t delay){
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