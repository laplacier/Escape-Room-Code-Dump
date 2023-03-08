#ifndef shift_reg
#define shift_reg

#define GENERIC_TASK_PRIO 1  // Any unspecified task
#define NUM_SIPO CONFIG_NUM_SIPO
#define NUM_PISO CONFIG_NUM_PISO
#define SHIFT_CLOCK_GPIO 7     // Pin 2 on all SN74HC165N and Pin 11 on all SN74HC595N
#define PISO_LOAD_GPIO   2     // Pin 1 on all SN74HC165N
#define PISO_DATA_GPIO   3     // Pin 9 on FIRST SN74HC165N
#define SIPO_LATCH_GPIO  4     // Pin 12 on all SN74HC595N
#define SIPO_DATA_GPIO   6     // Pin 14 on FIRST SN74HC595N

void shift_init(void);
void sipo_task(void *arg);
void piso_task(void *arg);

#endif