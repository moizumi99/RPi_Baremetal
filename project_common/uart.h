#ifndef UART_H
#define UART_H
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

void uart_init();
void uart_putc(uint8_t byte);
uint8_t uart_getc();
void uart_write(const char* buffer, size_t size);
void uart_puts(const char* str);

#endif
