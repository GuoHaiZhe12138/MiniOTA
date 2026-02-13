/**
 ******************************************************************************
 * @file    OtaPort.c
 * @author  MiniOTA Team
 * @brief   硬件平台适配层实现
 *          基于 STM32F4xx 系列 MCU 的特定实现（STM32F411CEU6）
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */
#include "OtaInterface.h"
#include "OtaXmodem.h"
#include "OtaUtils.h"

#include "stm32f4xx_flash.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_rcc.h"

/* STM32F411CEU6 主存 512KB，扇区划分与 FLASH_Sector_x 对应 */
#define OTA_F411_FLASH_BASE        0x08000000UL
#define OTA_F411_SECTOR_SIZE_16K   (16U * 1024U)
#define OTA_F411_SECTOR_SIZE_64K   (64U * 1024U)
#define OTA_F411_SECTOR_SIZE_128K  (128U * 1024U)

/**
 * @brief  根据地址返回 F411 的扇区索引（0~7）
 * @param  addr: Flash 地址
 * @return 扇区索引，若越界返回 0xFF
 */
static uint8_t OTA_F411_AddrToSector(uint32_t addr)
{
    if (addr < OTA_F411_FLASH_BASE)
    {
        return 0xFF;
    }
    addr -= OTA_F411_FLASH_BASE;

    if (addr < 4U * OTA_F411_SECTOR_SIZE_16K)  /* 0 ~ 64KB */
    {
        return (uint8_t)(addr / OTA_F411_SECTOR_SIZE_16K);
    }
    addr -= 4U * OTA_F411_SECTOR_SIZE_16K;

    if (addr < OTA_F411_SECTOR_SIZE_64K)      /* 64KB ~ 128KB, Sector 4 */
    {
        return 4U;
    }
    addr -= OTA_F411_SECTOR_SIZE_64K;

    if (addr < 3U * OTA_F411_SECTOR_SIZE_128K) /* 128KB ~ 512KB, Sector 5~7 */
    {
        return (uint8_t)(5U + addr / OTA_F411_SECTOR_SIZE_128K);
    }

    return 0xFF;
}

/**
 * @brief  将 F411 扇区索引转换为 StdPeriph 的 FLASH_Sector_x 宏值
 */
static uint32_t OTA_F411_SectorIndexToReg(uint8_t index)
{
    static const uint32_t sector_reg[] = {
        FLASH_Sector_0, FLASH_Sector_1, FLASH_Sector_2, FLASH_Sector_3,
        FLASH_Sector_4, FLASH_Sector_5, FLASH_Sector_6, FLASH_Sector_7
    };
    if (index > 7U)
    {
        return 0xFFFFU;
    }
    return sector_reg[index];
}

/**
 * @brief  判断是否应进入 IAP 模式
 * @return 1: 指定 GPIO 为高电平，进入 IAP; 0: 正常启动
 */
uint8_t OTA_ShouldEnterIap(void)
{
    return (uint8_t)GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
}

/**
 * @brief  外设逆初始化（跳转前清理）
 */
void OTA_PeripheralsDeInit(void)
{
    /* 用户可在此添加特定外设清理代码 */
}

/**
 * @brief  串口接收中断回调函数
 */
void OTA_ReceiveTask(uint8_t byte)
{
    OTA_XmodemRevByte(byte);
}

/**
 * @brief  Flash 解锁并清除标志位（F4 使用与 F1 不同的标志位）
 * @return 0: 成功
 */
uint8_t OTA_FlashUnlock(void)
{
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP   | FLASH_FLAG_OPERR  | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR| FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    return 0;
}

/**
 * @brief  Flash 上锁
 * @return 0: 成功
 */
uint8_t OTA_FlashLock(void)
{
    FLASH_Lock();
    return 0;
}

/**
 * @brief  擦除指定地址所在的 Flash 扇区（F4 无 ErasePage，按扇区擦除）
 * @param  addr: 目标地址（所在扇区将被整体擦除）
 * @return 0: 成功, 1: 失败
 */
int OTA_ErasePage(uint32_t addr)
{
    uint8_t  sec_idx = OTA_F411_AddrToSector(addr);
    uint32_t sec_reg = OTA_F411_SectorIndexToReg(sec_idx);

    if (sec_idx > 7U || sec_reg == 0xFFFFU)
    {
        return 1;
    }

    return (FLASH_EraseSector(sec_reg, VoltageRange_3) == FLASH_COMPLETE) ? 0 : 1;
}

/**
 * @brief  写入半字数据到 Flash
 */
int OTA_DrvProgramHalfword(uint32_t addr, uint16_t data)
{
    return (FLASH_ProgramHalfWord(addr, data) == FLASH_COMPLETE) ? 0 : 1;
}

/**
 * @brief  从 Flash 读取数据到缓冲区
 */
void OTA_DrvRead(uint32_t addr, uint8_t *buf, uint16_t len)
{
    OTA_MemCopy(buf, (uint8_t *)addr, len);
}

/**
 * @brief  发送一个字节到 USART1（用于 Xmodem 协议应答）
 */
uint8_t OTA_SendByte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
    {
        ;
    }
    USART_SendData(USART1, byte);
    return 0;
}

/**
 * @brief  发送调试信息到 USART2
 */
uint8_t OTA_DebugSend(const char *data)
{
    if (!data)
    {
        return 0;
    }
    while (*data)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
        {
            ;
        }
        USART_SendData(USART2, (uint16_t)(*data++));
    }
    return 1;
}

/**
 * @brief  毫秒级延时函数（F4 主频较高，计数需按实际时钟调整）
 */
void OTA_Delay1ms(void)
{
    volatile uint32_t count = 16000U;
    while (count--)
    {
        __NOP();
    }
}
