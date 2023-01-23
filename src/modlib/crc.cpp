#include "crc.h"
#include <stdint.h>

uint8_t get_crc_byte( const uint8_t buf[], int len)
{
    unsigned char crc = 0xFF;
    unsigned char pos = 0, i;
    for (pos = 0; pos < len; pos++)
    {
        crc ^= (unsigned char)buf[pos]; // XOR byte into least sig. byte of crc
        for (i = 8; i != 0; i--)
        { // Loop over each bit
            if ((crc & 0x01) != 0)
            {              // If the LSB is set
                crc >>= 1; // Shift right and XOR 0xA001
                crc ^= CRC_BYTE_POLY;
            }
            else           // Else LSB is not set
                crc >>= 1; // Just shift right
        }
    }
    return crc;
}
uint16_t  get_crc_word( const uint8_t buf[], int len)
{
    unsigned int  crc = 0xFFFF;
    unsigned int pos = 0, i;
    for (pos = 0; pos < len; pos++)
    {
        crc ^= (unsigned int )buf[pos]; // XOR byte into least sig. byte of crc
        for (i = 8; i != 0; i--)
        { // Loop over each bit
            if ((crc & 0x0001) != 0)
            {              // If the LSB is set
                crc >>= 1; // Shift right and XOR 0xA001
                crc ^= CRC_WORD_POLY;
            }
            else           // Else LSB is not set
                crc >>= 1; // Just shift right
        }
    }
    return crc;
}
