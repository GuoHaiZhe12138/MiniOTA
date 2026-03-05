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
#include "stm32f4xx.h"

/* ---------------------------------------------------------------------
 *  以下宏必须在 #include "stm32f411.h" 之前定义：
 *  - stm32f411.h 内会使用 OTA_FLASH_START_ADDRESS、OTA_FLASH_SIZE
 *  - 其间接包含的 OtaUtils.h 会使用 OTA_FLASH_PAGE_SIZE、OTA_META_SIZE 等
 * ---------------------------------------------------------------------
 */
/* Flash 总大小（STM32F411CEU6 主存 512KB） */
#define OTA_FLASH_SIZE            (512U * 1024U)

/* Flash 物理起始地址 */
#define OTA_FLASH_START_ADDRESS   0x08000000UL

/* Flash 页大小（与 Xmodem 包对齐，如 1024） */
#define OTA_FLASH_PAGE_SIZE       1024

/* F411 使用非均匀扇区，采用非均匀Flash模式 */
//#define OTA_FLASH_FORMAT            0 /* OTA_FLASH_FORMAT_UNIFORM */
#define OTA_FLASH_FORMAT            1 /* OTA_FLASH_FORMAT_NOT_UNIFORM */

/* F411：Meta 独占 Sector1(16K)，APP 从 Sector2 开始；缓冲扇区为 Sector7(128K) */
#if (OTA_FLASH_FORMAT == 1)
#define OTA_META_SIZE             (16U * 1024U)
#define OTA_APP_REGION_SIZE       (352U * 1024U)
#endif

/* F411：最后一个 128K 扇区作为缓冲区（Sector7: 0x08060000） */
#if (OTA_FLASH_FORMAT == 1)
#define OTA_BUFFER_SECTOR_START   0x08060000UL
#define OTA_BUFFER_SECTOR_SIZE   (128U * 1024U)
#endif

/* F411：暂定APP_A大小为224kB */
#if (OTA_FLASH_FORMAT == 1)
#define OTA_APP_SLOT_SIZE         (224U * 1024U)
#endif

/* 分配给 MiniOTA (Meta + APP_A + APP_B) 的起始地址
 * F4 按扇区擦除，须 ≥ 0x08004000，BootLoader 仅占 Sector0(16KB)。 */
#define OTA_TOTAL_START_ADDRESS   0x08004000UL

/* 引入 F411 Flash 布局模板（依赖上面已定义的宏） */
#include "stm32f411.h"

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
