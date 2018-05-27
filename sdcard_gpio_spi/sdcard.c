#include "sdcard.h"
#include "mmio.h"
#include "gpio.h"
#include "mylib.h"
#include "helper.h"

void card_dout(int32_t d);
void card_clk(int32_t d);
uint8_t card_din();
void card_cson();
void card_csoff();
// send & get SPI data
uint8_t send_cmd( uint8_t c );
// get R1 responce
uint8_t command_response();

// wait time for initialization period
// 2 is actually 250 KHz but should be OK
#define WAIT_400KHZ 2

// TODO: define the correct value
#define CLK_WAIT_20MHZ 2

typedef enum CLK_SPEED {
    CLK_400KHZ = 0,
    CLK_20MHZ = 1 } CLK_SPEED;

uint32_t c_size;
uint32_t card_size;
CLK_SPEED clock_status = CLK_400KHZ;

void card_dout(int32_t d) {
    if (d == 0) {
        gpioClear(49);
    } else {
        gpioSet(49);
    }
}

void card_clk(int32_t d) {
    if (d) {
        gpioSet(48);
    } else {
        gpioClear(48);
    }
    /* waitMicro(CLK_WAIT_400KHZ); */
    if (clock_status == CLK_400KHZ) {
        waitMicro(WAIT_400KHZ);
    } else {
        wait(CLK_WAIT_20MHZ);
    }
}

uint8_t card_din() {
    return gpioRead(50);
}

void card_cson() {
    // CS ON = Low
    gpioClear(53);
}

void card_csoff() {
    // CS OFF = High
    gpioSet(53);
}

uint8_t crc7(const uint8_t *buff, int32_t len )
{
    uint8_t crc = 0;

    while( len-- ) {
        uint8_t byte = *buff++;
        int32_t bits = 8;

        while( bits-- ) {
            crc <<= 1;
            if( (crc ^ byte) & 0x80 ) {
                crc ^= 0x89;
            }
            byte <<= 1;
        }
    }
    return( crc );
}

// send & get SPI data
uint8_t send_cmd( uint8_t c) {
    uint8_t ret = 0;
    for (int32_t i = 0; i < 8; i++) {
        card_dout(c & 0x80);
        card_clk(1);
        ret = (ret << 1) | card_din();
        card_clk(0);
        c <<= 1;
    }
    card_dout(1);
    
    return ret;
}


// get R1 responce
uint8_t command_response()
{
    int32_t timeout = 10;
    uint8_t ret;

    ret = 0xff;
    while ( ret == 0xff && timeout-- > 0 ) {
        ret = send_cmd( 0xff );
    }
    return ret;
}

void wait_for_cmd_rdy() {
    uint8_t ret = 0xff;
    int32_t n = 10;
    while (ret != 0xff && --n>0) {
        ret = send_cmd( 0xff );
    }
    if (n == 0) {
        printf("Error. SD CARD timed out while waiting for ready status\n");
    }
}


int bit_check(uint8_t din, int32_t pos)
{
    return (din >> pos) & 0x01;
}

void decode_R1(uint8_t result)
{
    if (bit_check(result, 0))
        printf("In Idle state: high\n");
    if (bit_check(result, 1))
        printf("Erase reset: high\n");
    if (bit_check(result, 2))
        printf("Illegal Command: high\n");
    if (bit_check(result, 3))
        printf("COM CRC error: high\n");
    if (bit_check(result, 4)) 
        printf("Erase sequence error: high\n");
    if (bit_check(result, 5))
        printf("Address error: high \n");
    if (bit_check(result, 6))
        printf("Parameter error: high \n");
    if (bit_check(result, 7))
        printf("Always zero: high \n");
}

void decode_R2(uint8_t result)
{
    if (bit_check(result, 0))
        printf("Card is locked\n");
    if (bit_check(result, 1))
        printf("WP erase skip or lock/unlock cmd failed\n");
    if (bit_check(result, 2))
        printf("Error\n");
    if (bit_check(result, 3))
        printf("CC error\n");
    if (bit_check(result, 4))
        printf("card ecc failed\n");
    if (bit_check(result, 5))
        printf("WP violation\n");
    if (bit_check(result, 6))
        printf("Erase param\n");
    if (bit_check(result, 7))
        printf("out of range or csd ocerwrite\n");
}

void decode_OCR(uint8_t *ocr)
{
    printf("OCR: %02X%02X%02X%02X\n", ocr[0], ocr[1], ocr[2], ocr[3]);
    printf("Low Voltage Range: %d\n", bit_check(ocr[3], 7));
    printf("2.7 - 2.8: %d\n", bit_check(ocr[2], 7));
    printf("2.8 - 2.9: %d\n", bit_check(ocr[1], 0));
    printf("2.9 - 3.0: %d\n", bit_check(ocr[1], 1));
    printf("3.0 - 3.1: %d\n", bit_check(ocr[1], 2));
    printf("3.1 - 3.2: %d\n", bit_check(ocr[1], 3));
    printf("3.2 - 3.3: %d\n", bit_check(ocr[1], 4));
    printf("3.3 - 3.4: %d\n", bit_check(ocr[1], 5));
    printf("3.4 - 3.5: %d\n", bit_check(ocr[1], 6));
    printf("3.5 - 3.6: %d\n", bit_check(ocr[1], 7));
    printf("Switching to 1.8V Accepted (S18A): %d\n", bit_check(ocr[0], 0));
    printf("UHS-II Card Status: %d\n", bit_check(ocr[0], 5));
    printf("Card Capacity Status (CCS) : %d\n", bit_check(ocr[0], 6));
    if (bit_check(ocr[0], 6)) {
        printf("This SDCARD is either SDHC or SDXC\n");
    } else {
        printf("This SDCARD is SDSC\n");
    }
    printf("Card power up status bit (busy): %d\n", bit_check(ocr[0], 7));
    printf("If busy is low, the card has not finished the power up routine\n");
}

uint32_t decode_CSDv1(uint8_t *csd)
{
    printf("CSD v1\n");
    //    *c_size = ((csd[7] & 0b11000000) >> 6)+ (csd[8] << 2) + ((csd[9] & 0b00000011) << 10);
    uint32_t c_size = cut_bits(csd, 62, 73);
    uint32_t c_size_mult = cut_bits(csd, 47, 49);
    uint32_t mult = (1 << (c_size_mult + 2));
    uint32_t blocknr = (c_size + 1) * mult;
    uint32_t read_bl_len = cut_bits(csd, 80, 83);
    uint32_t block_len = 1 << read_bl_len;
    uint32_t card_size = blocknr * block_len;
    return card_size;
}

uint32_t decode_CSDv2(uint8_t *csd)
{
    printf("CSD v2\n");
    // *c_size = csd[6] + (csd[7] << 8) + ((csd[8] & 0b00111111) << 16);
    uint32_t c_size = cut_bits(csd, 48, 69);
    uint32_t card_size = (c_size + 1) * 512;
    return card_size;
}

uint32_t decode_CSD(uint8_t *csd)
{
    int32_t csd_structure = (csd[15] >> 6) & 0b11;
    if (csd_structure == 0)
        return decode_CSDv1(csd);
    if (csd_structure == 1)
        return decode_CSDv2(csd);
    printf("Unsupported CSD version: %d\n", csd_structure);
    return 0;
}

void send_cmd_array(uint8_t *cmd_array)
{
    send_cmd(cmd_array[0]);
    send_cmd(cmd_array[1]);
    send_cmd(cmd_array[2]);
    send_cmd(cmd_array[3]);
    send_cmd(cmd_array[4]);
    send_cmd(cmd_array[5]);
}

uint8_t cmd_short_response(uint8_t *cmd_array)
{
    wait_for_cmd_rdy();
    send_cmd_array(cmd_array);
    uint8_t ret = command_response();
    return ret;
}

uint8_t cmd_long_response(uint8_t *cmd_array,
                                uint8_t *resp)
{
    wait_for_cmd_rdy();
    send_cmd_array(cmd_array);
    uint8_t ret = command_response();
    resp[0] = command_response();
    resp[1] = command_response();
    resp[2] = command_response();
    resp[3] = command_response();
    return ret;
}

void add_crc(uint8_t *cmd_array)
{
    uint8_t crc = (crc7(cmd_array, 5) << 1) & 0x0ff;
    crc = crc | 1;
    cmd_array[5] = crc;
}

uint8_t cmd(uint8_t cmd_num, uint32_t arguments)
{
    uint8_t ret;
    uint8_t cmd_array[] = {0x40 | cmd_num, 0, 0, 0, 0, 0xff};
    cmd_array[1] = (arguments >> 24) & 0xff;
    cmd_array[2] = (arguments >> 16) & 0xff;
    cmd_array[3] = (arguments >>  8) & 0xff;
    cmd_array[4] = (arguments      ) & 0xff;
    switch(cmd_num) {
    case 0:
    case 10:
    case 16:
    case 18:
    case 22:  // ACMD22
    case 25:
    case 27:
    case 32:
    case 33:
    case 42:
    case 51:  // ACMD51
        add_crc(cmd_array);
        break;
    case 17:
    case 24:
        break;
    case 55:
        cmd_array[5] = 0x65;
        break;
    default:
        printf("cmd%d can be issued with simple cmd(n)\n", cmd_num);
        return -1;
    }
    ret = cmd_short_response(cmd_array);
    return ret;
}

uint8_t cmd0_reset()
{
    uint8_t cmd_array[] = {0x40, 0, 0, 0, 0, 0xff};
    add_crc(cmd_array);
    wait_for_cmd_rdy();
    send_cmd_array(cmd_array);
    return send_cmd( 0xff );
}


uint8_t cmd1(uint8_t HCS)
{
    uint8_t cmd_array[] = {0x40 | 41, 0, 0, 0, 0, 0x77};
    cmd_array[1] = (HCS) ? 0x40 : 0;
    uint8_t ret = cmd_short_response(cmd_array);
    return ret;
}

uint8_t cmd8(uint8_t vhs,
                   uint8_t checkpattern, uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 8, 0, 0, vhs, checkpattern, 0xff};
    add_crc(cmd_array);
    uint8_t ret = cmd_long_response(cmd_array, resp);
    return ret;
}

uint8_t cmd9(uint8_t *csd)
{
    uint8_t cmd_array[] = {0x40 | 9, 0, 0, 0, 0, 0xff};
    uint8_t ret = cmd_short_response(cmd_array);
    uint8_t response = 0;
    while ( response != 0xfe ) {
        response = send_cmd( 0xff );
    }
    for (uint32_t i = 0; i < 16; i++) {
        csd[i] = send_cmd( 0xff );
    }
    return ret;
}

uint8_t cmd58(uint8_t *resp)
{
    uint8_t arg = 0x0ff & (0x40 | 58);
    uint8_t cmd_array[] = {arg, 0, 0, 0, 0, 0xff};
    add_crc(cmd_array);
    uint8_t ret = cmd_long_response(cmd_array, resp);
    return ret;
}

uint8_t acmd41(uint8_t HCS)
{
    uint8_t cmd_array[] = {0x40 | 41, 0, 0, 0, 0, 0x77};
    cmd_array[1] = (HCS) ? 0x40 : 0;
    uint8_t ret = cmd_short_response(cmd_array);
    return ret;
}

void set_cs_pin_input()
{
    gpioSetFunction(53, GPIO_INPUT);
}

void set_cs_pin_output()
{
    gpioSetFunction(53, GPIO_OUTPUT);
}

// Initialization of GPIO for SD CARD
void sdInitInterface()
{
    // GPIO SDC IF setup

    // disable EMMC by disabling CLK_EN and CLK_INTLEN
    mmio_set(EMMC_CONTROL0, 0x05, 0x00);

    // set up GPIO pins to
    // 47 : SD_CARD_DET -> Read
    // 48 : SD_CLK_R -> Write
    // 49 : SD_CMD_R -> Write
    // 50 : SD_DATA_0 -> Read (not used for SPI mode)
    // 51 : SD_DATA_1 -> Read (not used for SPI mode)
    // 52 : SD_DATA_2 -> Read (not used for SPI mode)
    // 53 : SD_DATA_3 (CS) -> Read (this is changed in sdcard init)
    gpioSetFunction(47, GPIO_INPUT);
    gpioSetFunction(48, GPIO_OUTPUT);
    gpioSetFunction(49, GPIO_OUTPUT);
    gpioSetFunction(50, GPIO_INPUT);
    gpioSetFunction(51, GPIO_INPUT);
    gpioSetFunction(52, GPIO_INPUT);
    gpioSetFunction(53, GPIO_INPUT);
    gpioSetPull(47, GPIO_PULLUPDOWN_UP);
    gpioSetPull(49, GPIO_PULLUPDOWN_UP);
    gpioSetPull(50, GPIO_PULLUPDOWN_UP);
    gpioSetPull(51, GPIO_PULLUPDOWN_UP);
    gpioSetPull(52, GPIO_PULLUPDOWN_UP);
    gpioSetPull(53, GPIO_PULLUPDOWN_UP);

    // wait until the change takes effect
    wait(150);
}

// SD card inital.  set to SPI mode.
uint8_t sdInitCard()
{
    uint8_t ret = 0xff, ret2 = 0xff;
    uint8_t resp[4], csd[16];

    // The sequence is explained in Figure 7-2 of SD card spec
    // Following is an overview of the flow
    // Dummy cycle of 74 or more
    // CMD0 (CS hi)
    // CMD0 (CS Low - asserted)
    // (This puts the SD CARD into SPI mode)
    // CMD8 (Check supported voltage range and card type)
    // CMD58 (Read OCR and chaeck the card voltage)
    // CMD55 (Part of ACMD41)
    // ACMD41 (Initialization command)
    // CMD58 (Get CCS)

    // Issue CMD0 to bring the status to IDLE
    clock_status = CLK_400KHZ;
    set_cs_pin_input();
    // dummy 74 clock is needed after startup
    for(int32_t i=0; i<10; i++) {
        send_cmd( 0xff );
    }
    ret = cmd0_reset();
    printf("CMD0 (no CS) is issed\n");
  
    // Switch to SPI mode
    set_cs_pin_output();
    card_csoff();
    uint8_t HCS = 1;  // Todo: change this depending on cmd58 response
    ret2 = 0xff;
    int32_t retry = 2;
    while ( ret2 != 0x00 & retry-- > 0) {
        for(int32_t i=0; i<10; i++) {
            send_cmd( 0xff );
        }
        card_cson();
    
        ret = 0xff;
        while (ret != 1) {
            ret = cmd(0, 0);
            printf("CMD0 res: %u\n", ret);
            decode_R1(ret);
            if (ret != 1) {
                waitMicro(1000000);
            }
        }

        ret = cmd8(0x01, 0xaa, resp);
        printf("CMD8 res: %x\n", ret);
        decode_R1(ret);
        printf("CMD version (00 expected): %02x\n", resp[0]);
        printf("Voltage Accepted (01 expected): %02x\n", resp[2]);
        printf("Check Pattern (aa expected): %02x\n", resp[3]);
        // check for illegal command or non-compatible card
        if (ret & 4) {
            printf("Error: CMD8 illegal command response. Possibly Ver1.x SDSC\n");
            return ret;
        } else if (ret != 1 || resp[2] != 1 || resp[3] != 0xaa) {
            printf("Error: CMD8 Error. Not compatible\n");
            return ret;
        }

        // Read OCR
        ret = cmd58(resp);
        printf("CMD58 res: %x\n", ret);
        decode_R1(ret);
        printf("CMD58 response: %02x%02x%02x%02x\n",
               resp[0], resp[1], resp[2], resp[3]);
        if (ret != 1) {
            printf("Error: CMD58 Error. Not compatible\n");
            return ret;
        }

        int32_t cmd_retry = 4;
        while (--cmd_retry > 0) {
            ret = cmd(55, 0);
            printf("CMD55 res: %x\n", ret);
            decode_R1(ret);
            wait_for_cmd_rdy();
      
            ret2 = acmd41(HCS);
            printf("ACMD41 res: %x\n", ret2);
            decode_R1(ret2);
      
            wait_for_cmd_rdy();
            if ( ret2 != 0x00 ) {
                waitMicro(1000000);
            }
        }
    }
    if (ret2 != 0) {
        // initialization error
        return ret2;
    }
    
    ret = cmd9(csd);
    printf("CMD9 res: %u\n", ret);
    decode_R1(ret);
    c_size = decode_CSD(csd);
    card_size = c_size * 512;
    printf("SD CARD size is %u KiB\n", card_size);
    printf(" %u MiB\n", card_size / 1024);
    
    ret = send_cmd( 0xff );
    ret = send_cmd( 0xff );
    printf("init res: %u\n", ret);

    ret = cmd58(resp);
    printf("CMD58 res: %x\n", ret);
    decode_R1(ret);
    decode_OCR(resp);
    
    send_cmd( 0xff );  // let the last command finish completely
  
    clock_status = CLK_20MHZ;
    card_csoff();

    return 0;
}

// write to SD card from rwbuffer 512 bytes block
int32_t sdWrite( uint32_t address, uint8_t *rwbuffer )
{
    uint8_t    *p;
    uint8_t    ret, status;

    card_cson();

    if ((ret = cmd(24, address)) != 0) {
        goto sdWriteError;
    }

    // Start Block Token for single block write (Spec 7.3.3.2)
    send_cmd( 0xfe );       
    for (int i = 0; i < 512; i++ ) {
        send_cmd( *rwbuffer++ );
    }
    // TODO: add CRC check
    send_cmd( 0xff ); // crc0
    send_cmd( 0xff ); // crc1

    ret = command_response();
    // DATA Response Token
    // Refer to Spec. 7.3.3. Control Token
    //  7  6  5  4  3  2 1  0
    //  x  x  x  0  Status  1
    //  Status = 010 - Data accepted
    //  Status = 101 - Data rejected due to a CRC error
    //  Status = 110 - Data rejected due to a Write error
    status = (ret & 0x0e) >> 1;
    if ( status != 0b010 ) {
        goto sdWriteError;
    }
    
    int32_t timeout = 10000;
    // SD CARD generates busy token (0x00)
    while( send_cmd( 0xff ) == 0x00 && timeout-- > 0) {
        wait(100);
    }
    if (timeout < 0) {
        goto sdWriteError;
    }

    // dummy clock
    send_cmd( 0xff );
    
    card_csoff();
    return 512;

 sdWriteError:
    card_csoff();
    printf("Error: SD CARD write error\n");
    return -1;
}

//-----
// read from SD card to rwbuffer  512bytes block
int32_t sdRead( uint32_t address , uint8_t *rwbuffer)
{
    uint8_t    *p;
    uint8_t    ret;
    int32_t     timeout;

    card_cson();
    if ((ret = cmd(17, address)) != 0) {
        goto sdReadError;
    }
    
    timeout = 100;
    // wait for Data Token Start byte 0xfe
    while( send_cmd( 0xff ) != 0xfe && timeout-- > 0) ;
    if ( timeout < 0 ) {
        goto sdReadError;
    }
    
    for (int32_t i=0; i < 512; i++ ) {
        *(rwbuffer++) = send_cmd( 0xff );
    }
    
    // TODO: add CRC check
    send_cmd( 0xff ); // CRC0
    send_cmd( 0xff ); // CRC1
    
    card_csoff();
    return 512;

 sdReadError:
    card_csoff();
    printf("Error: SD CARD read error (%u)\n", ret);
    return 1;
}

int32_t sdTransferBlocks( int64_t address, int32_t numBlocks, uint8_t* buffer, int32_t write ) {
    uint8_t ret;
    for (int32_t block = 0; block < numBlocks; block++) {
        if (write == 0) {
            ret = sdRead(address/512, buffer);
        } else {
            ret = sdWrite(address/512, buffer);
        }
        if (ret != 0) {
            printf("SD CARD Read Error: %d\n", (int32_t) ret);
            return -1;
        }
        address += 512;
        buffer += 512;
    }
    return 0;
}
