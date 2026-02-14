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

#if (OTA_FLASH_MODE == OTA_FLASH_MODE_MANUAL)
/*
 * stm32f4xx_flash.h 里的 FLASH_Sector_0～FLASH_Sector_7 是「寄存器编码」：
 *   FLASH_Sector_0=0x0000, Sector_1=0x0008, ..., Sector_7=0x0038
 * 这些值直接写入 FLASH->CR 的 SNB 位域，用于选择“第几个扇区”，并不表示扇区大小。
 *
 * 物理布局（地址与大小）在 .h 里没有，需查芯片参考手册（如 Table 4. Flash module organization）。
 * STM32F411xC/E 主存 512KB 的对应关系为：
 *   扇区索引    起始地址        大小
 *   Sector 0   0x08000000      16 KB
 *   Sector 1   0x08004000      16 KB
 *   Sector 2   0x08008000      16 KB
 *   Sector 3   0x0800C000      16 KB
 *   Sector 4   0x08010000      64 KB
 *   Sector 5   0x08020000     128 KB
 *   Sector 6   0x08040000     128 KB
 *   Sector 7   0x08060000     128 KB
 * 下面 OTA_F411_AddrToSector 按该布局实现「地址 → 扇区索引」；索引再通过
 * OTA_F411_SectorIndexToReg 转成 FLASH_Sector_x 传给 FLASH_EraseSector。
 */
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
#endif

/**
 * @brief  判断是否应进入 IAP 模式
 * @return 1: GPIOB Pin11 为高电平，进入 IAP; 0: 正常启动
 */
uint8_t OTA_ShouldEnterIap(void)
{
	return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
}

/**
 * @brief  外设逆初始化（跳转前清理）
 *         根据具体应用需要，可在此关闭所有外设时钟、中断等
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

    /* 禁止擦除 Sector0（0x08000000~0x08003FFF），该区应仅存放 BootLoader */
    if (sec_idx == 0U)
    {
        return 1;
    }

    return (FLASH_EraseSector(sec_reg, VoltageRange_3) == FLASH_COMPLETE) ? 0 : 1;
}

/**
 * @brief  写入半字数据到 Flash
 * @param  addr: 目标地址
 * @param  data: 16位数据
 * @return 0: 成功, 1: 失败
 */
int OTA_DrvProgramHalfword(uint32_t addr, uint16_t data)
{
    return (FLASH_ProgramHalfWord(addr, data) == FLASH_COMPLETE) ? 0 : 1;
}

/**
 * @brief  从 Flash 读取数据到缓冲区
 * @param  addr: 源地址
 * @param  buf: 目标缓冲区
 * @param  len: 读取长度
 */
void OTA_DrvRead(uint32_t addr, uint8_t *buf, uint16_t len)
{
	OTA_MemCopy(buf,(uint8_t *)addr, len);
}

/**
 * @brief  发送一个字节到 USART1（用于 Xmodem 协议应答）
 * @param  byte: 待发送字节
 * @return 0: 发送成功（硬件发送完成即返回）
 */
uint8_t OTA_SendByte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, byte);
    return 0; 
}

/**
 * @brief  发送调试信息到 USART2
 * @param  data: 字符串指针
 * @return 1: 成功, 0: 参数无效
 */
uint8_t OTA_DebugSend(const char *data)
{
    if (!data) return 0;
    while (*data)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, *data++);
    }

    return 1;
}

/**
 * @brief  毫秒级延时函数（基于空操作循环）
 *         注意：此延时精度依赖于 CPU 频率
 */
void OTA_Delay1ms(void)
{
    volatile uint32_t count = 16000U;   //f4默认主频为16MHz
    while (count--)
    {
        __NOP();
    }
}
