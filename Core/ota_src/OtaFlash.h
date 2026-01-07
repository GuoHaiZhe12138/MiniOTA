/**
 ******************************************************************************
 * @file    OtaFlash.h
 * @author  MiniOTA Team
 * @brief   Flash 驱动层抽象头文件
 *          定义 Flash 句柄结构及页缓冲管理接口
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */

#ifndef OTAFLASH_H
#define OTAFLASH_H

#include "OtaInterface.h"

/** @defgroup OTA_Flash_Handle
 * @{
 */
/**
 * @brief Flash 操作句柄结构体
 */
typedef struct {
    uint32_t curr_addr;          /**< 当前操作的 Flash 地址 */
    uint16_t page_offset;        /**< 当前页内的偏移量 */
    uint8_t  page_buf[OTA_FLASH_PAGE_SIZE];  /**< 页缓冲区，大小为 Flash 页大小 */
} flashHandle_t;
/**
 * @}
 */

/**
 * @brief  获取当前 Flash 操作地址
 * @return 当前地址
 */
uint32_t Flash_GetCurAddr(void);

/**
 * @brief  获取当前页内偏移
 * @return 页内偏移
 */
uint16_t Flash_GetPageOffset(void);

/**
 * @brief  获取页缓冲区的指针
 * @return 页缓冲区指针
 */
uint8_t *Flash_GetMirr(void);

/**
 * @brief  设置当前 Flash 操作地址
 * @param  ch: 目标地址
 */
void Flash_SetCurAddr(uint32_t ch);

/**
 * @brief  设置当前页内偏移
 * @param  ch: 目标偏移
 */
void Flash_SetPageOffset(uint16_t ch);

/**
 * @brief  将数据复制到页缓冲区
 * @param  mirr: 源数据指针
 * @param  length: 复制长度
 */
void Flash_SetMirr(const uint8_t *mirr, uint16_t length);

/**
 * @brief  初始化 Flash 句柄，并预读当前页内容
 * @param  addr: 起始地址
 */
void FlashHandle_Init(uint32_t addr);

/**
 * @brief  将页缓冲区写入 Flash，包含擦除、编程、校验全流程
 * @return 0: 成功, 1: 失败
 */
int Flash_Write(void);

#endif
