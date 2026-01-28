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

/*
 * =====================================================================
 *  CMSIS 设备头文件配置
 * =====================================================================
 *  本框架只依赖 ARM CMSIS-Core，不直接依赖任何厂商 HAL。
 *  用户【必须】在此处包含且仅包含一个 CMSIS 设备头文件，
 *  该头文件通常由芯片厂商提供，用于描述具体 MCU 的内核配置。
 *
 *  示例（根据所用芯片选择其一）：
 *
 *      #include "stm32f103xb.h"   // STM32F1，Cortex-M3
 *      #include "stm32f407xx.h"   // STM32F4，Cortex-M4
 *      #include "gd32f30x.h"      // GD32，Cortex-M3
 *      #include "sam3x8e.h"       // Microchip SAM3，Cortex-M3
 *  要求：
 *    - 该头文件必须符合 CMSIS 规范
 *    - 内部应正确包含 core_cmX.h（X = 3 / 4 / 7 等）
 *  注意：
 *    - ❌ 不要在此处直接包含 core_cmX.h
 *    - ❌ 不要包含 HAL / 外设驱动头文件 */
#include "stm32f10x.h"

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
 * @brief  串口中断/数据接收回调
 * @param  byte: 接收到的数据字节
 */
void OTA_ReceiveTask(uint8_t byte);

#endif
