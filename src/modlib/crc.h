#ifndef _CRC_H_
#define _CRC_H_
#include <stdint.h>
#define CRC_BYTE_POLY        0xA5  
#define CRC_WORD_POLY        0xAA55
uint8_t get_crc_byte(const uint8_t  buf[], int len);
uint16_t  get_crc_word( const uint8_t buf[], int len);
#endif