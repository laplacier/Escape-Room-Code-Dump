#ifndef twai_can
#define twai_can

// GPIO Pin assignments
#define CAN_TX_GPIO                     32 // ESP32 Tx Pin to CAN Pin
#define CAN_RX_GPIO                     33 // ESP32 Rx Pin to CAN Pin

// FreeRTOS task priorities
#define CTRL_TASK_PRIO                  10 // Control_Task
#define RX_TASK_PRIO                    9  // Rx_Task
#define TX_TASK_PRIO                    8  // Tx_Task

#define ERR_DELAY_US                    800     //Approximate time for arbitration phase at 25KBPS
#define ERR_PERIOD_US                   80      //Approximate time for two bits at 25KBPS

#define ID_PROP                         0x01 // 8-bit Prop ID, combined with 3-bit priority for full CAN ID

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

void ctrl_task(void *arg);
void rx_task(void *arg);
void tx_task(void *arg);
void fake_bus_task(void *arg);
void twai_can_init(void);

#endif