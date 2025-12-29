

#ifndef OTAUTILS_H
#define OTAUTILS_H



void U8ArryCopy(uint8_t *dst, const uint8_t *src, uint32_t len);
uint16_t XmodemCrc16(const uint8_t *buf, uint32_t len);


#endif
