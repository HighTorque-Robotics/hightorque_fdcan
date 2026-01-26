#include <stdint.h>



uint8_t crc8_ccitt(unsigned char *pchMessage, unsigned int dwLength, unsigned char ucCRC8);
uint16_t crc16_ccitt(uint16_t crc, uint8_t const *buffer, uint16_t len);

