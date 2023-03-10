#ifndef prop_gpio
#define prop_gpio
typedef enum {
    SET_MASK,
    SET_STATES
} gpio_task_action_t;

#define bitRead(a,b) (!!((a) & (1ULL<<(b))))            // a = var, b = bit position
#define bitWrite(a,b,x) ((a) = (a & ~(1ULL<<b))|(x<<b)) // a = var, b = bit position, x = value
#define READBYTE(VAR, IDX)    (((VAR) >> ((IDX) * 8)) & 0xFF)
#define BYTESHIFTL(VAR, IDX)    (((VAR) <<= ((IDX) * 8)))
#endif