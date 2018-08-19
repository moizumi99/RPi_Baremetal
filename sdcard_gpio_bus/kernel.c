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
    uint8_t buffer0[4096], buffer1[4096], buffer[512 * 4 * 4];
    
	uart_init();
    printf("\n--------- Demo Start -----------\n");
	uart_puts("UART set up done\r\n");

    printf("This is a demo program of SD CARD access in SD mode\n");

    sdInitInterface();
    if (sdInitCard() == 0) {
        printf("SD Card Initialization success\n");
    } else {
        printf("SD Card Initialization fail\n");
        goto demo_end;
    }

    printf("Switch to high speed mode\n");
    if (sdHighSpeedMode() < 0) {
      printf("Switching to HighSpeed mode failed\n");
      goto demo_end;
    }
    printf("Switching to high speed mode succeeded\n");

    printf("Check SCR\n");
    sdCheckSCR(buffer0);
    printf("SCR check done\n");

    printf("Read from block 0\n");
    if (sdTransferBlocks(0, 1, buffer0, 0) < 0) {
        goto demo_end;
    }
    if (sdTransferBlocks(1 * 512, 1, buffer0 + 512, 0) < 0) {
        goto demo_end;
    }
    if (sdTransferBlocks(2 * 512, 1, buffer0 + 1024, 0) < 0) {
        goto demo_end;
    }
    if (sdTransferBlocks(3 * 512, 1, buffer0 + 1536, 0) < 0) {
        goto demo_end;
    }

    printf("Read from block 0 x 2\n");
    if (sdTransferBlocks(0, 4, buffer1, 0) < 0) {
        goto demo_end;
    }

    printf("single read\n");
    dump(buffer0, 512);
    printf("multiple read\n");
    dump(buffer1, 512);

    for(int32_t cnt = 0; cnt < 1024; cnt++) {
        if (buffer0[cnt] != buffer1[cnt]) {
            printf("Difference found at %4x\n", cnt);
            printf("%2x VS %2x\n", buffer0[cnt], buffer1[cnt]);
            goto demo_end;
        }
    }
    printf("No difference between single read and multiple read\n");
    
    printf("SD CARD, read write demo\n");

    // Single block read write demo
    printf("Read from block 4\n");
    sdRead(4, buffer);
    dump(buffer, 512);

    printf("Write increasing numbers (0, 1, 2, 3, ...) to block 4\n");
    for (uint32_t i=0; i<512; i++) {
        buffer[512 + i] = i & 0xff;
    }
    if (sdWrite(4, buffer + 512) != 512) {
        printf("Write Error\n");
        goto demo_end;
    }

    printf("Read from block 4\n");
    if (sdRead(4, buffer + 1024) != 512) {
        printf("Read Error\n");
        goto demo_end;
    }
    dump(buffer + 1024, 512);

    int errors = 0;
    for (int32_t i = 0; i < 512; i++) {
        if (buffer[512 + i] != buffer[1024 + i]) {
            printf("Error: mismatch at %d-th byte (%u vs %u)\n", i, buffer[512 + i], buffer[1024 + i]);
            errors++;
        }
    }
    if (errors == 0) {
        printf("No mismatch\n");
    }
    
    printf("Write the original to block 4\n");
    if (sdWrite(4, buffer) != 512) {
        printf("Write error\n");
        goto demo_end;
    }

    printf("Read from block 4\n");
    if (sdRead(4, buffer + 1536) != 512) {
        printf("Read error\n");
        goto demo_end;
    }
    dump(buffer + 1536, 512);

    errors = 0;
    for (int32_t i = 0; i < 512; i++) {
        if (buffer[i] != buffer[1536 + i]) {
            printf("Error: mismatch at %d-th byte (%u vs %u)\n", i, buffer[i], buffer[1536 + i]);
            errors++;
        }
    }
    if (errors == 0) {
        printf("No mismatch\n");
    }

    // Multiple blocks read write demo
    printf("Read from block 4 - 7\n");
    sdReadMulti(4, 4, buffer);
    dump(buffer, 512 * 4);

    printf("Write increasing numbers (0, 1, 2, 3, ...) to block 4 - 7\n");
    for (uint32_t i = 0; i < 512 * 4; i++) {
        buffer[512 * 4 + i] = i & 0xff;
    }
    if (sdWriteMulti(4, 4, buffer + 512 * 4) != 512 * 4) {
        printf("Write Error\n");
        goto demo_end;
    }

    printf("Read from block 4 - 7\n");
    if (sdReadMulti(4, 4, buffer + 512 * 4 * 2) != 512 * 4) {
        printf("Read Error\n");
        goto demo_end;
    }
    dump(buffer + 512 * 4 * 2, 512 * 4);

    errors = 0;
    for (int32_t i = 0; i < 512 * 4; i++) {
        if (buffer[512 * 4 + i] != buffer[512 * 4 * 2 + i]) {
            printf("Error: mismatch at %d-th byte (%u vs %u)\n", i,
                   buffer[512 * 4 + i], buffer[512 * 4 * 2 + i]);
            errors++;
        }
    }
    if (errors == 0) {
        printf("No mismatch\n");
    }
    
    printf("Write the original to block 4 - 7\n");
    if (sdWriteMulti(4, 4, buffer) != 512 * 4) {
        printf("Write error\n");
        goto demo_end;
    }

    printf("Read from block 4 - 7\n");
    if (sdReadMulti(4, 4, buffer + 512 * 4 * 3) != 512 * 4) {
        printf("Read error\n");
        goto demo_end;
    }
    dump(buffer + 512 * 4 * 3, 512 * 4);

    errors = 0;
    for (int32_t i = 0; i < 512 * 4; i++) {
        if (buffer[i] != buffer[512 * 4 * 3 + i]) {
            printf("Error: mismatch at %d-th byte (%u vs %u)\n", i,
                   buffer[i], buffer[512 * 4 * 3 + i]);
            errors++;
        }
    }
    if (errors == 0) {
        printf("No mismatch\n");
    }

    // file access demo
    printf("\nFile system access demo\n");
    printf("Read the FAT file system. ");
    init_filesystem();

    printf("Showing the root folder contents\n");
    cmd_dir();

 demo_end:
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

