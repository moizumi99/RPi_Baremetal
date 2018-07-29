#include "mylib.h"
#include "mmio.h"

void gpioSetFunction(int32_t pin, uint32_t val)
{
	uint32_t adr, reg;
	int32_t base=0;
	//	printf("Input  val: %03x\n", val);
	if (0<=pin && pin<10) {
		adr = GPFSEL0;
		base = 0;
	} else if (10<=pin && pin<20) {
		adr = GPFSEL1;
		base = 10;
	} else if (20<=pin && pin<30) {
		adr = GPFSEL2;
		base = 20;
	} else if (30<=pin && pin<40) {
		adr = GPFSEL3;
		base = 30;
	} else if (40<=pin && pin<50) {
		adr = GPFSEL4;
		base = 40;
	} else if (50<=pin && pin<=53) {
		adr = GPFSEL5;
		base = 50;
	} else {
		printf("Error gpioSetFunction, pin:%d\n", pin);
		return;
	}
	reg = mmio_read(adr);
	reg = reg & (~(7 << ((pin-base)*3)));
	reg = reg | ((val & 7) << ((pin-base)*3));
	mmio_write(adr, reg);
	waitMicro(1);
}

// val = 0 : 
void gpioSetPull(int32_t pin, uint32_t val)
{
	uint32_t adr;
	uint32_t enbit;
	if (pin<32) {
		enbit = 1 << pin;
		adr = GPPUDCLK0;
	} else {
		enbit = 1 << (pin - 32);
		adr = GPPUDCLK1;
	}
	mmio_write(GPPUD, val);
	wait(150);
	mmio_write(adr, enbit);
	wait(150);
	mmio_write(GPPUD, 0);
	mmio_write(adr, 0);
}

void gpioSet(int32_t pin)
{
    uint32_t adr, enbit;
    if (pin < 32) {
        enbit = 1 << pin;
        adr = GPSET0;
    } else {
        enbit = 1 << (pin - 32);
        adr = GPSET1;
    }
    mmio_write(adr, enbit);
}

void gpioClear(int32_t pin)
{
    uint32_t adr, enbit;
    if (pin < 32) {
        enbit = 1 << pin;
        adr = GPCLR0;
    } else {
        enbit = 1 << (pin - 32);
        adr = GPCLR1;
    }
    mmio_write(adr, enbit);
}

int32_t gpioRead(int32_t pin)
{
    uint32_t adr, sft;
    if (pin < 32) {
        sft = pin;
        adr = GPLEV0;
    } else {
        sft = pin - 32;
        adr = GPLEV1;
    }
    uint32_t val = mmio_read(adr);
    return (val >> sft) & 1;
}
