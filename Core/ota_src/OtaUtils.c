/**
 ******************************************************************************
 * @file    OtaUtils.c
 * @author  MiniOTA Team
 * @brief   工具函数集
 *          提供内存操作、CRC计算、十六进制打印等基础功能
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */

#include "stdint.h"
#include "OtaPort.h"
#include "OtaUtils.h"
#include "OtaInterface.h"

/**
 * @brief  uint8_t 数组复制
 * @param  dst: 目标缓冲区
 * @param  src: 源缓冲区
 * @param  len: 复制长度
 */
void U8ArryCopy(uint8_t *dst, const uint8_t *src, uint32_t len)
{
    if (dst == 0 || src == 0) return;

    while (len--)
    {
        *dst++ = *src++;
    }
}

/**
 * @brief  XMODEM CRC16 计算
 * @param  buf: 数据缓冲区
 * @param  len: 数据长度
 * @return CRC16 校验值
 */
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

/**
 * @brief  内存设置（填充）函数
 * @param  dst: 目标缓冲区
 * @param  val: 填充值
 * @param  len: 填充长度
 */
void OTA_MemSet(uint8_t *dst, uint8_t val, uint32_t len)
{
    while (len--)
    {
        *dst++ = val;
    }
}

/**
 * @brief  内存复制函数
 * @param  dst: 目标缓冲区
 * @param  src: 源缓冲区
 * @param  len: 复制长度
 */
void OTA_MemCopy(uint8_t *dst, const uint8_t *src, uint32_t len)
{
    while (len--)
    {
        *dst++ = *src++;
    }
}

/**
 * @brief  打印32位十六进制数值到调试串口
 * @param  value: 待打印的32位数值
 */
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
