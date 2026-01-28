/**
 ******************************************************************************
 * @file    OtaUtils.h
 * @author  MiniOTA Team
 * @brief   工具函数及公共数据结构定义
 *          包含 OTA 核心数据结构、枚举、宏定义及工具函数声明
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */

#ifndef OTAUTILS_H
#define OTAUTILS_H

#include <stdint.h>

/** @defgroup OTA_Configuration_Constants
 * @{
 */
#define OTA_MAGIC_NUM       0x5A5A0001  /**< Meta 数据有效性识别魔数 */
#define APP_MAGIC_NUM       0x424C4150  /**< "BLAP" - BootLoader APp 固件头魔数 */
#define U32_INVALID         0UL         /**< 32位无效值 */

/**
 * @brief App 分区状态枚举
 */
typedef enum __OTA_SLOT_STATE
{
    SLOT_STATE_EMPTY = 0xFF,    /**< 初始状态/已擦除 */
    SLOT_STATE_UNCONFIRMED = 0, /**< 刚刷完，尚未成功启动过 */
    SLOT_STATE_VALID = 1,       /**< 已经成功启动过，是安全的备份 */
    SLOT_STATE_INVALID = 2      /**< 校验失败或启动过程中异常 */
} OTA_SLOT_STATE_E;

/**
 * @brief Flash 内 App 代码检查结果枚举
 */
typedef enum __OTA_APP_CHECK_RESULT
{
    APP_CHECK_OK = 0,        /**< App 合法 */
    APP_CHECK_SP_INVALID,    /**< 初始堆栈指针无效 */
    APP_CHECK_PC_INVALID     /**< 复位向量非法（非 Thumb 地址） */
} OTA_APP_CHECK_RESULT_E;

/**
 * @brief 用户设置参数合理性检查结果枚举
 */
typedef enum __OTA_USER_SETINGS_STATE
{
    OTA_OK = 0,               /**< 参数合法 */
    OTA_ERR_FLASH_RANGE,      /**< App 区间超出 Flash 范围 */
    OTA_ERR_ALIGN,            /**< App 起始地址未对齐 Flash 页 */
    OTA_ERR_SIZE,             /**< App 区域大小不合法 */
} OTA_USER_SETINGS_STATE_E;

/**
 * @brief 目标插槽枚举
 */
typedef enum __OTA_ACIVE_SLOT
{
    SLOT_A = 0,              /**< 分区 A */
    SLOT_B = 1               /**< 分区 B */
} OTA_ACIVE_SLOT_E;

/**
 * @brief 固件头部结构 (放在每个 Slot 的头部)
 */
typedef struct __OTA_APP_IMG_HEADER
{
    uint32_t magic;         /**< 固定魔数，用于快速校验头部是否存在 */
    uint32_t img_size;      /**< 固件实际大小 (不含头) */
    uint32_t version;       /**< 版本号 (用于比较新旧) */
    uint16_t img_crc16;     /**< 固件数据的 CRC16 校验值 */
    uint8_t  reserved[2];   /**< 保留字段，保证结构体16字节对齐 */
} OTA_APP_IMG_HEADER_E;

/**
 * @brief Meta 分区结构 (全局状态)
 */
typedef struct __OTA_META_DATA
{
    uint32_t     magic;       /**< Meta 数据有效性魔数 */
    uint32_t     seq_num;     /**< 序列号 (每次更新+1，用于简单的寿命均衡或版本追踪) */
    OTA_ACIVE_SLOT_E active_slot; /**< 当前应启动的插槽 (A 或 B) */
    OTA_SLOT_STATE_E  slotAStatus; /**< slotA 状态 */
    OTA_SLOT_STATE_E  slotBStatus; /**< slotB 状态 */
    uint8_t      reserved[5]; /**< 保留字段，保证结构体16字节对齐 */
} OTA_META_DATA_E;

/**
 * @brief 应用函数指针类型定义
 */
typedef void (*pFunction)(void);


void OTA_U8ArryCopy(uint8_t *dst, const uint8_t *src, uint32_t len);
uint16_t OTA_GetCrc16(const uint8_t *buf, uint32_t len);
void OTA_MemSet(uint8_t *dst, uint8_t val, uint32_t len);
void OTA_MemCopy(uint8_t *dst, const uint8_t *src, uint32_t len);
void OTA_PrintHex32(uint32_t value);

#endif
