#include <stdbool.h>
#include "sdcard_cmd.h"
#include "sdcard.h"
#include "mylib.h"
#include "helper.h"

typedef enum SD_STATUS {
    STATE_IDLE = 0,
    STATE_READY = 1,
    STATE_IDENT = 2,
    STATE_STBY = 3,
    STATE_TRAN = 4,
    STATE_DATA = 5,
    STATE_RCV = 6,
    STATE_PRG = 7,
    STATE_DIS = 8
} SD_STATUS;

uint8_t sdState;

bool sd_interface_init_done = false;
bool sd_card_init_done = false;


// SD card inital.  set to BUS mode.
uint8_t sdInitCard()
{
    uint8_t resp[R2_RESP_LENGTH];
    memset(resp, 0, R2_RESP_LENGTH);
    int32_t ret;
    if (!sd_interface_init_done) {
        sdInitInterface();
        sd_interface_init_done = true;
    }
    set_clock_mode_slow();
    // dummy clock (>74) is needed after startup 
    send_clock(80);
    // reset
    LOG("CMD0 start\n");
    cmd0();
    set_rca(resp);
    sdState = STATE_IDLE;

    LOG("CMD8 start\n");
    if (cmd8(0x01, 0xaa, resp) < 0) {
        goto ERROR;
    }

    while (true) {
        LOG("CMD55 start\n");
        if (cmd55(resp) < 0) {
            goto ERROR;
        }
        if ((resp[4] & 0x020) == 0) {
            LOG("Not ready for ACMD\n");
            continue;
        }
        
        LOG("ACMD41 start\n");
        if (acmd41(resp) < 0) {
            goto ERROR;
        }
        if (check_acmd41_busy(resp) == 0) {
            LOG("The SD Card is ready\n");
            break;
        }
        LOG("Error: CMD41 busy. Not Ready. Retry\n");
        waitMicro(100000);
    }
    LOG("CMD2 start\n");
    if (cmd2(resp) < 0) {
        goto ERROR;
    }
    
    LOG("CMD3 start\n");
    if (cmd3(resp) < 0) {
        goto ERROR;
    }
    sdState = STATE_STBY;
    set_rca(resp);

    if (sdState != STATE_TRAN) {
        if (sdState != STATE_STBY) {
            LOG("Not in STBY. Can't move to TRAN.\n");
            goto ERROR;
        }
        if (moveToTranState()) {
            LOG("Failed to move to Trans state\n");
            goto ERROR;
        }
        sdState = STATE_TRAN;
    }

    LOG("Switch to wide bus mode\n");
    LOG("CMD55 start\n");
    if (cmd55(resp) < 0) {
        goto ERROR;
    }
    LOG("ACMD6 start\n");
    if (acmd6(resp) < 0) {
        goto ERROR;
    }
    LOG("Bus is switched to wide bus (4 bit) mode\n");

    sd_card_init_done = true;
    return 0;

ERROR:
    sd_card_init_done = false;
    LOG("Error\n");
    return -1;
}

// switch to high speed mode
int32_t sdHighSpeedMode()
{
    if (!sd_card_init_done) {
        LOG("sd card not initialized yet");
        return -1;
    }
    uint8_t resp[R1_RESP_LENGTH];
    uint8_t buffer[1024];
    const int function_group = 1;  // Access mode
    const int function = 1;  // High-speed/SDR25
    int32_t length = cmd6(function_group, function, resp, buffer);
    // SD CARD status is 512 bits = 64 bytes
    if (length != 64) {
        LOG("CMD6 failed %d\n", length);
        return -1;
    }
    LOGDUMP(buffer, 64);
    return 0;
}

uint8_t sdStatus() {
    uint8_t resp[R1_RESP_LENGTH];
    uint32_t ret = cmd13(resp, 0);
    sdState = (resp[3] >> 1) & 0x0F;
    return ret;
}

uint8_t sdGetCSDRegister(uint8_t *resp) {
    uint8_t cmd9_resp[R2_RESP_LENGTH];
    if (sdState != STATE_STBY) {
        if (sdState != STATE_TRAN) {
            LOG("NOT in TRAN. Can't move to STBY.\n");
            goto ERROR;
        }
        if (moveToStbyState()) {
            goto ERROR;
        }
        sdState = STATE_STBY;
    }
    LOG("CMD9 start\n");
    if (cmd9(cmd9_resp)) {
        return -1;
    }
    memcpy(resp, cmd9_resp + 1, 16);
    if (moveToTranState()) {
        goto ERROR;
    }
    return 0;
 ERROR:
    return -1;
}

int32_t sdCheckSCR()
{
    uint8_t resp[R1_RESP_LENGTH];
    uint8_t buffer[1024];
    if (cmd55(resp) < 0) {
        LOG("CMD55 failed\n");
        return -1;
    }
    int32_t length = acmd51(resp, buffer);
    // SCR is 64 bit = 8 bytes
    if (length < 8) {
        LOG("ACMD51 returned short SCR\n");
        return -1;
    }
    LOGDUMP(buffer, 8);
    LOG("SCR Structure: %2x\n", (buffer[0] >> 4) & 0x0F);
    LOG("Spec Version: %2x\n", (buffer[0] & 0x0f));
    LOG("DAT Bus Width Support: %2x\n", buffer[1] & 0x0f);
    LOG("Set Block Count command (CMD23) Support: %2x\n", (buffer[4] >> 1) & 1);
    return 0;
}

//-----
// read from SD card to rwbuffer  512bytes block
int32_t sdRead( uint32_t address , uint8_t *buffer)
{
    uint8_t resp[R1_RESP_LENGTH];
    uint32_t ret;
    set_clock_mode_fast();
    ret = cmd17(address, resp, buffer);
    set_clock_mode_slow();
    return ret;
}

int32_t sdReadMulti( uint32_t address, uint32_t block_count, uint8_t *buffer)
{
    uint8_t resp[R1_RESP_LENGTH];
    int32_t ret;
    set_clock_mode_fast();
    ret = cmd18(address, block_count, resp, buffer);
    if (ret < 0) {
        LOG("CMD18 command error\n");
    }
 error:
    if (cmd12(resp) < 0) {
        LOG("CMD12 command error\n");
        ret = -1;
    }
 exit:
    set_clock_mode_slow();
    return ret;
}

// Write to SD card from buffer. One 512bytes block.
int32_t sdWrite(uint32_t address, const uint8_t *buffer)
{
    uint8_t resp[R1_RESP_LENGTH];
    uint32_t ret;
    set_clock_mode_fast();
    if (cmd24(address, resp, buffer)) {
        LOG("CMD24 error\n");
        goto ERROR;
    }
    
    uint32_t data_cnt = write_data(512, buffer);
    send_clock(8);
    set_clock_mode_slow();
    return data_cnt;
    
 ERROR:
    if (cmd12(resp) < 0) {
        LOG("CMD12 command error\n");
    }
    send_clock(8);
    set_clock_mode_slow();
    return -1;
}

// Writes TO SD Card multiple blocks.
int32_t sdWriteMulti( uint32_t address, uint32_t num_blocks, const uint8_t *buffer)
{
    uint8_t resp[R1_RESP_LENGTH];
    uint32_t ret;
    set_clock_mode_fast();
    if (cmd25(address, resp, buffer)) {
        LOG("CMD25 error\n");
        goto EXIT;
    }

    uint32_t total_data_cnt = 0;
    for (int block_cnt = 0; block_cnt < num_blocks; block_cnt++) {
        int data_cnt = write_data(512, buffer + 512 * block_cnt);
        total_data_cnt += data_cnt;
        if (data_cnt < 512) {
            // something wrong with transfer
            LOG("Blcok number #%d transfer bytes = %d"
                   "(512 expected). Abort.\n", block_cnt, data_cnt);
            goto EXIT;
        }
    }
    
 EXIT:
    if (cmd12(resp) < 0) {
        LOG("CMD12 command time out\n");
    }
    send_clock(8);
    set_clock_mode_slow();
    return total_data_cnt;
}

int32_t sdTransferBlocks( int64_t address, int32_t numBlocks, uint8_t* buffer, int32_t write ) {
    uint32_t ret;
    const uint32_t block_size = 512;
    
    if (numBlocks == 1) {
        if (write == 0) {
            ret = sdRead(address/512, buffer);
        } else {
            ret = sdWrite(address/512, (const uint8_t*) buffer);
        }
    } else {
        if (write == 0) {
            ret = sdReadMulti(address/512, numBlocks, buffer);
        } else {
            ret = sdWriteMulti(address/512, numBlocks, (const uint8_t*) buffer);
        }
    }
    if (ret != numBlocks * block_size) {
        LOG("Number of transferred byte size (%d) is smaller than target (%d * %d = %d)\n",
            ret, numBlocks, block_size, numBlocks * block_size);
        return -1;
    }
    return 0;
}

