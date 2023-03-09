#ifndef prop_gpio
#define prop_gpio
typedef enum {
    HIGH,
    LOW,
    READ,
    READ_ALL,
    DISABLE,
    SET_OUTPUT,
    SET_INPUT,
    SET_INPUT_PULLUP,
    SET_INPUT_PULLDOWN,
    SET_ADC,
    SET_DAC
} gpio_task_action_t;

#endif