#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "soc/gpio_sig_map.h" // For TWAI_TX_IDX
#include "twai_can.h"
#include "puzzle.h"

static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NO_ACK);
static const twai_message_t ping_resp = {.identifier = 0b11000000000 + ID_PROP, .data_length_code = 0,
                                        .data = {0,0,0,0,0,0,0,0}, .self = 1};

QueueHandle_t ctrl_task_queue;
QueueHandle_t tx_task_queue;
SemaphoreHandle_t ctrl_task_sem;
SemaphoreHandle_t rx_task_sem;
SemaphoreHandle_t rx_payload_sem;
SemaphoreHandle_t tx_task_sem;

extern SemaphoreHandle_t puzzle_task_sem;
extern QueueHandle_t puzzle_task_queue;

uint8_t rx_payload[7];

void twai_can_init(void){
    tx_task_queue = xQueueCreate(1, sizeof(tx_task_action_t));
    ctrl_task_queue = xQueueCreate(1, sizeof(ctrl_task_action_t));
    ctrl_task_sem = xSemaphoreCreateCounting( 10, 0 );
    rx_task_sem = xSemaphoreCreateBinary();
    tx_task_sem = xSemaphoreCreateCounting( 10, 0 );
    rx_payload_sem = xSemaphoreCreateBinary(); // Allows rx_task to write to the data payload

    xTaskCreatePinnedToCore(tx_task, "CAN_Tx", 4096, NULL, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(ctrl_task, "CAN_Controller", 4096, NULL, CTRL_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(rx_task, "CAN_Rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(fake_bus_task, "CAN_Fake_Bus_Task", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

    //Install TWAI driver, trigger tasks to start
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI("TWAI_CAN", "Driver installed");
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI("TWAI_CAN", "Driver started");

    ctrl_task_action_t ctrl_action = BEGIN;
    xQueueSend(ctrl_task_queue, &ctrl_action, portMAX_DELAY); // Send BEGIN job to control task
    xSemaphoreGive(ctrl_task_sem);                            // Unblock control task
    ESP_LOGI("TWAI_CAN", "Setup complete");
}

void ctrl_task(void *arg){
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

void rx_task(void *arg){
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
            //ESP_LOGI(TAG, "No messages on the bus?");
        }
    }
}

void tx_task(void *arg){
    tx_task_action_t action;
    static const char* TAG = "CAN_Tx";
    ESP_LOGI(TAG, "Task initialized");
    for(;;){
        xSemaphoreTake(tx_task_sem, portMAX_DELAY); // Blocked from executing until ctrl_task gives semaphore
        xQueueReceive(tx_task_queue, &action, pdMS_TO_TICKS(10)); // Pull task from queue
        switch(action){
            case TX_HELLO:
                if (twai_transmit(&ping_resp, portMAX_DELAY) == ESP_OK) {
                    ESP_LOGI(TAG, "Transmitted HELLO");
                } else {
                    ESP_LOGI(TAG, "Failed to transmit HELLO");
                }
                break;
            case TX_PING:
                //Queue message for transmission
                if (twai_transmit(&ping_resp, portMAX_DELAY) == ESP_OK) {
                    ESP_LOGI(TAG, "Transmitted ping response");
                } else {
                    ESP_LOGI(TAG, "Failed to transmit ping response");
                }
                break;
            default:
                ESP_LOGI(TAG, "Unknown action received: %d",action);
        }
    }
}

void fake_bus_task(void *arg){
    twai_message_t gpio_mask = {.identifier = 0b00100000000, .data_length_code = 8,
                                        .data = {0x01,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, .self = 1};
    twai_message_t gpio_states = {.identifier = 0b00100000000, .data_length_code = 8,
                                        .data = {0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00}, .self = 1};
    twai_message_t play_sound = {.identifier = 0b00100000000, .data_length_code = 3,
                                        .data = {0x01,0x03,0x01}, .self = 1};
    static const char* TAG = "CAN_Fake_Bus";
    ESP_LOGI(TAG, "Task initialized");
    for(;;){
        // Fake ping request
        //twai_transmit(&ping_req, portMAX_DELAY);
        //vTaskDelay(pdMS_TO_TICKS(5000)); // Simulating no messages for 5 seconds
        //Fake GPIO change
        twai_transmit(&gpio_mask, portMAX_DELAY);
        twai_transmit(&gpio_states, portMAX_DELAY);
        for(int i=2; i<8; i++){
            gpio_states.data[i]++; // Alter the pins being changed for next loop
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Simulating no messages for 5 seconds
        // Fake sound command
        twai_transmit(&play_sound, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(5000)); // Simulating no messages for 5 seconds
    }
}