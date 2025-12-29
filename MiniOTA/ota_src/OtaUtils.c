

#include "stdint.h"
#include "OtaPort.h"
#include "OtaUtils.h"
#include "OtaInterface.h"

// uint8Êý×é¸´ÖÆ
void U8ArryCopy(uint8_t *dst, const uint8_t *src, uint32_t len)
{
    if (dst == 0 || src == 0) return;

    while (len--)
    {
        *dst++ = *src++;
    }
}

// CRC16/XMODEM (poly=0x1021, init=0)
uint16_t XmodemCrc16(const uint8_t *buf, uint32_t len)
{
    uint16_t crc = 0;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)buf[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}



