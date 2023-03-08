#include "driver/uart.h"
#include "sound.h"

#define UART_RX 17
#define UART_TX 16
#define UART UART_NUM_2

static TaskHandle_t sound_task_handle;
static void sound_init(void) 
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // We will only send data to the DFPlayer Mini in NO ACK mode, no RX required
    uart_driver_install(UART, 0, 0, 0, NULL, 0);
    uart_param_config(UART, &uart_config);
    uart_set_pin(UART, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Send commands to DFPlayer Mini to initialize sound settings
    sendAudioCommand(0x0D,0);              // Send command 0x0D (Enable DFPlayer Mini) and no parameter required
    sendAudioCommand(0x07,0);              // Send command 0x07 (Set EQ) and parameter 0x00 (Normal EQ)
    sendAudioCommand(0x06, CONFIG_VOLUME); // Send command 0x06 (Set Volume) and parameter CONFIG_VOLUME
    
    // Create sound task
    xTaskCreatePinnedToCore(sound_task, "sound", 4096, NULL, GENERIC_TASK_PRIO, &sound_task_handle, tskNO_AFFINITY);
    //xTaskNotify(sound_task_handle,0x50,eSetValueWithOverwrite);
}

static void sendAudioCommand(uint8_t command, uint16_t parameter){
    //------------------ CREATE INSTRUCTION -------------------------//
    uint8_t startByte     = 0x7E; // Start
    uint8_t versionByte   = 0xFF; // Version
    uint8_t commandLength = 0x06; // Length
    uint8_t feedback      = 0x00; // Feedback
    uint8_t endByte       = 0xEF; // End

    uint16_t checksum = -(         // Create the two-byte checksum
        versionByte +
        commandLength +
        command +
        feedback +
        (parameter >> 8) +
        (parameter & 0xFF)
    );

    uint8_t instruction[10] = {   // Create a byte array of the instruction to send 
        startByte,
        versionByte,
        commandLength,
        command,
        feedback,
        (parameter >> 8),   // High byte
        (parameter & 0xFF), // Low byte
        (checksum >> 8),    // High byte
        (checksum & 0xFF),  // Low byte
        endByte
    };

    //------------------- SEND INSTRUCTION --------------------------//
    for (int i=0; i<10; i++){                      // For each byte in the instruction...
        uart_write_bytes(UART, instruction[i], 1); // Send the selected byte to the DFPlayer Mini via serial
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);           // Wait for the DFPlayer Mini to process the instruction
}

static void sound_task(void *arg){
    static const char* TAG = "Sound";
    uint8_t track;
    while(1){
        xTaskNotifyWait(0X00, ULONG_MAX, &track, portMAX_DELAY); // Blocked from executing until puzzle_task gives track to play
        sendAudioCommand(0x03,track);                            // Play track.mp3 in the MP3 folder of the DFPlayer Mini
        ESP_LOGI(TAG, "Playing %d.mp3",track);
    }
}