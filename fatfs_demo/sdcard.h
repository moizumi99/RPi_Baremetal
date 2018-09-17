#ifndef SDCARD_H
#define SDCARD_H

#include <stdint.h>

// Check the sdcard status. Returns zero if readable
uint8_t sdStatus();
uint8_t sdGetCSDRegister(uint8_t *resp);
    
// SD card inital.  set to SPI mode.
uint8_t sdInitCard();
int32_t sdTransferBlocks( int64_t address, int32_t numBlocks, uint8_t* buffer, int32_t write );
int32_t sdCheckSCR();
//-----
// Switch to high speed (25MHz) mode
int32_t sdHighSpeedMode();
    
/* // write to SD card from rwbuffer  512 bytes block */
int32_t sdWrite( uint32_t address, const uint8_t *buffer );
int32_t sdWriteMulti( uint32_t address, uint32_t num_blocks, const uint8_t *buffer );

/* // read from SD card to rwbuffer  512bytes block */
int32_t sdRead( uint32_t address, uint8_t *buffer );
int32_t sdReadMulti( uint32_t address, uint32_t block_length, uint8_t *buffer );

#define SDREAD 0
#define SDWRITE 1

#endif  // SDCARD_H
