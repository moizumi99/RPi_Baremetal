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

FRESULT scan_files( TCHAR* path) {

    FRESULT res;
    FILINFO fno;
    DIR dir;
    TCHAR *fn;
#if _USE_LFN
    static TCHAR lfnp[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = _MAX_LFN + 1;
#endif

    res = f_opendir(&dir, path);
    if (res == FR_OK) {
        int i = strlen(path);
        while ( true ) {
            res = f_readdir(&dir, &fno);
            if (res || !fno.fname[0]) break;
            if (fno.fname[0] == '.') continue;
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            if (fno.fattrib & AM_DIR) {
                sprintf(&path[i], "%s", fn);
                res = scan_files(path);
                if (res) break;
                path[i] = 0;
            } else {
                printf("%s/%s\n", path, fn);
            }
        }
    }
                
    return res;
}

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    FIL Fil;			/* File object needed for each open file */
    
	uart_init();
    printf("\n--------- Demo Start -----------\n");
	uart_puts("UART set up done\r\n");
    printf("This is a demo program of SD CARD access in SD mode\n");

	UINT bw;

	f_mount(&FatFs, "", 0);		/* Give a work area to the default drive */

    // Read demo
	if (f_open(&Fil, "newfile.txt", FA_READ) == FR_OK) {	/* Open a file */

        uint8_t buffer[4096];
    
		f_read(&Fil, buffer, 4096, &bw);	/* Read data to the file */

		f_close(&Fil);								/* Close the file */

        printf("Read of newfile.txt success\n");
        printf("Read %d bytes\n", bw);

        for(uint32_t i = 0; i < bw; i++) {
            uart_putc(buffer[i]);
        }
	} else {
        printf("Read of newfile.txt failed\n");
    }

    // Write demo
	if (f_open(&Fil, "testfile.txt", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {	/* Create a file */
		f_write(&Fil, "It works!\r\n", 11, &bw);	/* Write data to the file */
		f_close(&Fil);								/* Close the file */
    }    
    if (bw == 11) {
        printf("Write test pass\n");
    } else {
        printf("Write test fail (only %d letters written)\n", bw);
    }

	if (f_open(&Fil, "testfile.txt", FA_READ) == FR_OK) {	/* Open a file */
        uint8_t buffer[4096];
		f_read(&Fil, buffer, 4096, &bw);	/* Write data to the file */
		f_close(&Fil);								/* Close the file */
        printf("Read of testfile.txt success\n");
        printf("Read %d bytes\n", bw);
        for(uint32_t i = 0; i < bw; i++) {
            uart_putc(buffer[i]);
        }
	} else {
        printf("Open of testfile.txt failed\n");
    }

    // Directory demo
    char path[1024];
    strcpy(path, "0:");
    FRESULT res = scan_files(path);
    printf("rc=%d\n", res);

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

