/**
 ******************************************************************************
 * @file    OtaInterface.h
 * @author  MiniOTA Team
 * @brief   MiniOTA 全局配置与入口接口定义
 *          包含 Flash 分区布局定义、App 起始地址及全局 OTA 运行接口
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */

#ifndef OTAINTERFACE_H
#define OTAINTERFACE_H

#include <stdint.h>
#include <stddef.h>

/** @defgroup OTA_Flash_Layout_Settings
 * @{
 */
/* Flash 总大小 */
#define OTA_FLASH_SIZE            0x8000

/* Flash 物理起始地址 */
#define OTA_FLASH_START_ADDRESS   0x08000000UL

/* 分配给 MiniOTA (Meta + APP_A + APP_B) 的起始地址 */
#define OTA_TOTAL_START_ADDRESS   0x08003000UL

/* MiniOTA 管理区域的最大大小(字节) */
#define OTA_APP_MAX_SIZE          (OTA_FLASH_SIZE - (OTA_TOTAL_START_ADDRESS - OTA_FLASH_START_ADDRESS))

/* Flash 页大小 (Cortex-M3 常用 1024 或 2048) */
#define OTA_FLASH_PAGE_SIZE       1024
/**
 * @}
 */

/** @defgroup OTA_Internal_Memory_Map
 * @{
 */
/* 状态区(Meta)大小: 占用一页 */
#define OTA_META_SIZE             OTA_FLASH_PAGE_SIZE

/* 状态区(Meta)起始地址 */
#define OTA_META_ADDR             OTA_TOTAL_START_ADDRESS

/* APP 分区(A+B)的起始地址 */
#define OTA_APP_REGION_ADDR       (OTA_META_ADDR + OTA_META_SIZE)

/* APP 分区(A+B)的总可用空间 */
#define OTA_APP_REGION_SIZE       (OTA_FLASH_SIZE - (OTA_APP_REGION_ADDR - OTA_FLASH_START_ADDRESS))

/* 单个 APP 分区的大小 (对齐到页) */
#define OTA_APP_SLOT_SIZE         ((OTA_APP_REGION_SIZE / 2) / OTA_FLASH_PAGE_SIZE * OTA_FLASH_PAGE_SIZE)

/* APP_A 分区起始地址 */
#define OTA_APP_A_ADDR            OTA_APP_REGION_ADDR

/* APP_B 分区起始地址 */
#define OTA_APP_B_ADDR            (OTA_APP_A_ADDR + OTA_APP_SLOT_SIZE)
/**
 * @}
 */

/**
 * @brief  OTA 检查与运行主逻辑
 *         通常在 main 函数开始处调用，用于检查升级状态并决定跳转或进入 IAP
 */
void OTA_Run(void);

/**
 * @brief  启动 IAP 流程
 *         通过串口/Xmodem 接收新固件
 */
void OTA_StartIAP(void);

#endif
