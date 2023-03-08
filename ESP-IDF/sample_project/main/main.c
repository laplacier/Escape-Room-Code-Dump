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
#define TX_GPIO_NUM                     22 // ESP32 Tx Pin to CAN Pin
#define RX_GPIO_NUM                     19 // ESP32 Rx Pin to CAN Pin

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
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NO_ACK);
static const twai_message_t ping_resp = {.identifier = 0b11000000000 + ID_PROP, .data_length_code = 0,
                                        .data = {0,0,0,0,0,0,0,0}, .self = 1};
//static const twai_message_t ping_req = {.identifier = 0b11100000000 + 0x00, .data_length_code = 1,
                                        //.data = {1,0,0,0,0,0,0,0}, .self = 1};
static QueueHandle_t ctrl_task_queue;
static QueueHandle_t tx_task_queue;
static QueueHandle_t puzzle_task_queue;
static SemaphoreHandle_t ctrl_task_sem;
static SemaphoreHandle_t rx_task_sem;
static SemaphoreHandle_t rx_payload_sem;
static SemaphoreHandle_t tx_task_sem;
static SemaphoreHandle_t puzzle_task_sem;
static SemaphoreHandle_t puzzle_payload_sem;
//static SemaphoreHandle_t music_task_sem;
//static SemaphoreHandle_t gpio_task_sem;

uint8_t rx_payload[7];

typedef enum {
    BEGIN,
    RX_PING,
    RX_CMD,
    INHERIT_REQ,
    INHERIT_STOP
} ctrl_task_action_t;

typedef enum {
    TX_HELLO,
    TX_PING,
    TX_DATA,
    TX_DND,
    TX_INHERIT,
    INHERIT_PASS
} tx_task_action_t;

typedef enum {
    CMD
} puzzle_task_action_t;


static void ctrl_task(void *arg){
    ctrl_task_action_t ctrl_action;
    puzzle_task_action_t puzzle_action;
    tx_task_action_t tx_action;
    static const char* TAG = "CAN_Controller";
    xSemaphoreGive(rx_task_sem); // Allow rx_task to begin receiving CAN messages
    for(;;){
        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY); // Blocked from executing until app_main, puzzle_task or rx_task gives a semaphore
        xQueueReceive(ctrl_task_queue, &ctrl_action, pdMS_TO_TICKS(10)); // Pull task from queue
        if(ctrl_action == BEGIN){
            tx_action = TX_HELLO;
            xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
            xSemaphoreGive(tx_task_sem);
            xSemaphoreGive(rx_payload_sem); // Give control of rx_payload to rx_task
            ESP_LOGI(TAG, "Init successful");
        }
        else if(ctrl_action == RX_PING){
            tx_action = TX_PING;
            xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
            xSemaphoreGive(tx_task_sem);
            ESP_LOGI(TAG, "Sent ping task to TX");
        }
        else if(ctrl_action == RX_CMD){
            puzzle_action = CMD;
            xQueueSend(puzzle_task_queue, &puzzle_action, portMAX_DELAY);
            xSemaphoreGive(puzzle_task_sem);
            ESP_LOGI(TAG, "Sent CMD to Puzzle");
        }
    }
}

static void rx_task(void *arg){
    static const char* TAG = "CAN_Rx";
    static const char* type[8]= {"ALL_COMMAND","COMMAND","unused","unused","INHERIT","ALL_PING_REQ","PING_RESP","PING_REQ"};
    uint8_t msg_id;
    uint8_t msg_type;
    twai_message_t rx_msg;
    ctrl_task_action_t ctrl_action;
    xSemaphoreTake(rx_task_sem, portMAX_DELAY); // Blocked from beginning until ctrl_task gives semaphore
    ESP_LOGI(TAG, "Task initialized");
    while(1){ // Runs forever after taking semaphore
        //Wait for message
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(11000)) == ESP_OK) { // Wait for messages on CAN bus
            ESP_LOGI(TAG, "Received message...");
            msg_id = (uint8_t)(rx_msg.identifier & 0xFF);
            msg_type = (uint8_t)(rx_msg.identifier >> 8);
            ESP_LOGI(TAG, "From_ID: %d, Type: %s, To_ID: %d",msg_id,type[msg_type],rx_msg.data[0]);
            switch(msg_type){
                case 0: case 1: // Command
                    ctrl_action = RX_CMD;
                    xSemaphoreTake(rx_payload_sem, portMAX_DELAY); // Can only take if payload not being read by puzzle_task
                    for(int i=1; i<rx_msg.data_length_code; i++){
                        rx_payload[i-1] = rx_msg.data[i];
                    }
                    xQueueSend(ctrl_task_queue, &ctrl_action, portMAX_DELAY);
                    xSemaphoreGive(ctrl_task_sem);
                    break;
                case 5: case 7: // Ping request
                    ctrl_action = RX_PING;
                    xQueueSend(ctrl_task_queue, &ctrl_action, portMAX_DELAY);
                    xSemaphoreGive(ctrl_task_sem);
                    break;
                case 6: // Ping response
                    break;
                default: // Unknown command
            }
        } else {
            ESP_LOGI(TAG, "No messages on the bus?");
        }
    }
}

static void tx_task(void *arg){
    tx_task_action_t action;
    static const char* TAG = "CAN_Tx";
    ESP_LOGI(TAG, "Task initialized");
    for(;;){
        xSemaphoreTake(tx_task_sem, portMAX_DELAY); // Blocked from executing until ctrl_task gives semaphore
        xQueueReceive(tx_task_queue, &action, pdMS_TO_TICKS(10)); // Pull task from queue
        if (action == TX_PING) {
            //Queue message for transmission
            if (twai_transmit(&ping_resp, portMAX_DELAY) == ESP_OK) {
                ESP_LOGI(TAG, "Transmitted ping response");
            } else {
                ESP_LOGI(TAG, "Failed to transmit message");
            }
        }
        else{
            ESP_LOGI(TAG, "Unknown action received: %d",action);
        }
    }
}

static void fake_bus_task(void *arg){
    twai_message_t gpio_mask = {.identifier = 0b00100000000, .data_length_code = 8,
                                        .data = {0x01,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, .self = 1};
    twai_message_t gpio_states = {.identifier = 0b00100000000, .data_length_code = 8,
                                        .data = {0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00}, .self = 1};
    static const char* TAG = "CAN_Fake_Bus";
    ESP_LOGI(TAG, "Task initialized");
    for(;;){
        //Fake ping request
        //twai_transmit(&ping_req, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(5000)); // Simulating no messages for 5 seconds
        //Fake GPIO change
        twai_transmit(&gpio_mask, portMAX_DELAY);
        twai_transmit(&gpio_states, portMAX_DELAY);
        for(int i=2; i<8; i++){
            gpio_states.data[i]++; // Alter the pins being changed for next loop
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Simulating no messages for 5 seconds
    }
}

static void puzzle_task(void *arg){
    puzzle_task_action_t action;
    static const char* TAG = "Puzzle";
    static const char* cmd[4]= {"STATE","GPIO_MASK","GPIO","MUSIC"};
    uint8_t game_state = 0;
    uint8_t gpio_mask[6] = {0,0,0,0,0,0};
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
    ESP_LOGI(TAG, "Task initialized");
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

void app_main(void){
    static const char* TAG = "Init_Main";
    tx_task_queue = xQueueCreate(1, sizeof(tx_task_action_t));
    ctrl_task_queue = xQueueCreate(1, sizeof(ctrl_task_action_t));
    puzzle_task_queue = xQueueCreate(1, sizeof(puzzle_task_action_t));
    ctrl_task_sem = xSemaphoreCreateCounting( 10, 0 );
    rx_task_sem = xSemaphoreCreateBinary();
    tx_task_sem = xSemaphoreCreateCounting( 10, 0 );
    puzzle_task_sem = xSemaphoreCreateCounting( 10, 0 );
    rx_payload_sem = xSemaphoreCreateBinary(); // Allows rx_task to write to the data payload
    puzzle_payload_sem = xSemaphoreCreateBinary(); // Allows puzzle_task to read the data payload

    xTaskCreatePinnedToCore(tx_task, "CAN_Tx", 4096, NULL, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(ctrl_task, "CAN_Controller", 4096, NULL, CTRL_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(rx_task, "CAN_Rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(fake_bus_task, "CAN_Fake_Bus_Task", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(puzzle_task, "Puzzle", 4096, NULL, PUZZLE_TASK_PRIO, NULL, tskNO_AFFINITY);
    //xTaskCreatePinnedToCore(music_task, "Music", 4096, NULL, GENERIC_TASK_PRIO, NULL, tskNO_AFFINITY);
    //xTaskCreatePinnedToCore(gpio_task, "GPIO", 4096, NULL, GENERIC_TASK_PRIO, NULL, tskNO_AFFINITY);

    sound_init();
    //Install TWAI driver, trigger tasks to start
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(TAG, "Driver installed");
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "Driver started");

    ctrl_task_action_t ctrl_action = BEGIN;
    xQueueSend(ctrl_task_queue, &ctrl_action, portMAX_DELAY); // Send BEGIN job to control task
    xSemaphoreGive(ctrl_task_sem);                            // Unblock control task
}
