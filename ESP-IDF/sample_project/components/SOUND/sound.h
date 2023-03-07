#include "driver/uart.h"

#define UART_RX (GPIO_NUM_17)
#define UART_TX (GPIO_NUM_16)
#define UART UART_NUM_2

static const int TX_BUF_SIZE = 1024;