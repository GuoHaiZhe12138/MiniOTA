/**
 ******************************************************************************
 * @file    OtaXmodem.h
 * @author  MiniOTA Team
 * @brief   Xmodem 协议传输头文件
 *          定义 Xmodem 协议状态机、控制字符及数据结构
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */

#ifndef __OTAXMODEM_H
#define __OTAXMODEM_H

#include "OtaInterface.h"

/** @defgroup XMODEM_Control_Characters
 * @{
 */
#define XM_SOH   0x01  /**< 128字节数据包起始符 */
#define XM_STX   0x02  /**< 1024字节数据包起始符 */
#define XM_EOT   0x04  /**< 传输结束符 */
#define XM_ACK   0x06  /**< 确认应答 */
#define XM_NAK   0x15  /**< 否定应答/请求重发 */
#define XM_CAN   0x18  /**< 传输取消 */
/**
 * @}
 */

/** @defgroup XMODEM_State_Machine
 * @{
 */

/**
 * @brief Xmodem 状态机状态枚举
 */
typedef enum __OTA_XM_STATE
{
    XM_WAIT_START = 0,  /**< 等待起始符 */
    XM_WAIT_BLK,        /**< 等待包序号 */
    XM_WAIT_BLK_INV,    /**< 等待包序号反码 */
    XM_WAIT_DATA,       /**< 等待数据 */
    XM_WAIT_CRC1,       /**< 等待CRC高字节 */
    XM_WAIT_CRC2,       /**< 等待CRC低字节 */
    XM_STATE_MAX        /**< 状态总数 */
} OTA_XM_STATE_E;

/**
 * @brief 接收完成标志枚举
 */
typedef enum __OTA_REC_FLAG_STATE
{
    REC_FLAG_IDLE = 0,  /**< 空闲 */
    REC_FLAG_WORKING,   /**< 传输进行中 */
    REC_FLAG_FINISH,    /**< 传输完成 */
    REC_FLAG_INT        /**< 传输中断 */
} OTA_REC_FLAG_STATE_E;

/**
 * @brief Xmodem 协议句柄结构体
 */
typedef struct __OTA_XMODEM_HANDLE
{
    OTA_XM_STATE_E state;        /**< 当前状态 */
    uint8_t    blk;          /**< 当前包序号 */
    uint8_t    blk_inv;      /**< 当前包序号反码 */
    uint8_t    expected_blk; /**< 期望的下一包序号 */
    uint16_t   data_len;     /**< 当前包数据长度 */
    uint16_t   data_cnt;     /**< 已接收数据计数 */
    uint16_t   crc_recv;     /**< 接收到的CRC值 */
    uint16_t   crc_calc;     /**< 计算得到的CRC值 */
    uint8_t    data_buf[1024]; /**< 数据缓冲区 */
} OTA_XMODEM_HANDLE;

/**
 * @brief 状态处理函数指针类型
 */
typedef void (*xm_state_fn_t)(uint8_t);
/**
 * @}
 */

/** @defgroup XMODEM_API
 * @{
 */

/**
 * @brief  Xmodem 协议初始化
 * @param  addr: Flash 写入起始地址
 */
void OTA_XmodemInit(uint32_t addr);

/**
 * @brief  Xmodem 字节接收处理
 * @param  ch: 接收到的字节
 */
void OTA_XmodemRevByte(uint8_t ch);

/**
 * @brief  获取接收完成标志
 * @return 接收标志状态
 */
uint8_t OTA_XmodemRevCompFlag(void);

/**
 * @brief  获取 Xmodem 状态
 * @return Xmodem 状态
 */
OTA_XM_STATE_E OTA_GetXmodemState(void);
/**
 * @}
 */

#endif
