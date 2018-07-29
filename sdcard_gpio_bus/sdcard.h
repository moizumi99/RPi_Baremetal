#ifndef SDCARD_H
#define SDCARD_H

#include <stdint.h>

// Initialization of GPIO for SD CARD
void sdInitInterface();

// SD card inital.  set to SPI mode.
uint8_t sdInitCard();
int32_t sdTransferBlocks( int64_t address, int32_t numBlocks, uint8_t* buffer, int32_t write );
int32_t sdCheckSCR();
//-----
// Switch to high speed (25MHz) mode
int32_t sdHighSpeedMode();
    
/* // write to SD card from rwbuffer  512 bytes block */
/* int32_t sdWrite( uint32_t address, uint8_t *rwbuffer ); */

/* // read from SD card to rwbuffer  512bytes block */
/* int32_t sdReadSingle( uint32_t address, uint8_t *rwbuffer ); */
/* int32_t sdReadBlocks( uint32_t address, uint32_t block_length, uint8_t *rwbuffer ); */


#endif  // SDCARD_H
