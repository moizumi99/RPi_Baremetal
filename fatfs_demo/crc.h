#ifndef CRC_H
#define CRC_H
#include <stdint.h>

uint8_t crc7(const uint8_t *buff, int32_t len );
uint8_t crc7_update(uint8_t crc, uint8_t new_bit);
uint16_t crc16_update(uint16_t crc, uint8_t new_bit);
uint16_t crc16(const uint8_t *buff, int32_t len);
void add_crc(uint8_t *cmd_array);
int32_t check_crc7(uint8_t *resp, int length_of_resp);
int32_t check_crc16(uint8_t *resp, int length_of_resp, uint16_t received_crc);

#endif // CRC_H
