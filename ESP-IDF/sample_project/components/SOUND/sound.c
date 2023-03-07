#include "driver/uart.h"

#define UART_RX 17
#define UART_TX 16
#define UART UART_NUM_2

static const int TX_BUF_SIZE = 1024;

void sound_init(void) 
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // We will only send data to the DFPlayer Mini in NO ACK mode, no RX
    uart_driver_install(UART, 0, 0, 0, NULL, 0);
    uart_param_config(UART, &uart_config);
    uart_set_pin(UART, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

static void tx_task(void *arg)
{
	char* Txdata = (char*) malloc(100);
    while (1) {
    	sprintf (Txdata, "Hello world index = %d\r\n", num++);
        uart_write_bytes(UART, Txdata, strlen(Txdata));
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
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
        highByte(parameter) +
        lowByte(parameter)
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
    for (int i=0; i<10; i++){       // For each byte in the instruction...
    uart_write_bytes(UART, instruction[i], 1);      // Send the selected byte to the DFMini Player via serial
    }
    delay(100);                            // Wait for the DFMini Player to process the instruction
}