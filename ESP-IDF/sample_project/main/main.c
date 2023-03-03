#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_rom_gpio.h"
#include "esp_rom_sys.h"
#include "soc/gpio_sig_map.h" // For TWAI_TX_IDX

/* --------------------- Definitions and static variables ------------------ */
// GPIO Pin assignments
#define TX_GPIO_NUM                     19 // ESP32 Tx Pin to CAN Pin
#define RX_GPIO_NUM                     22 // ESP32 Rx Pin to CAN Pin

// FreeRTOS task priorities
#define CTRL_TASK_PRIO                  10 // Control_Task
#define RX_TASK_PRIO                    9  // Rx_Task
#define TX_TASK_PRIO                    8  // Tx_Task
#define PUZZLE_TASK_PRIO                7  // Puzzle_Task
#define GENERIC_TASK_PRIO               1  // Any unspecified task

#define ERR_DELAY_US                    800     //Approximate time for arbitration phase at 25KBPS
#define ERR_PERIOD_US                   80      //Approximate time for two bits at 25KBPS

#define ID_PROP                         0x01 // 8-bit Prop ID, combined with 3-bit priority for full CAN ID
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_25KBITS();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NO_ACK);
static const twai_message_t tx_msg = {.identifier = 0, .data_length_code = 0};
static const twai_message_t ping_resp = {.identifier = 0b01100000000 + ID_PROP, .data_length_code = 0,
                                        .data = {0,0,0,0,0,0,0,0}};
static SemaphoreHandle_t ctrl_task_sem;
static SemaphoreHandle_t rx_task_sem;
static SemaphoreHandle_t tx_task_sem;
//static SemaphoreHandle_t puzzle_task_sem;
//static SemaphoreHandle_t music_task_sem;
//static SemaphoreHandle_t gpio_task_sem;

typedef enum {
    BEGIN,
    TX_HELLO,
    TX_PING,
    RX_PING,
    TX_DATA,
    SOLVE,
    RESET,
    PLAY_SOUND,
    INHERIT_REQ,
    INHERIT_PASS,
    INHERIT_STOP
} ctrl_task_action_t;

typedef enum {
    RX_PING,
    SOLVE,
    RESET,
    PLAY_SOUND,
    INHERIT_STOP
} rx_task_action_t;

typedef enum {
    TX_HELLO,
    TX_PING,
    TX_DATA,
    TX_DND,
    TX_INHERIT
} tx_task_action_t;


static void ctrl_task(void *arg){
    static const char* TAG = "CAN_Controller";
    for(;;){
        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY); // Blocked from executing until app_main, puzzle_task or rx_task gives a semaphore

    }
}

static void rx_task(void *arg){
    static const char* TAG = "CAN_Rx";
    xSemaphoreTake(rx_task_sem, portMAX_DELAY); // Blocked from beginning until ctrl_task gives semaphore
    ESP_LOGD(TAG, "Task initialized");
    for(;;){ // Runs forever after taking semaphore
        vTaskDelay(pdMS_TO_TICKS(1000)); // Simulating no messages
    }
}

static void tx_task(void *arg){
    tx_task_action_t action;
    static const char* TAG = "CAN_Tx";
    ESP_LOGD(TAG, "Task initialized");
    for(;;){
        xSemaphoreTake(tx_task_sem, portMAX_DELAY); // Blocked from executing until ctrl_task gives semaphore
        xQueueReceive(tx_task_queue, &action, portMAX_DELAY);
        if (action == TX_SEND_PING_RESP) {
            //Transmit ping response to master
            twai_transmit(&ping_resp, portMAX_DELAY);
            ESP_LOGI(EXAMPLE_TAG, "Transmitted ping response");
        }
    }
}

void app_main(void){
    tx_task_queue = xQueueCreate(1, sizeof(tx_task_action_t));
    rx_task_queue = xQueueCreate(1, sizeof(rx_task_action_t));
    ctrl_task_queue = xQueueCreate(1, sizeof(ctrl_task_action_t));
    ctrl_task_sem = xSemaphoreCreateCounting( 10, 0 );
    rx_task_sem = xSemaphoreCreateBinary();
    tx_task_sem = xSemaphoreCreateCounting( 10, 0 );
    //puzzle_task_sem = xSemaphoreCreateCounting( 10, 0 );

    xTaskCreatePinnedToCore(tx_task, "CAN_Tx", 4096, NULL, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(ctrl_task, "CAN_Controller", 4096, NULL, CTRL_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(rx_task, "CAN_Rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    //xTaskCreatePinnedToCore(puzzle_task, "Puzzle", 4096, NULL, PUZZLE_TASK_PRIO, NULL, tskNO_AFFINITY);
    //xTaskCreatePinnedToCore(music_task, "Music", 4096, NULL, GENERIC_TASK_PRIO, NULL, tskNO_AFFINITY);
    //xTaskCreatePinnedToCore(gpio_task, "GPIO", 4096, NULL, GENERIC_TASK_PRIO, NULL, tskNO_AFFINITY);
}
