#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include "mylib.h"
#include "uart.h"
#include "gpio.h"
#include "mmio.h"
#include "ff.h"

void _enable_jtag();

FATFS FatFs;		/* FatFs work area needed for each volume */
FIL Fil;			/* File object needed for each open file */

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
	uart_init();
    printf("\n--------- Demo Start -----------\n");
	uart_puts("UART set up done\r\n");

    printf("This is a demo program of SD CARD access in SD mode\n");

	UINT bw;

	f_mount(&FatFs, "", 0);		/* Give a work area to the default drive */

	if (f_open(&Fil, "newfile.txt", FA_READ) == FR_OK) {	/* Open a file */

        uint8_t buffer[4096];
    
		f_read(&Fil, buffer, 4096, &bw);	/* Write data to the file */

		f_close(&Fil);								/* Close the file */

        printf("Read of newfile.txt success\n");
        printf("Read %d bytes\n", bw);

        for(uint32_t i = 0; i < bw; i++) {
            uart_putc(buffer[i]);
        }
	} else {
        printf("Read of newfile.txt failed\n");
    }

    printf("\nDemo end\n");
    
    // LED blink to show the end
    gpioSetFunction(16, 1);
	while ( true ) {
      gpioClear(16);
      waitMicro(1000000);
      gpioSet(16);
      waitMicro(1000000);
    }
      
}

