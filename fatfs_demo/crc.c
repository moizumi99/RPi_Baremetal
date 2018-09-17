#include <stdint.h>
#include "crc.h"
#include "mylib.h"

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
    return crc;
}

uint8_t crc7_update(uint8_t crc, uint8_t new_bit)
{
    new_bit ^= ((crc >> 6) & 1);
    crc <<= 1;
    if (new_bit) {
        crc ^= 0x89;
    }
    return crc;
}


uint16_t crc16_update(uint16_t crc, uint8_t new_bit)
{
    new_bit ^= ((crc >> 15) & 1);
    crc <<= 1;
    if (new_bit) {
        crc ^= 0x1021;
    }
    return crc;
}

uint16_t crc16(const uint8_t *buff, int32_t len) {
    uint32_t crc = 0;

    while( len-- ) {
        uint8_t byte = *buff++;
        int32_t bits = 8;

        while( bits-- ) {
            crc <<= 1;
            if( ((crc >> 16) ^ (byte >> 7)) & 1 ) {
                crc ^= 0x11021;;
            }
            byte <<= 1;
        }
    }
    return crc & 0xffff;
}

void add_crc(uint8_t *cmd_array)
{
    uint8_t crc = (crc7(cmd_array, 5) << 1) & 0x0ff;
    crc = crc | 1;
    cmd_array[5] = crc;
}

int32_t check_crc7(uint8_t *resp, int length_of_resp) {
    uint8_t expected_crc = crc7(resp, length_of_resp - 1);
    uint8_t actual_crc = ((resp[length_of_resp - 1] >> 1) & 0xff);
    if ( actual_crc != expected_crc ) {
        LOG("CRC7 response mismatch \n");
        LOG("Expected CRC: %x, Actual CRC: %x\n", expected_crc, actual_crc);
        return -1;
    }
    return 0;
}

int32_t check_crc16(uint8_t *resp, int length_of_resp, uint16_t received_crc) {
    uint16_t expected_crc = crc16(resp, length_of_resp);
    if (received_crc != expected_crc ) {
        LOG("CRC16 response mismatch \n");
        LOG("Expected CRC: %x, Actual CRC: %x\n", expected_crc, received_crc);
        return -1;
    }
    return 0;
}

