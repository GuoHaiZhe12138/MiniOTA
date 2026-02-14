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

/* =====================================================================
 *  CMSIS 设备头文件配置
 * =====================================================================
 *  本框架只依赖 ARM CMSIS-Core，不直接依赖任何厂商 HAL。
 *  你【必须】在此处包含且仅包含一个 CMSIS 设备头文件，
 *  该头文件通常由芯片厂商提供，用于描述具体 MCU 的内核配置。
 *
 *  示例（根据所用芯片选择其一）：
 *
 *      #include "stm32f103xb.h"   // STM32F1，Cortex-M3
 *      #include "stm32f407xx.h"   // STM32F4，Cortex-M4
 *      #include "gd32f30x.h"      // GD32，Cortex-M3
 *      #include "sam3x8e.h"       // Microchip SAM3，Cortex-M3
 *  注意：
 *    - ❌ 不要在此处直接包含 core_cmX.h
 *    - ❌ 不要包含 HAL / 外设驱动头文件 */
#include "stm32f10x.h"

/* ---------------------------------------------------------------------
 *  以下宏必须在 #include "stm32f103.h" 之前定义，供布局模板使用
 *  - stm32f103.h 内会使用 OTA_FLASH_START_ADDRESS、OTA_FLASH_SIZE
 *  - 其间接包含的 OtaUtils.h 会使用 OTA_FLASH_PAGE_SIZE、OTA_META_SIZE 等
 * ---------------------------------------------------------------------
 */
/* Flash 总大小 */
#define OTA_FLASH_SIZE            0x8000

/* Flash 物理起始地址 */
#define OTA_FLASH_START_ADDRESS   0x08000000UL

/* Flash 页大小 (Cortex-M3 常用 1024 或 2048) */
#define OTA_FLASH_PAGE_SIZE       1024

/* F1 设备使用均匀页 Flash，直接采用自动模式 */
#define OTA_FLASH_MODE            OTA_FLASH_MODE_AUTO
//#define OTA_FLASH_MODE            OTA_FLASH_MODE_MANUAL

/* (模板示例, 请参考f411例程) F411：Meta 独占 Sector1(16K)，APP 从 Sector2 开始；缓冲扇区为 Sector7(128K) */
#if (OTA_FLASH_MODE == OTA_FLASH_MODE_MANUAL)
#define OTA_META_SIZE             (16U * 1024U)
#define OTA_APP_REGION_SIZE       (352U * 1024U)
#endif

/* (模板示例, 请参考f411例程) F411：最后一个 128K 扇区作为缓冲区（Sector7: 0x08060000） */
#if (OTA_FLASH_MODE == OTA_FLASH_MODE_MANUAL)
#define OTA_BUFFER_SECTOR_START   0x08060000UL  //(16*4+64+128*2)*1024=60000(HEX)
#define OTA_BUFFER_SECTOR_SIZE   (128U * 1024U)
#endif

/* 分配给 MiniOTA (Meta + APP_A + APP_B) 的起始地址 
 * F1 按页擦除 */
#define OTA_TOTAL_START_ADDRESS   0x08003000UL

/* 引入 F103 Flash 布局模板 */
#include "stm32f103.h"

/* MiniOTA 管理区域的最大大小(字节) */
#define OTA_APP_MAX_SIZE          (OTA_FLASH_SIZE - (OTA_TOTAL_START_ADDRESS - OTA_FLASH_START_ADDRESS))
/**
 * @}
 */

/**
 * @brief  OTA 检查与运行主逻辑
 *         通常在 main 函数开始处调用，用于检查升级状态并决定跳转或进入 IAP
 */
void OTA_Run(void);

/**
 * @brief  串口中断/数据接收回调
 * @param  byte: 接收到的数据字节
 */
void OTA_ReceiveTask(uint8_t byte);

#endif
