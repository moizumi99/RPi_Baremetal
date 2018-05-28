#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include "mylib.h"
#include "uart.h"
#include "mmio.h"
#include "gpio.h"
#include "sdcard.h"
#include "file.h"

void _enable_jtag();
void cmd_dir();

uint8_t rdedata[512*32];

#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    uint32_t mask, data;
    uint8_t res;
    uint8_t buffer[2048];
    
	uart_init();
	uart_puts("UART set up done\r\n");

    printf("This is a demo program of SD CARD access in SPI mode\n");

    sdInitInterface();

    printf("SD Initialization start\n");
    
    res = 0xff;
    uint8_t key;
    while (1) {
        if (sdInitCard() == 0) {
            break;
        }
        printf("Error: SD Initialization failed.\n");
        printf("Init response: %u\n", res);
        printf("Please remove and re-insert the SD card\n");
        printf("Press ENTER key when done: ");
        while(uart_getc() != 13) ;
        uart_putc(13);
    }
    printf("SD Initialization done\n");
    waitMicro(1000000);

    printf("SD CARD, read write demo\n");
    printf("Read from block 4\n");
    sdRead(4, buffer);
    dump(buffer, 512);

    for (uint32_t i=0; i<512; i++) {
        buffer[512 + i] = i & 0xff;
    }
    printf("Write to block 4\n");
    sdWrite(4, buffer + 512);

    printf("Read from block 4\n");
    sdRead(4, buffer + 1024);
    dump(buffer + 1024, 512);

    printf("Write the original to block 4\n");
    sdWrite(4, buffer);

    printf("Read from block 4\n");
    sdRead(4, buffer + 1536);
    dump(buffer + 1536, 512);

    for (int32_t i = 0; i < 512; i++) {
        if (buffer[i] != buffer[1536 + i]) {
            printf("Error: mismatch at %d-th byte (%u vs %u)\n", i, buffer[i], buffer[1536 + i]);
        }
    }

    printf("\nFile system access demo\n");
    printf("Read the FAT file system. ");
    init_filesystem();

    printf("Showing the root folder contents\n");
    printf("(This may take a few minutes)\n");
    cmd_dir();

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


void cmd_dir()
{
	volatile struct FILEINFO *finfo = (volatile struct FILEINFO *) (rdedata); // RPi only
	int32_t i, j;
	char s[30];
	
	for (i = 0; i <512; i++) {
		if (finfo[i].name[0] == 0x00) {
            debug_log("End of root entry\n");
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
                s[29] = 0;
				printf("%s", s);
			}
		}
	}
	return;
}
