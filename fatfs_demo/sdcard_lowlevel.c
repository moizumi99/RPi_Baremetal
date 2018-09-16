#include <stdint.h>
#include "sdcard_lowlevel.h"
#include "mmio.h"
#include "gpio.h"
#include "mylib.h"
#include "gpio.h"

enum PIN_NUMBER {
    SD_DET_PIN = 47,
    SD_CLK_PIN = 48,
    SD_CMD_PIN = 49,
    SD_DA0_PIN = 50,
    SD_DA1_PIN = 51,
    SD_DA2_PIN = 52,
    SD_DA3_PIN = 53
} PIN_NUMBER;

void initialize_pins() {
    // disable EMMC by disabling CLK_EN and CLK_INTLEN
    mmio_set(EMMC_CONTROL0, 0x05, 0x00);
}

int get_pin_number(int pin) {
    int pin_number;
    switch(pin) {
    case SD_DET:
        pin_number = SD_DET_PIN;
        break;
    case SD_CLK:
        pin_number = SD_CLK_PIN;
        break;
    case SD_CMD:
        pin_number = SD_CMD_PIN;
        break;
    case SD_DA0:
        pin_number = SD_DA0_PIN;
        break;
    case SD_DA1:
        pin_number = SD_DA1_PIN;
        break;
    case SD_DA2:
        pin_number = SD_DA2_PIN;
        break;
    case SD_DA3:
        pin_number = SD_DA3_PIN;
        break;
    defulat:
        printf("Wrong pin specified (%d)\n", pin);
        return -1;
    }
    return pin_number;
}


void set_pin(int pin) {
    int pin_number = get_pin_number(pin);
    gpioSet(pin_number);
}

void clear_pin(int pin) {
    int pin_number = get_pin_number(pin);
    gpioClear(pin_number);
}

void set_pin_direction(int pin, int direction) {
    int pin_number = get_pin_number(pin);
    int function = (direction == PIN_INPUT) ? GPIO_INPUT : GPIO_OUTPUT;
    gpioSetFunction(pin_number, function);
}

int32_t get_pin_signal(int pin) {
    int pin_number = get_pin_number(pin);
    return gpioRead(pin_number);
}

void set_pin_pullup(int pin) {
    int pin_number = get_pin_number(pin);
    gpioSetPull(pin_number, GPIO_PULLUPDOWN_UP);
}
