#include <stdbool.h>
#include "sdcard_lowlevel.h"
#include "sdcard.h"
#include "mylib.h"
#include "helper.h"
#include "crc.h"
#define WAIT_400KHZ 2
// TODO: define the correct value
#define CLK_WAIT_25MHZ 1

PIN_DIRECTION cmd_direction;
PIN_DIRECTION data_direction;

#define START_BIT 0
#define STOP_BIT 7

#define R1_RESP_LENGTH 6
#define R2_RESP_LENGTH 17
#define R3_RESP_LENGTH 6
#define R7_RESP_LENGTH 6

static uint16_t global_card_rca = 0;
static uint16_t global_clk_mode = 0;
static uint16_t high_speed_mode_en = 0;

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

void clk_high_400k() {
    set_pin(SD_CLK);
    waitMicro(WAIT_400KHZ);
}

void clk_low_400k() {
    clear_pin(SD_CLK);
    waitMicro(WAIT_400KHZ);
}

void clk_high_25M() {
    set_pin(SD_CLK);
    wait(CLK_WAIT_25MHZ);
}

void clk_low_25M() {
    clear_pin(SD_CLK);
    wait(CLK_WAIT_25MHZ);
}

void clk_high() {
    if (global_clk_mode) {
        clk_high_25M();
    } else {
        clk_high_400k();
    }
}

void clk_low() {
    if (global_clk_mode) {
        clk_low_25M();
    } else {
        clk_low_400k();
    }
}

void set_clock_mode_fast() {
    global_clk_mode = 1;
}

void set_clock_mode_slow() {
    global_clk_mode = 0;
}

void send_clock_400k(int32_t n) {
    for (int32_t i = 0; i < n; i++) {
        clk_high_400k();
        clk_low_400k();
    }
}

void send_clock_25M(int32_t n) {
    for (int32_t i = 0; i < n; i++) {
        clk_high_25M();
        clk_low_25M();
    }
}

void send_clock(int32_t n) {
    if (global_clk_mode == 0) {
      send_clock_400k(n);
    } else {
      send_clock_25M(n);
    }
}

void set_cmd_direction_output() {
    if (cmd_direction != PIN_OUTPUT) {
        set_pin_direction(SD_CMD, PIN_OUTPUT);
        cmd_direction = PIN_OUTPUT;
    }
}

void set_cmd_direction_input() {
    if (cmd_direction != PIN_INPUT) {
        set_pin_direction(SD_CMD, PIN_INPUT);
        cmd_direction = PIN_INPUT;
    }
}

void set_data_direction_output() {
    if (data_direction != PIN_OUTPUT) {
        set_pin_direction(SD_DA0, PIN_OUTPUT);
        set_pin_direction(SD_DA1, PIN_OUTPUT);
        set_pin_direction(SD_DA2, PIN_OUTPUT);
        set_pin_direction(SD_DA3, PIN_OUTPUT);
        data_direction = PIN_OUTPUT;
    }
}

void set_data_direction_input() {
    if (data_direction != PIN_INPUT) {
        set_pin_direction(SD_DA0, PIN_INPUT);
        set_pin_direction(SD_DA1, PIN_INPUT);
        set_pin_direction(SD_DA2, PIN_INPUT);
        set_pin_direction(SD_DA3, PIN_INPUT);
        data_direction = PIN_INPUT;
    }
}

void set_cmd_1bit(int32_t d) {
    if (d == 0) {
        clear_pin(SD_CMD);
    } else {
        set_pin(SD_CMD);
    }
}

// send command
void send_cmd( uint8_t c) {
    for (int32_t i = 0; i < 8; i++) {
        set_cmd_1bit(c & 0x80);
        clk_high();
        clk_low();
        c <<= 1;
    }
    set_cmd_1bit(1);
}

// get command response
uint8_t get_cmd_1bit() {
    uint8_t ret;
    clk_high();
    ret = get_pin_signal(SD_CMD);
    clk_low();
    return ret;
}

int32_t get_cmd_byte() {
    int32_t ret = 0;
    for (int32_t i=0; i < 8; i++) {
        ret = (ret << 1) | get_cmd_1bit();
    }
    return ret;
}

// get command response and data response
uint8_t get_cmd_and_dat_4bit(uint8_t *dat) {
    uint8_t ret;
    uint8_t dat_read;
    clk_high_25M();
    ret = get_pin_signal(SD_CMD);
    dat_read  = get_pin_signal(SD_DA3) << 3;
    dat_read |= get_pin_signal(SD_DA2) << 2;
    dat_read |= get_pin_signal(SD_DA1) << 1;
    dat_read |= get_pin_signal(SD_DA0);
    clk_low_25M();
    *dat = dat_read;
    return ret;
}

// get dat0 1 bit response
uint8_t get_dat0_1bit() {
    uint8_t ret;
    clk_high();
    ret = get_pin_signal(SD_DA0);
    clk_low();
    return ret;
}

// get 8 cycle response from dat0 line
int32_t get_dat0_response() {
    int32_t ret = 0;
    for (int32_t i=0; i < 8; i++) {
        ret = (ret << 1) | get_dat0_1bit();
    }
    return ret;
}

int32_t get_cmd_response(uint32_t length, uint8_t *resp)
{
    set_cmd_direction_input();
    int32_t timeout = (global_clk_mode == 0) ? 25000 / 10 : 25000 / 10;
    int32_t ret = 1;
    while ((ret = get_cmd_1bit()) != 0) {
        if (--timeout == 0) {
            goto ERROR;
        }
    }
    for (int32_t i=0; i < 7; i++) {
        ret = (ret << 1) | get_cmd_1bit();
    }
    resp[0] = ret & 0xFF;
    for (int32_t i = 1; i < length; i++) {
        resp[i] = get_cmd_byte();
    }
    return 0;
 ERROR:
    LOG("CMD response timeout\n");
    return -1;
    
}

int32_t wait_for_cmd_rdy() {
    int32_t timeout = (global_clk_mode == 0) ? 4096 : 4096 * 256;
    uint8_t ret = 0;
    set_cmd_direction_input();
    while(ret != 0xFF & timeout-->0) {
        ret = get_cmd_byte();
    }
    if (ret != 0xFF) {
        LOG("Time out. The CMD LINE is busy\n");
        return -1;
    }
    return 0;
}

void send_cmd_array(uint8_t *cmd_array)
{
    LOG("Start sending cmd string: ");
    LOG("%2x %2x %2x %2x %2x %2x\n", cmd_array[0], cmd_array[1], cmd_array[2],
           cmd_array[3], cmd_array[4], cmd_array[5]);
    set_cmd_direction_output();
    send_cmd(cmd_array[0]);
    send_cmd(cmd_array[1]);
    send_cmd(cmd_array[2]);
    send_cmd(cmd_array[3]);
    send_cmd(cmd_array[4]);
    send_cmd(cmd_array[5]);
    set_cmd_direction_input();
}

int32_t wait_for_dat_rdy() {
    int32_t timeout = (global_clk_mode == 0) ? 4096 : 4096 * 256;
    uint8_t ret = 0;
    while(ret != 0xFF & timeout-->0) {
        ret = get_dat0_response();
    }
    if (ret != 0xFF) {
        LOG("Time out. The DAT0 LINE is busy\n");
        return -1;
    }
    return 0;
}

int32_t check_R1_error(uint8_t *resp)
{
    uint32_t error;
    // SD CARD status bits
    // (X) are the ones checked here
    // 31 : OUT_OF_RANGE (X)
    // 30 : ADDRESS_ERROR (X)
    // 29 : BLOCK_LEN_ERROR (X)
    // 28 : ERASE_SEQ_ERROR (X)
    // 27 : ERASE_PARAM (X)
    // 26 : WP_VIOLATION (X)
    // 25 : CARD_IS_LOCKED
    // 24 : LOCK_UNLOCK_FAILED (X)
    // 23 : COM_CRC_ERROR (X)
    // 22 : ILLEGAL_COMMAND (X)
    // 21 : CARD_ECC_FAILED (X)
    // 20 : CC_ERROR (X)
    // 19 : ERROR (X)
    // 18 - 17 : reserved
    // 16 : CSD_OVERWRITE (X)
    // 15 : WP_ERASE_SKIP
    // 14 : CARD_ECC_DISABLED
    // 13 : ERASE_RESET
    // 12 - 9: CURRENT_STATE
    // 8  : READY_FOR_DATA
    // 7  : reserved
    // 6  : FX_EVENT
    // 5  : APP_CMD
    // 4  : rserved
    // 3  : AKE_SEQ_ERROR (X)
    // 2 - 0 : reserved
    error = (resp[1] & 0b11111101) << 24;
    error |= (resp[2] & 0b11111001) << 16;
    // error |= (resp[3] & 0b00000000);
    error |= (resp[4] & 0b00001000);
    return error;
}

void dump_R1_status(uint32_t error)
{
    char *error_messages[] = {
        "OUT_OF_RANGE",
        "ADDRESS_ERROR",
        "BLOCK_LEN_ERROR",
        "ERASE_SEQ_ERROR",
        "ERASE_PARAM",
        "WP_VIOLATION",
        "CARD_IS_LOCKED",
        "LOCK_UNLOCK_FAILED",
        "COM_CRC_ERROR",
        "ILLEGAL_COMMAND",
        "CARD_ECC_FAILED",
        "CC_ERROR",
        "ERROR",
        "reserved", "reserved",
        "CSD_OVERWRITE",
        "WP_ERASE_SKIP",
        "CARD_ECC_DISABLED", 
        "ERASE_RESET",
        "CURRENT_STATE3", "CURRENT_STATE2", "CURRENT_STATE1", "CURRENT_STATE0",
        "READY_FOR_DATA", "reserved",
        "FX_EVENT",
        "APP_CMD",
        "reserved",
        "AKE_SEQ_ERROR", "reserved", "reserved", "reserved"};
    for (int i = 0; i < 32; i++) {
        LOG("%s: %d\n", error_messages[31-i], error & 1);
        error >>= 1;
    }
    
}

int32_t check_R1_response(uint8_t *resp, uint8_t cmd)
{
    LOG("[47-40]: CMD Index          : %02x\n", resp[0]);
    LOG("[39-32]: Card status [31-24]: %02x\n", resp[1]);
    LOG("[31-24]: Card status [23-16]: %02x\n", resp[2]);
    LOG("[23-16]: Card status [15- 8]: %02x\n", resp[3]);
    LOG("[15- 8]: Card status [7 - 0]: %02x\n", resp[4]);
    LOG("[ 7- 0]: CRC + end bit: %02x\n", resp[5]);
    int ret = 0;
    if (resp[0] != cmd) {
        LOG("CMD Index error\n");
        ret = -1;
    }
    uint32_t crc_error = check_crc7(resp, 6);
    if (crc_error) {
        LOG("CRC error.\n");
        ret = -1;
    }
    uint32_t error = check_R1_error(resp);
    if (error != 0) {
        LOG("Card response error\n");
        dump_R1_status(error);
        ret = -1;
    }
    if ((resp[5] & 1) != 1) {
        LOG("End bit missing. resp[5]: %x\n", resp[5]);
        ret = -1;
    }
    return ret;
}

int send_cmd_with_crc(uint8_t cmd_array[]) {
    add_crc(cmd_array);
    if (wait_for_cmd_rdy() < 0) {
        LOG("CMD ready Timeout\n");
        return -1;
    }
    send_cmd_array(cmd_array);
    return 0;
}

int32_t cmd0() {
    uint8_t cmd_array[] = {0x40, 0, 0, 0, 0, 0xff};
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    send_clock(8);
    return 0;
}

int32_t cmd8(uint8_t vhs,
                   uint8_t checkpattern, uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 8, 0, 0, vhs, checkpattern, 0xff};
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    if (get_cmd_response(R7_RESP_LENGTH, resp)) {
        LOG("CMD response Timeout\n");
        return -1;
    }
    send_clock(8);

    // resp[] check
    LOG("[47-40]: CMD Index (08 expected): %02x\n", resp[0]);
    LOG("[39-32]: RESERVED (00 expected): %02x\n", resp[1]);
    LOG("[31-24]: RESERVED (00 expected): %02x\n", resp[2]);
    LOG("[23-16]: Voltage Accepted (01 expected): %02x\n", resp[3]);
    LOG("[15- 8]: Check Pattern (aa expected): %02x\n", resp[4]);
    LOG("[ 7- 0]: CRC + end bit: %02x\n", resp[5]);
    // check for illegal command or non-compatible card
    if (resp[0] != 8 || resp[3] != 1 || resp[4] != 0xaa) {
        LOG("Response error\n");
        return -1;
    }
    if (check_crc7(resp, 6)) {
        LOG("CRC error\n");
        return -1;
    }
    return 0;
}

int get_and_check_R1_response(int cmd, uint8_t *resp) {
    if ( get_cmd_response(R1_RESP_LENGTH, resp)) {
        return -1;
    }
    send_clock(8);
    if (check_R1_response(resp, cmd) < 0) {
        LOG("R1 response error\n");
        return -1;
    }
    return 0;
}

int send_cmd_and_check_R1_response(uint8_t *cmd_array, int cmd, uint8_t *resp) {
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    if (get_and_check_R1_response(cmd, resp)) {
        return -1;
    }
    return 0;
}

int32_t cmd13(uint8_t *resp, uint8_t task)
{
    uint8_t cmd_array[] = {0x40 | 13, 0, 0, 0, 0, 0xff};
    cmd_array[1] = (global_card_rca >> 8) & 0xff;
    cmd_array[2] = global_card_rca & 0xff;
    cmd_array[3] = (task) ? 0x80 : 0;
    if (send_cmd_and_check_R1_response(cmd_array, 13, resp)) {
        return -1;
    }
    send_clock(512);
    return 0;
}

int32_t cmd55(uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 55, 0, 0, 0, 0, 0xff};
    cmd_array[1] = (global_card_rca >> 8) & 0xff;
    cmd_array[2] = global_card_rca & 0xff;
    return (send_cmd_and_check_R1_response(cmd_array, 55, resp));
 }

int32_t acmd41(uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 41, 0, 0, 0, 0, 0xff};
    cmd_array[1] = (1 << 6); // HCS=1, XPC=0, S18R=0
    cmd_array[2] = 0x30; // OCR: 3.3-3.4, 3.3-3.2
    cmd_array[3] = 0;    // OCR
    cmd_array[4] = 0;    // OCR
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    if (get_cmd_response(R3_RESP_LENGTH, resp)) {
        LOG("Response timeout\n");
        return -1;
    }
    send_clock(8);
    return 0;
}

int32_t check_acmd41_busy(uint8_t *resp)
{
    LOG("[47-40]: CMD Index (0x3F expected): %02x\n", resp[0]);
    LOG("[39-32]: OCR[32-24] (0xc0 expected): %02x\n", resp[1]);
    LOG("[31-24]: OCR[23-16]: %02x\n", resp[2]);
    LOG("[23-16]: OCR[15- 8]: %02x\n", resp[3]);
    LOG("[15- 8]: OCR[7 - 0]: %02x\n", resp[4]);
    LOG("[ 7- 0]: 0xFF expected: %02x\n", resp[5]);
    LOG("Bysy bit: %X\n", (resp[1] & (1 << 7)) >> 7);
    LOG("CCS: %X\n", (resp[1] & (1 << 6)) >> 6);
    LOG("UHS-II: %X\n", (resp[1] & (1 << 5)) >> 5);
    LOG("S18A: %X\n", (resp[1] & 1));
    if ((resp[1] & (1 << 7)) == 0) {
        return -1;
    }
    return 0;
}

int32_t check_cmd2_response(uint8_t *resp)
{
    uint8_t name[6];
    LOG("[135:128]: Reserved (3F expected): %02x\n", resp[0]);
    LOG("[127:120]: Card CID [127:120]: %02x\n", resp[1]);
    LOG("[119:112]: Card CID [119:112]: %02x\n", resp[2]);
    LOG("[111:104]: Card CID [111:104]: %02x\n", resp[3]);
    LOG("[103: 96]: Card CID [103: 96]: %02x\n", resp[4]);
    LOG("[ 97: 88]: Card CID [ 97: 88]: %02x\n", resp[5]);
    LOG("[ 87: 80]: Card CID [ 87: 80]: %02x\n", resp[6]);
    LOG("[ 79: 72]: Card CID [ 79: 72]: %02x\n", resp[7]);
    LOG("[ 71: 64]: Card CID [ 71: 64]: %02x\n", resp[8]);
    LOG("[ 63: 56]: Card CID [ 63: 56]: %02x\n", resp[9]);
    LOG("[ 55: 48]: Card CID [ 55: 48]: %02x\n", resp[10]);
    LOG("[ 47: 40]: Card CID [ 47: 40]: %02x\n", resp[11]);
    LOG("[ 39: 32]: Card CID [ 39: 32]: %02x\n", resp[12]);
    LOG("[ 31: 24]: Card CID [ 31: 24]: %02x\n", resp[13]);
    LOG("[ 23: 16]: Card CID [ 23: 16]: %02x\n", resp[14]);
    LOG("[ 15:  8]: Card CID [ 15:  8]: %02x\n", resp[15]);
    LOG("[  7:  0]: CRC + end bit: %02x\n", resp[16]);

    LOG("Manufacturer ID (MID): %d\n", resp[1]);
    LOG("OED/Applicaiton ID: %d\n", (resp[2] << 8) + resp[3]);
    name[0] = resp[4];
    name[1] = resp[5];
    name[2] = resp[6];
    name[3] = resp[7];
    name[4] = resp[8];
    name[5] = 0;
    LOG("Product name: %s\n", name);
    LOG("Product revision: %d\n", resp[9] & 0x0ff );;
    LOG("Product Serial Number %d\n", (resp[10] << 24) + (resp[11] << 16)
           + (resp[12] << 8) + (resp[13]));
    LOG("Manufacturing Data: %d\n", ((resp[14] & 0x0F) << 8) + resp[15]);
    LOG("CRC7 check sum: %d\n", resp[16] >> 1);
    if (resp[0] != 0x3F) {
        LOG("Reserved bit mismatch\n");
        return -1;
    }
    // with R2 response, the first byte is ignored for crc7
    if (check_crc7(resp + 1, 16)) {
        return -1;
    }
    return 0;
}

int32_t cmd2(uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 2, 0, 0, 0, 0, 0xff};
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    if (get_cmd_response(R2_RESP_LENGTH, resp)) {
        return -1;
    }
    send_clock(8);
    if (check_cmd2_response(resp)) {
        return -1;
    }
    return 0;
}

int32_t check_cmd3_response(uint8_t *resp)
{
    LOG("[47:40]: CMD Index (03 expected): %02x\n", resp[0]);
    LOG("[39:32]: RCA[31:16]: %02x\n", resp[1]);
    LOG("[31:24]: RCA[15: 0]: %02x\n", resp[2]);
    LOG("[23:16]: Status[15: 8]: %02x\n", resp[3]);
    LOG("[15: 8]: Status[ 7: 0]: %02x\n", resp[4]);
    LOG("[ 7: 0]: CRC + end bit: %02x\n", resp[5]);

    LOG("New RCA: %04x\n", (resp[1] << 8) + resp[2]);
    LOG("Status: %04x\n", (resp[3] << 8) + resp[4]);
    // resp[3] is ((x) is expectation)
    // 7: COM_CRC_ERROR (0)
    // 6: ILLEGAL_COMMAND (0)
    // 5: ERROR (0)
    // 4: CURRENT_STATE[3] (0)
    // 3: CURRENT_STATE[2] (0)
    // 2: CURRENT_STATE[1] (1)
    // 1: CURRENT_STATE[0] (0)
    // 0: READY_FOR_DATA (1)
    // resp[4] is
    // 7: reserved (0)
    // 6: FX_EVENT (0)
    // 5: APP_CMD (NA)
    // 4: reserved (0)
    // 3: AKE_SEQ_ERROR (0)
    // 2-0: reserved (000)
    //
    // CURRENT_STATE should be TRAN = 4
    if (resp[3] != 0b00000101 || (resp[4] & 0x08) != 0) {
        LOG("CMD3 response error\n");
        return -1;
    }
    if (check_crc7(resp, 6)) {
        LOG("CRC7 error\n");
        return -1;
    }
    sdState = STATE_STBY;
    return 0;
}

int32_t cmd3(uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 3, 0, 0, 0, 0, 0xff};
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    if (get_cmd_response(R1_RESP_LENGTH, resp)) {
        return -1;
    }
    send_clock_400k(8);
    if (check_cmd3_response(resp)) {
        return -1;
    }
    return 0;
}


int32_t cmd7(uint8_t *resp, uint16_t rca)
{
    uint8_t cmd_array[] = {0x40 | 7, 0, 0, 0, 0, 0xff};
    cmd_array[1] = (rca >> 8) & 0xff;
    cmd_array[2] = rca & 0xff;
    /* cmd_array[1] = (global_card_rca >> 8) & 0xff; */
    /* cmd_array[2] = global_card_rca & 0xff; */
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    if (rca == 0) {
        return 0;
    }
    if (get_and_check_R1_response(7, resp)) {
        return -1;
    }
    if (wait_for_dat_rdy() < 0) {
        return -1;
    }
    return 0;
}

int32_t moveToTranState() {
    uint8_t resp[R1_RESP_LENGTH];
    if (sdState == STATE_TRAN) {
        return 0;
    } else if (sdState != STATE_STBY) {
        LOG("Not in STBY. Can't move to TRAN.\n");
        goto ERROR;
    }
    if (wait_for_dat_rdy()) {
        goto ERROR;
    }
    LOG("CMD7 start\n");
    if (cmd7(resp, global_card_rca) < 0) {
        goto ERROR;
    }
    if (((resp[3] >> 1) & 0x0F) != 3) {
        LOG("Card is not in STBY (3) status but in %d\n", ((resp[3] >> 1) & 0x0F));
        LOG("Can't move to TRAN status.\n");
        goto ERROR;
    }
    LOG("Moved to TRAN status\n");
    sdState = STATE_TRAN;
    return 0;
 ERROR:
    return -1;
}

int32_t moveToStbyState() {
    uint8_t resp[R1_RESP_LENGTH];
    if (sdState == STATE_STBY) {
        return 0;
    } else if (sdState != STATE_TRAN) {
        LOG("NOT in TRAN. Can't move to STBY.\n");
        goto ERROR;
    }
    if (wait_for_dat_rdy()) {
        goto ERROR;
    }
    LOG("CMD7 start\n");
    if (cmd7(resp, 0) < 0) {
        goto ERROR;
    }
    /* if (((resp[3] >> 1) & 0x0F) != 4) { */
    /*     LOG("Card is not in TRAN (4) status but in %d\n", ((resp[3] >> 1) & 0x0F)); */
    /*     LOG("Can't move to STBY status.\n"); */
    /*     goto ERROR; */
    /* } */
    LOG("Moved to STBY status.\n");
    sdState = STATE_STBY;
    return 0;
 ERROR:
    return -1;
}

int32_t check_cmd9_response(uint8_t *resp)
{
    bool error = false;
    LOG("CSD[125:128]: %02x\n", resp[0]);
    LOG("CSD[127:120]: %02x\n", resp[1]);
    LOG("CSD[119:112]: %02x\n", resp[2]);
    LOG("CSD[111:104]: %02x\n", resp[3]);
    LOG("CSD[103: 96]: %02x\n", resp[4]);
    LOG("CSD[ 95: 88]: %02x\n", resp[5]);
    LOG("CSD[ 87: 80]: %02x\n", resp[6]);
    LOG("CSD[ 79: 72]: %02x\n", resp[7]);
    LOG("CSD[ 71: 64]: %02x\n", resp[8]);
    LOG("CSD[ 63: 56]: %02x\n", resp[9]);
    LOG("CSD[ 55: 48]: %02x\n", resp[10]);
    LOG("CSD[ 47: 40]: %02x\n", resp[11]);
    LOG("CSD[ 39: 32]: %02x\n", resp[12]);
    LOG("CSD[ 31: 24]: %02x\n", resp[13]);
    LOG("CSD[ 23: 16]: %02x\n", resp[14]);
    LOG("CSD[ 15:  8]: %02x\n", resp[15]);
    LOG("CSD[  7:  0]: %02x\n", resp[16]);
    if ((resp[1] >> 6) == 0) {
        LOG("Standard Capacity\n");
    } else if ((resp[1] >> 6) == 1) {
        LOG("High capatcity\n");
    } else {
        LOG("Wrong CSD structure %d\n", resp[1] >> 6);
        error |= true;
    }
    // with R2 response, the first byte is ignored for crc7
    if (check_crc7(resp + 1, R2_RESP_LENGTH - 1)) {
        LOG("CRC7 error\n");
        error |= true;
    }
    return error ? 1 : 0;
}

int32_t cmd9(uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 9, 0, 0, 0, 0, 0xff};
    cmd_array[1] = (global_card_rca >> 8) & 0xff;
    cmd_array[2] = global_card_rca & 0xff;
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    if (get_cmd_response(R2_RESP_LENGTH, resp)) {
        LOG("No response for CMD9. Time out.\n");
        return -1;
    }
    send_clock(8);
    if (check_cmd9_response(resp)) {
        return -1;
    }
    return 0;
}

int32_t acmd6(uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 6, 0, 0, 0, 0, 0xff};
    cmd_array[4] = 0b010;
    return (send_cmd_and_check_R1_response(cmd_array, 6, resp));
}

int32_t cmd23(uint32_t block_count, uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 23, 0, 0, 0, 0, 0xff};
    cmd_array[1] = (block_count >> 24) & 0xff;
    cmd_array[2] = (block_count >> 16) & 0xff;
    cmd_array[3] = (block_count >>  8) & 0xff;
    cmd_array[4] = block_count & 0xff;
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    int32_t ret = get_cmd_response(R1_RESP_LENGTH, resp);
    send_clock(8);
    if (ret < 0) {
        LOG("Block count set fail\n");
    }
    return ret;
}

int32_t read_data(uint8_t *resp, int32_t block_length, uint8_t *rwbuffer)
{
    for (int i = 0; i < 6; i++) {
        resp[i] = 0;
    }
    int8_t response_cnt = -1;
    int32_t data_cnt = -1;
    int32_t crc_cnt = 0;
    uint16_t rcrc0 = 0, rcrc1 = 0, rcrc2 = 0, rcrc3 = 0;
    uint16_t ecrc0 = 0, ecrc1 = 0, ecrc2 = 0, ecrc3 = 0;
    int32_t timeout_limit = 25000000 / 10 * 10;
    // time out is 100 ms. Assuming 25M Hz, timeout cycle is 25000000/10
    // *10 is for just in case
    if (global_clk_mode == 0) {
      timeout_limit = 400000 / 10 * 10;
    // time out is 100 ms. Assuming 400K Hz, timeout cycle is 400000/10
    // *10 is for just in case
    }
    int32_t timeout_cnt = 0;
    while(response_cnt < 48 || data_cnt < block_length * 8 || crc_cnt < 7) {
        uint8_t dat;
        uint8_t ret = get_cmd_and_dat_4bit(&dat);
        if (ret == 0 && response_cnt == -1) {
            response_cnt = 0;
        }
        if (0 <= response_cnt && response_cnt < 48) {
            uint32_t byte_pos =  response_cnt >> 3;
            uint32_t shift_cnt = response_cnt & 0x07;
            resp[byte_pos] |= (ret << (7 - shift_cnt));
            response_cnt++;
        }
        if (0 <= data_cnt && data_cnt < block_length * 8) {
            uint32_t byte_pos = data_cnt >> 3;
            uint32_t shift_cnt = data_cnt & 0x07;
            if (shift_cnt == 0) {
                rwbuffer[byte_pos] = (dat & 0x0F) << 4;
            } else {
                rwbuffer[byte_pos] |= dat & 0x0F;
            }
            data_cnt += 4;
            ecrc3 = crc7_update(ecrc3, (dat >> 3) & 1);
            ecrc2 = crc7_update(ecrc2, (dat >> 2) & 1);
            ecrc1 = crc7_update(ecrc1, (dat >> 1) & 1);
            ecrc0 = crc7_update(ecrc0, dat & 1);
        }
        if (block_length * 8 <= data_cnt && crc_cnt < 7) {
            rcrc3 |= (rcrc3 << 1) | ((dat >> 3) & 1);
            rcrc2 |= (rcrc2 << 1) | ((dat >> 2) & 1);
            rcrc1 |= (rcrc1 << 1) | ((dat >> 1) & 1);
            rcrc0 |= (rcrc0 << 1) | ((dat >> 0) & 1);
            crc_cnt++;
        }
        if ((dat & 0x0F) == 0x00 && data_cnt == -1) {
            data_cnt = 0;
        }
        if (++timeout_cnt > timeout_limit) {
            LOG("Data read timeout\n");
            LOG("response_cnt: %d\n", response_cnt);
            LOG("data_cnt: %d\n", data_cnt);
            break;
        }
    }
    if (rcrc0 != ecrc0 || rcrc1 != ecrc1 || rcrc2 != ecrc2 || rcrc3 != ecrc3) {
        LOG("CRC7 mismatch at DAT 0\n");
        LOG("crc0: recevied %02x, calculated %02x\n", rcrc0, ecrc0);
        LOG("crc1: recevied %02x, calculated %02x\n", rcrc1, ecrc1);
        LOG("crc2: recevied %02x, calculated %02x\n", rcrc2, ecrc2);
        LOG("crc3: recevied %02x, calculated %02x\n", rcrc3, ecrc3);
    }
    uint8_t r0[2] = {0x18, 0x00};
    LOG("0x1800 -> CRC = %02x\n", crc7(r0, 2));
    
    return data_cnt / 8;
}


int32_t read_data_blocks(uint32_t block_counts, uint8_t *resp, uint8_t *rwbuffer)
{
    for (int i = 0; i < 6; i++) {
        resp[i] = 0;
    }
    uint8_t cmd_array[] = {0x40 | 12, 0, 0, 0, 0, 0xff};
    add_crc(cmd_array);
    int32_t block_length = 512; // 
    // time out is 100 ms. Assuming 25MHz, timeout cycle is 25000000/10
    // The clock generated by this program probably won't reach 25MHz
    // *10 at the end is just in case
    int32_t timeout_limit = 25000000 / 10 * 10;
    int8_t response_cnt = 0;
    int32_t data_cnt = 0;
    int32_t buffer_cnt = 0;
    uint32_t block_cnt = 0;
    uint32_t cmd_cnt = 0;
    uint16_t crc0 = 0, crc1 = 0, crc2 = 0, crc3 = 0;
    int8_t cmd_state = 1;
    int8_t dat_state = 0;
    int32_t timeout_cnt = 0;
    uint32_t crc_cnt;
    while(cmd_state != 10 || dat_state != 10) {
        uint8_t dat;
        uint8_t ret = get_cmd_and_dat_4bit(&dat);
        // cmd_sequence
        switch(cmd_state) {
        uint32_t byte_pos, shift_cnt;
        /* case 0: // maybe send cmd18 should be moved here? */
        /*     cmd_state = 1; */
        /*     break; */
        case 1: // wait for response
            if (ret == 0) {
                response_cnt = 1;
                cmd_state = 2;
                timeout_cnt = 0;
            }
            break;
        case 2: // response
            byte_pos =  response_cnt >> 3;
            shift_cnt = response_cnt & 0x07;
            resp[byte_pos] |= (ret << (7 - shift_cnt));
            response_cnt++;
            if (response_cnt == 48) {
                // TODO: Check response
                cmd_state = 10;
            }
            break;
        }
        // data path sate machine
        switch(dat_state) {
        case 0: // wait for data input
            // busy signal appears only on dat0 line (page 7)
            if ((dat & 0x08) == 0) {
                dat_state = 1;
                data_cnt = 0;
                timeout_cnt = 0;
            }
            break;
        case 1: // data read
            if (buffer_cnt & 0x07) {
                rwbuffer[buffer_cnt >> 3] |= (dat & 0x0f);
            } else {
                rwbuffer[buffer_cnt >> 3] = (dat & 0x0f) << 4;
            }
            data_cnt++;
            buffer_cnt += 4;
            if (data_cnt == block_length * 2) {
                crc_cnt = 0;
                dat_state = 2;
            }
            break;
        case 2: // crc read
            crc0 |= (crc0 << 1) | ((dat >> 3) & 1);
            crc1 |= (crc1 << 1) | ((dat >> 2) & 1);
            crc2 |= (crc2 << 1) | ((dat >> 1) & 1);
            crc3 |= (crc3 << 1) | (dat & 1);
            crc_cnt++;
            if (crc_cnt == 16) {
                dat_state = 3;
            }
            break;
        case 3: // check stop bit
            if (dat != 0x0f) {
                LOG("Data stop bit is not right: %x\n", dat);
                goto error;
            }
            block_cnt++;
            if (block_cnt == block_counts) {
                dat_state = 10;
            } else {
                dat_state = 0;
            }
        }
        if (++timeout_cnt > timeout_limit) {
            LOG("SD CARD multiple blocks read Time out\n");
            goto error;
        }
    }
    return (buffer_cnt >> 3);
 error:
    LOG("response_cnt: %d\n", response_cnt);
    LOG("block_cnt: %d\n", block_cnt);
    LOG("data_cnt: %d\n", data_cnt);
    LOG("buffer_cnt: %d\n", buffer_cnt);
    return -1;
}

int32_t cmd12(uint8_t *resp)
{
    uint8_t cmd_array[] = {0x40 | 12, 0, 0, 0, 0, 0xff};
    if (send_cmd_and_check_R1_response(cmd_array, 12, resp)) {
        return -1;
    }
    if (wait_for_dat_rdy() < 0) {
        return -1;
    }
    return 0;
}

void set_address_to_cmd_array(int cmd, uint32_t address, uint8_t *cmd_array) {
    cmd_array[0] = 0x40 | cmd;
    cmd_array[1] = (address >> 24) & 0xFF;
    cmd_array[2] = (address >> 16) & 0xFF;
    cmd_array[3] = (address >> 8) & 0xFF;
    cmd_array[4] = address & 0xFF;
}

int32_t cmd17(uint32_t address, uint8_t *resp, uint8_t *rwbuffer) {
    const int cmd = 17;
    uint8_t cmd_array[6];
    set_address_to_cmd_array(cmd, address, cmd_array);
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    int32_t data_cnt = read_data(resp, 512, rwbuffer);
    if (check_R1_response(resp, cmd) < 0) {
        return -1;
    }
    send_clock(8);
    
    return data_cnt;
}

int32_t cmd18(uint32_t address, uint32_t block_counts, uint8_t *resp, uint8_t *rwbuffer) {
    const int cmd = 18;
    uint8_t cmd_array[6];
    set_address_to_cmd_array(cmd, address, cmd_array);
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    int32_t data_cnt = read_data_blocks(block_counts, resp, rwbuffer);
    if (check_R1_response(resp, cmd) < 0) {
        return -1;
    }
    send_clock(8);
    
    return data_cnt;
}

int32_t cmd6(uint8_t function_group, uint8_t function, uint8_t *resp, uint8_t *rwbuffer)
{
    uint8_t cmd_array[] = {0x40 | 6, 0, 0, 0, 0, 0xff};
    switch(function_group) {
    case 1: cmd_array[4] = function & 0x0F;
        break;
    case 2:cmd_array[4] = (function & 0x0F) << 4;
        break;
    case 3: cmd_array[3] = function & 0x0F;
        break;
    case 4:cmd_array[3] = (function & 0x0F) << 4;
        break;
    default:
        ;
    }
    cmd_array[1] = 0x80; // switch function
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    
    LOG("Read sd card status\n");
    // SD CARD status is 512 bits
    int32_t data_cnt = read_data(resp, 64, rwbuffer);
    if (check_R1_response(resp, 6)) {
        return -1;
    }
    send_clock(8);
    
    return data_cnt;
}


int32_t acmd51(uint8_t *resp, uint8_t *buffer)
{
    uint8_t cmd_array[] = {0x40 | 51, 0, 0, 0, 0, 0xff};
    if (send_cmd_with_crc(cmd_array)) {
        return -1;
    }
    LOG("Read data\n");
    // SCR status is 64 bits
    int32_t data_cnt = read_data(resp, 64 / 8, buffer);
    if (data_cnt != 64 / 8) {
        LOG("Data length mismatch\n");
        return -1;
    }
    if (check_R1_response(resp, 51)) {
        return -1;
    }
    send_clock(8);
    
    return data_cnt;
}

// send data to data bus
void send_data_to_bus(uint8_t dat)
{
    enum PINS dat_pin[] = {SD_DA0, SD_DA1, SD_DA2, SD_DA3};
    for (int pos = 0; pos < 4; pos++) {
        if (dat & 1) {
            set_pin(dat_pin[pos]);
        } else {
            clear_pin(dat_pin[pos]);
        }
        dat >>= 1;
    }
    clk_high();
    clk_low();
}

int32_t write_data(int32_t bytes_in_block, const uint8_t *rwbuffer)
{
    uint16_t crc0 = 0, crc1 = 0, crc2 = 0, crc3 = 0;
    int32_t byte_cnt = 0;
    int32_t timeout_limit = 25000000 / 10;
    // time out is 100 ms. Assuming 25MHz, timeout cycle is 25000000/10
    set_data_direction_output();
    // send start bit
    send_data_to_bus(START_BIT);
    for (int data_cnt = 0; data_cnt < bytes_in_block * 2; data_cnt++) {
        uint8_t dat;
        uint32_t byte_pos = data_cnt >> 1;
        uint32_t shift_cnt = data_cnt & 1;
        if (shift_cnt == 0) {
            dat = rwbuffer[byte_pos] >> 4;
        } else {
            dat = rwbuffer[byte_pos] & 0x0F;
            byte_cnt++;
        }
        crc3 = crc16_update(crc3, (dat >> 3) & 1);
        crc2 = crc16_update(crc2, (dat >> 2) & 1);
        crc1 = crc16_update(crc1, (dat >> 1) & 1);
        crc0 = crc16_update(crc0, dat & 1);
        send_data_to_bus(dat);
    }
    // send crc16 code
    for (int crc_cnt = 0; crc_cnt < 16; crc_cnt++) {
        uint8_t dat = 0;
        int bit3 = (crc3 >> 15) & 1;
        int bit2 = (crc2 >> 15) & 1;
        int bit1 = (crc1 >> 15) & 1;
        int bit0 = (crc0 >> 15) & 1;
        dat = (bit3 << 3) | (bit2 << 2) | (bit1 << 1) | bit0;
        crc0 <<= 1;
        crc1 <<= 1;
        crc2 <<= 1;
        crc3 <<= 1;
        send_data_to_bus(dat);
    }
    // send stop bit at the end of CRC
    send_data_to_bus(STOP_BIT);
    set_data_direction_input();

    uint8_t ret = 0;
    for (int i = 0; i < 6; i++) {
        ret = (ret << 1) | get_dat0_1bit();
    }
    if ((ret & 0x0F) != 0b00101) {
        goto RESPONSE_ERROR;
    }
    LOG("CRC Response Good\n");
    // wait for busy (dat0 == 0) to end
    if (wait_for_dat_rdy() < 0) {
        LOG("Command wait ready time out\n");
        goto WRITE_BUSY_ERROR;
    }
    return byte_cnt;

 RESPONSE_ERROR:
    if ((ret & 0x0F) == 0b01011) {
        LOG("Write CRC response error %x\n", ret);
        return -1;
    } else if ((ret & 0x0F) == 0b01101) {
        LOG("Write rejected error %x\n", ret);
        return -1;
    } 
    LOG("CRC Data response token is unknown %x\n", ret);
    return -1;
 WRITE_BUSY_ERROR:
    LOG("Write busy signal time out error\n");
    return -1;
}

// Single write command
int32_t cmd24(uint32_t address, uint8_t *resp, const uint8_t *rwbuffer)
{
    uint8_t cmd_array[6];
    set_address_to_cmd_array(24, address, cmd_array);
    return send_cmd_and_check_R1_response(cmd_array, 24, resp);
}

// TODO refactor cmd24 and cmd25 (both are almost same)
// WRITE_MULTIPLE_BLOCK command
int32_t cmd25(uint32_t address, uint8_t *resp, const uint8_t *rwbuffer)
{
    uint8_t cmd_array[6];
    set_address_to_cmd_array(25, address, cmd_array);
    return send_cmd_and_check_R1_response(cmd_array, 25, resp);
}

void set_rca(uint8_t *resp)
{
    global_card_rca = ((resp[1] << 8) | resp[2]) & 0xFFFF;
    LOG("New card_rca: %4x\n", global_card_rca);
}

// SD CARD Level control commands

// Initialization of GPIO for SD CARD
void sdInitInterface()
{
    if (sd_interface_init_done) {
        return;
    }
    // GPIO SDC IF setup

    initialize_pins();
    // set up GPIO pins to
    // 47 : SD_CARD_DET -> Read
    // 48 : SD_CLK_R -> Write
    // 49 : SD_CMD_R -> Write
    // 50 : SD_DATA_0 -> Read 
    // 51 : SD_DATA_1 -> Read 
    // 52 : SD_DATA_2 -> Read 
    // 53 : SD_DATA_3 -> Read 
    set_pin_direction(SD_DET, PIN_INPUT);
    set_pin_direction(SD_CLK, PIN_OUTPUT);
    set_pin_direction(SD_CMD, PIN_INPUT);
    set_pin_direction(SD_DA0, PIN_INPUT);
    set_pin_direction(SD_DA1, PIN_INPUT);
    set_pin_direction(SD_DA2, PIN_INPUT);
    set_pin_direction(SD_DA3, PIN_INPUT);
    set_pin_pullup(SD_DET);
    set_pin_pullup(SD_CMD);
    set_pin_pullup(SD_DA0);
    set_pin_pullup(SD_DA1);
    set_pin_pullup(SD_DA2);
    set_pin_pullup(SD_DA3);

    // set initial pin directions
    cmd_direction = PIN_INPUT;
    data_direction = PIN_INPUT;
    
    // wait until the change takes effect
    wait(150);
    sd_interface_init_done = true;
}

// SD card inital.  set to SPI mode.
uint8_t sdInitCard()
{
    uint8_t resp[R2_RESP_LENGTH];
    memset(resp, 0, R2_RESP_LENGTH);
    int32_t ret;
    if (!sd_interface_init_done) {
        LOG("SD Interface intialization not done");
        goto ERROR;
    }
    set_clock_mode_slow();
    // dummy clock (>74) is needed after startup 
    send_clock_400k(80);
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
    set_rca(resp);

    if (moveToTranState()) {
        LOG("Failed to move to Trans state\n");
        goto ERROR;
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
    if (moveToStbyState()) {
        goto ERROR;
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
    global_clk_mode = 1;
    ret = cmd17(address, resp, buffer);
    global_clk_mode = 0;
    return ret;
}

int32_t sdReadMulti( uint32_t address, uint32_t block_count, uint8_t *buffer)
{
    uint8_t resp[R1_RESP_LENGTH];
    int32_t ret;
    global_clk_mode = 1;
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
    global_clk_mode = 0;
    return ret;
}

// Write to SD card from buffer. One 512bytes block.
int32_t sdWrite(uint32_t address, const uint8_t *buffer)
{
    uint8_t resp[R1_RESP_LENGTH];
    uint32_t ret;
    global_clk_mode = 1;
    if (cmd24(address, resp, buffer)) {
        LOG("CMD24 error\n");
        goto ERROR;
    }
    
    uint32_t data_cnt = write_data(512, buffer);
    send_clock(8);
    global_clk_mode = 0;
    return data_cnt;
    
 ERROR:
    if (cmd12(resp) < 0) {
        LOG("CMD12 command error\n");
    }
    send_clock(8);
    global_clk_mode = 0;
    return -1;
}

// Writes TO SD Card multiple blocks.
int32_t sdWriteMulti( uint32_t address, uint32_t num_blocks, const uint8_t *buffer)
{
    uint8_t resp[R1_RESP_LENGTH];
    uint32_t ret;
    global_clk_mode = 1;
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
    global_clk_mode = 0;
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

