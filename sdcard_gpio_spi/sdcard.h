#ifndef SDCARD_H
#define SDCARD_H

#include <stdint.h>

// Initialization of GPIO for SD CARD
void sdInitInterface();

// SD card inital.  set to SPI mode.
uint8_t sdInitCard();
int32_t sdTransferBlocks( int64_t address, int32_t numBlocks, uint8_t* buffer, int32_t write );
//-----
// write to SD card from rwbuffer  512 bytes block
int32_t sdWrite( uint32_t address, uint8_t *rwbuffer );

// read from SD card to rwbuffer  512bytes block
int32_t sdRead( uint32_t address, uint8_t *rwbuffer );


#endif  // SDCARD_H
