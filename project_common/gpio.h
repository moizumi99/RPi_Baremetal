#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

void gpioSetFunction(int32_t pin, uint32_t val);
void gpioSetPull(int32_t pin, uint32_t val);
void gpioSet(int32_t pin);
void gpioClear(int32_t pin);
int32_t gpioRead(int32_t pin);

enum {
    GPIO_INPUT = 0,
    GPIO_OUTPUT = 1,
    GPIO_ALT0 = 4,
    GPIO_ALT1 = 5,
    GPIO_ALT2 = 6,
    GPIO_ALT3 = 7,
    GPIO_ALT4 = 3,
    GPIO_ALT5 = 2
};

enum {
    GPIO_PULLUPDOWN_OFF = 0,
    GPIO_PULLUPDOWN_DOWN = 1,
    GPIO_PULLUPDOWN_UP = 2
};


#endif  // GPIO_H
