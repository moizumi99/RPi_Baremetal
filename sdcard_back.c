#include "sdcard.h"
#include "mmio.h"
#include "gpio.h"
#include "mylib.h"

unsigned char rwbuffer[512];

enum {
    CRC_ON = 1,
    CRC_OFF = 0
};

void card_dout(int d) {
    if (d) {
        gpioSet(49);
    } else {
        gpioClear(49);
    }
}

void card_clk(int d) {
    if (d) {
        gpioSet(48);
    } else {
        gpioClear(48);
    }
    wait(100);
}

unsigned char card_din() {
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

uint8_t calcCRC7(const uint8_t *buff, int len )
{
    uint8_t crc = 0;

    while( len-- ) {
        uint8_t byte = *buff++;
        int bits = 8;

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
unsigned char spi_tx( unsigned char c )
{
	unsigned char    ret;

	ret = 0;
    if( c & 0x80 ) card_dout(1); else card_dout(0);
    card_clk(1);
    if ( card_din() ) ret |= 0x80;
    card_clk(0);

    if( c & 0x40 ) card_dout(1); else card_dout(0);
    card_clk(1);
    if ( card_din() ) ret |= 0x40;
    card_clk(0);

    if( c & 0x20 ) card_dout(1); else card_dout(0);
    card_clk(1);
    if ( card_din() ) ret |= 0x20;
    card_clk(0);

    if( c & 0x10 ) card_dout(1); else card_dout(0);
    card_clk(1);
    if ( card_din() ) ret |= 0x10;
    card_clk(0);

    if( c & 0x08 ) card_dout(1); else card_dout(0);
    card_clk(1);
    if ( card_din() ) ret |= 0x08;
    card_clk(0);

    if( c & 0x04 ) card_dout(1); else card_dout(0);
    card_clk(1);
    if ( card_din() ) ret |= 0x04;
    card_clk(0);

    if( c & 0x02 ) card_dout(1); else card_dout(0);
    card_clk(1);
    if ( card_din() ) ret |= 0x02;
    card_clk(0);

    if( c & 0x01 ) card_dout(1); else card_dout(0);
    card_clk(1);
    if ( card_din() ) ret |= 0x01;
    card_clk(0);

    card_dout(1);
    return ret;
}

//-----
// get R1 responce
unsigned char spi_getresponce(int n, unsigned char nexp)
{
	unsigned char    ret, loops;

    for ( loops = 0; loops < n; loops++ ) {
        ret = spi_tx( 0xff );
        if ( ret != nexp )
            break;
    }
    return ret;
}

void usleep(int t)
{
    waitMicro(t);
}

int bit_check(unsigned char din, int pos)
{
    return (din >> pos) & 0x01;
}

void decode_R1(unsigned char result)
{
    if ( result == 0)
        printf("R1 response all zero\n");
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

void decode_OCR(unsigned char *ocr)
{
    printf("Low Voltage Range: %d\n", bit_check(ocr[0], 7));
    printf("2.7 - 2.8: %d\n", bit_check(ocr[1], 7));
    printf("2.8 - 2.9: %d\n", bit_check(ocr[2], 0));
    printf("2.9 - 3.0: %d\n", bit_check(ocr[2], 1));
    printf("3.0 - 3.1: %d\n", bit_check(ocr[2], 2));
    printf("3.1 - 3.2: %d\n", bit_check(ocr[2], 3));
    printf("3.2 - 3.3: %d\n", bit_check(ocr[2], 4));
    printf("3.3 - 3.4: %d\n", bit_check(ocr[2], 5));
    printf("3.4 - 3.5: %d\n", bit_check(ocr[2], 6));
    printf("3.5 - 3.6: %d\n", bit_check(ocr[2], 7));
    printf("Switching to 1.8V Accepted (S18A): %d\n", bit_check(ocr[3], 0));
    printf("UHS-II Card Status: %d\n", bit_check(ocr[3], 5));
    printf("Card Capacity Status (CCS) : %d\n", bit_check(ocr[3], 6));
    if (bit_check(ocr[3], 6)) {
        printf("This SDCARD is either SDHC or SDXC\n");
    } else {
        printf("This SDCARD is SDSC\n");
    }
    printf("Card power up status bit (busy): %d\n", bit_check(ocr[3], 7));
    printf("If busy is low, the card has not finished the power up routine\n");
}

void send_cmd(unsigned char *cmd_array)
{
    spi_tx(cmd_array[0]);
    spi_tx(cmd_array[1]);
    spi_tx(cmd_array[2]);
    spi_tx(cmd_array[3]);
    spi_tx(cmd_array[4]);
    spi_tx(cmd_array[5]);
}

unsigned char cmd_short_response(unsigned char *cmd_array)
{
    send_cmd(cmd_array);
    unsigned char ret = spi_getresponce(10, 0xff);
    spi_tx(0xff); // dummy clock
    return ret;
}

unsigned char cmd_long_response(unsigned char *cmd_array,
                                unsigned char *resp)
{
    send_cmd(cmd_array);
    unsigned char ret = spi_getresponce(10, 0xff);
    resp[0] = spi_getresponce(10, 0xff);
    resp[1] = spi_getresponce(10, 0xff);
    resp[2] = spi_getresponce(10, 0xff);
    resp[3] = spi_getresponce(10, 0xff);
    spi_tx(0xff); // dummy clock
    return ret;
}

void add_crc(unsigned char *cmd_array)
{
    unsigned char crc = (calcCRC7(cmd_array, 5) << 1) | 1;
    cmd_array[5] = crc;
}

unsigned char cmd(unsigned char cmd_num)
{
    switch(cmd_num) {
        unsigned char cmd_array[] = {0x40 | cmd_num, 0, 0, 0, 0xff};
    case 0:
    case 10:
    case 16:
    case 18:
    case 22:  // ACMD22
    case 24:
    case 25:
    case 27:
    case 32:
    case 33:
    case 42:
    case 51:  // ACMD51
        add_crc(cmd_array);
    case 9:
    case 17:
    case 55:
        return cmd_short_response(cmd_array);
    default:
        printf("cmd%d needs arguments or has long output\n", cmd_num);
        printf("Cannot be issued with simple cmd(n)\n");
        return -1;
    }
}

unsigned char cmd8(unsigned char vhs,
                   unsigned char checkpattern, unsigned char *resp)
{
    unsigned char cmd_array[] = {0x40 | 8, 0, 0, vhs, checkpattern, 0xff};
    return cmd_long_response(cmd_array, resp);
}

unsigned char cmd58(unsigned char *resp)
{
    unsigned char arg = 0x0ff & (0x40 | 58);
    unsigned char cmd_array[] = {arg, 0, 0, 0, 0, 0xff};
    add_crc(cmd_array);
    return cmd_long_response(cmd_array, resp);
}

unsigned char acmd41(unsigned char HCS)
{
    unsigned char cmd_array[] = {0x40 | 41, 0, 0, 0, 0, 0xff};
    if (HCS) {
        cmd_array[1] = 0x40;
    }
    return cmd_short_response(cmd_array);
}

// SD card inital.  set to SPI mode.
unsigned char sd_init()
{
    unsigned char    ret, ret2;
    uint8_t resp[4];

    printf("start initialization\n");
    spi_tx( 0xff );                 // output dummy 80 clock
    spi_tx( 0xff );
    spi_tx( 0xff );
    spi_tx( 0xff );
    spi_tx( 0xff );
    spi_tx( 0xff );
    spi_tx( 0xff );
    spi_tx( 0xff );
    spi_tx( 0xff );
    spi_tx( 0xff );
    usleep(20); // 10 us

    card_cson();
    ret = 0xff;
    while (ret != 1) {
        spi_tx( 0x40 );     // CMD0
        spi_tx( 0x00 );
        spi_tx( 0x00 );
        spi_tx( 0x00 );
        spi_tx( 0x00 );
        spi_tx( 0x95 );
        ret = spi_getresponce(10, 0xff);
        spi_tx( 0xff);
        printf("CMD0 res: %u\n", ret);
        decode_R1(ret);
        if (ret != 1) {
            usleep(1000000);
        }
    }

    spi_tx( 0x48 );     // CMD8
    spi_tx( 0x00 );
    spi_tx( 0x00 );
    spi_tx( 0x01 );
    spi_tx( 0xaa );
    spi_tx( 0x87 ); // need correct value

    ret = spi_getresponce(10, 0xff); // gets 0x01
    printf("CMD8 res: %x\n", ret);
    decode_R1(ret);
    ret = spi_tx( 0xff ); // gets 0x00
    printf("CMD version (00 expected): %02x\n", ret);
    ret = spi_tx( 0xff ); // gets 0x00
    printf("Reserved (00 expected): %02x\n", ret);
    ret = spi_tx( 0xff ); // gets 0x01
    printf("Voltage Accepted (01 expected): %02x\n", ret);
    ret = spi_tx( 0xff ); // gets 0xaa
    printf("Check Pattern (aa expected): %02x\n", ret);
    spi_tx( 0xff);
    
    /* ret = cmd8(0x01, 0xaa, resp); */
    /* printf("CMD8 res: %x\n", ret); */
    /* decode_R1(ret); */
    /* printf("CMD version (00 expected): %x\n", resp[0]); */
    /* printf("Reserved (00 expected): %x\n", resp[1]); */
    /* printf("Voltage Accepted (01 expected): %x\n", resp[2]); */
    /* printf("Check Pattern (aa expected): %x\n", resp[3]); */


    /* ret = cmd58(resp); */
    /* printf("CMD58 res: %x\n", ret); */
    /* decode_R1(ret); */
    /* decode_OCR(resp); */
    
    while ( ret != 0x00 || ret2 != 0x00 ) {

        spi_tx( 0x77 );     // CMD55
        spi_tx( 0x00 );
        spi_tx( 0x00 );
        spi_tx( 0x00 );
        spi_tx( 0x00 );
        spi_tx( 0xff );
        ret = spi_getresponce(10, 0xff); // gets 0x00
        spi_tx( 0xff);
        
        /* ret = cmd(55); */
        printf("CMD55 res: %x\n", ret);
        decode_R1(ret);

        spi_tx( 0x69 );
        spi_tx( 0x40 );
        spi_tx( 0x00 );
        spi_tx( 0x00 );
        spi_tx( 0x00 );
        spi_tx( 0xff );
        ret2 = spi_getresponce(10, 0xff); // gets 0x00
        spi_tx( 0xff);

        /* unsigned char HCS = 1; */
        /* ret2 = acmd41(HCS); */
        printf("ACMD41 res: %x\n", ret2);
        decode_R1(ret2);

        if ( ret != 0x00 || ret2 != 0x00 ) {
            waitMicro(1000000);
        }
   }

    spi_tx( 0x49 );     // CMD9
    spi_tx( 0x00 );
    spi_tx( 0x00 );
    spi_tx( 0x00 );
    spi_tx( 0x00 );
    spi_tx( 0xff ); //

    ret = spi_tx( 0xff );
    printf("CMD9 res: %u\n", ret);
    while ( ret != 0xfe ) {
        ret = spi_tx( 0xff );
    }
    for (int i = 0; i < 16; i++) ret = spi_tx( 0xff );
    ret = spi_tx( 0xff );
    ret = spi_tx( 0xff );

    card_csoff();
//////    spi_tx( 0xff );

    printf("init res: %u\n", ret);
    return ret;
}

//-----
// write to SD card from rwbuffer  512 bytes block
unsigned char sd_write( long address )
{
	unsigned char    *p;
	unsigned char    ret;
    int     i, j;

    card_cson();

    spi_tx( 0x40 + 24 );        // CMD24 write single block
    spi_tx( address >> 24 );
    spi_tx( address >> 16 );
    spi_tx( address >>  8 );
    spi_tx( address  );
    spi_tx( 0xff );

    ret = spi_getresponce(10, 0xff);
    if ( ret != 0 )
        return ret;

    spi_tx( 0xfe );             // block start code
    for ( p = rwbuffer, i = 0; i < 512; i++ )
        spi_tx( *p++ );
    spi_tx( 0xff );             // CRC 2bytes
    spi_tx( 0xff );

    ret = spi_getresponce(10, 0xff) & 0x1f;
    // data responce : xxx0sss1
    //  010  data accepted
    //  101  data rejected due to a CRC error
    //  110  data rejected due to a write error
    if ( ret == 0x05 ) {
        for ( i = 0; i < 10000; i++ ) { // by iwata
            ret = spi_tx( 0xff );
            for ( j = 0; j < 25; j++ ) ;    // *** polling delay is must ***
            //cohex2( ret ); co( ' ');
            if ( ret == 0 )     // wait idle
                continue;
            else
                break;
        }
    }

    //cohex4( i ); crlf();
    card_csoff();
    return ret & 0x03;
}

//-----
// read from SD card to rwbuffer  512bytes block
unsigned char sd_read( long address )
{
	unsigned char    *p;
	unsigned char    ret;
    int     i;

    card_cson();

    spi_tx( 0x40 + 17 );        // CMD17  read single block
    spi_tx( address >> 24 );
    spi_tx( address >> 16 );
    spi_tx( address >>  8 );
    spi_tx( address  );
    spi_tx( 0xff );

    ret = 0x00;
    if ( spi_getresponce(10, 0xff) == 0 ) {
       for ( i = 0; i < 100; i++ ) {
            if ( spi_tx( 0xff ) == 0xfe ) { // is block start code
                ret = 0x01;
                break;
            }
        }
    } else {
    }

    if ( ret == 0x01 ) {
        for ( p = rwbuffer, i = 0; i < 512; i++ ) *p++ = spi_tx( 0xff );
    }

    spi_tx( 0xff );     // read out CRC 2bytes
    spi_tx( 0xff );

    card_csoff();

    return ret;
}
