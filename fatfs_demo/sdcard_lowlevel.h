#ifndef SDCARD_LOWLEVEL_H
#define SDCARD_LOWLEVEL_H
#include <stdint.h>

typedef enum PIN_DIRECTION {
    PIN_INPUT = 0,
    PIN_OUTPUT = 1 } PIN_DIRECTION;

enum PINS {
    SD_DET = 0,
    SD_CLK = 1,
    SD_CMD = 2,
    SD_DA0 = 3,
    SD_DA1 = 4,
    SD_DA2 = 5,
    SD_DA3 = 6} PINS;

void initialize_pins();
void set_pin(int pin);
void clear_pin(int pin);
void set_pin_direction(int pin, int direction);
int32_t get_pin_signal(int pin);
void set_pin_pullup(int pin);

#endif // SDCARD_LOWLEVEL_H
