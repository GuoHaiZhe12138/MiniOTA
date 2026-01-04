

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



void OTA_MemSet(uint8_t *dst, uint8_t val, uint32_t len)
{
    while (len--)
    {
        *dst++ = val;
    }
}

void OTA_MemCopy(uint8_t *dst, const uint8_t *src, uint32_t len)
{
    while (len--)
    {
        *dst++ = *src++;
    }
}

void OTA_PrintHex32(uint32_t value)
{
    char buf[11];          // "0x12345678\0"
    buf[0] = '0';
    buf[1] = 'x';

    for (int i = 0; i < 8; i++)
    {
        uint8_t nibble = (value >> (28 - i * 4)) & 0xF;

        if (nibble < 10)
            buf[2 + i] = '0' + nibble;
        else
            buf[2 + i] = 'A' + (nibble - 10);
    }

    buf[10] = '\0';
    OTA_DebugSend(buf);
}




