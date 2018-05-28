//#include <string.h>

#include "mmio.h"
#include "uart.h"
#include "gpio.h"
#include "mylib.h"

void _enable_jtag();


/* Loop <delay> times in a way that the compiler won't optimize away. */
// http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C
static inline void delay_mmio(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : "=r"(count): [count]"0"(count) : "cc");
}

// http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C
void uart_init()
{
	uint32_t reg;
	// Disable AUX (AUX provides mini UART fnction with same ports. Disable before enabling UART)
	mmio_write(AUX_ENABLES, mmio_read(AUX_ENABLES) & (!0x00000001));

	// Disable UART0.
	mmio_write(UART0_CR, 0x00000000);
	
	// Setup the GPIO pin 14 && 15 to alto 0
    gpioSetFunction(14, GPIO_ALT0);
    gpioSetFunction(15, GPIO_ALT0);
 
    // pull down GPIO pin 14 & 15
    gpioSetPull(14, GPIO_PULLUPDOWN_DOWN);
    gpioSetPull(15, GPIO_PULLUPDOWN_DOWN);
    
	// Clear pending interrupts.
	mmio_write(UART0_ICR, 0x7FF);
 
	// Set integer & fractional part of baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200.
 
	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	// Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
	mmio_write(UART0_IBRD, 1);
	mmio_write(UART0_FBRD, 40);
 
	// Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
	mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));
	//	mmio_write(UART0_LCRH, (1 << 5) | (1 << 6));  // fifo disabled
 
	// Mask all interrupts.
	mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	                       (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
 
	// Enable UART0, receive & transfer part of UART.
	mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));

	// enable JTAG
	_enable_jtag();
	
}
 
void uart_putc(uint8_t byte)
{
	// Wait for UART to become ready to transmit.
	while ( mmio_read(UART0_FR) & (1 << 5) ) { }
	mmio_write(UART0_DR, byte);
}
 
uint8_t uart_getc()
{
    // Wait for UART to have recieved something.
    while ( mmio_read(UART0_FR) & (1 << 4) ) { }
    return mmio_read(UART0_DR);
}
 
void uart_write(const char* buffer, size_t size)
{
	size_t i;
	for (i = 0; i < size; i++ ) {
		//		if (buffer[i]=='n') {
		//			uart_putc('\r');
		//		}
		uart_putc(buffer[i]);
	}
}
 
void uart_puts(const char* str)
{
	uart_write((const char*) str, strlen(str));
}

