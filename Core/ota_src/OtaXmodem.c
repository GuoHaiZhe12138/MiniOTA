/**
 ******************************************************************************
 * @file    OtaXmodem.c
 * @author  MiniOTA Team
 * @brief   Xmodem 协议传输实现
 *          基于状态机的表驱动设计，支持 128/1024 字节包及 CRC 校验
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */

#include "OtaXmodem.h"
#include "OtaInterface.h"
#include "OtaPort.h"
#include "OtaUtils.h"
#include "OtaFlash.h"


/** Xmodem 协议句柄 */
static xmodem_t xm;

/** 接收完成标志 */
static RecFlagState RecComp_Flag;

/* 状态处理函数声明 */
static void Handle_WaitStart(uint8_t ch);
static void Handle_WaitBlk(uint8_t ch);
static void Handle_WaitBlkInv(uint8_t ch);
static void Handle_WaitData(uint8_t ch);
static void Handle_WaitCrc1(uint8_t ch);
static void Handle_WaitCrc2(uint8_t ch);

/** 状态处理函数表（表驱动设计） */
static const xm_state_fn_t xm_state_handlers[XM_STATE_MAX] = {
    [XM_WAIT_START]   = Handle_WaitStart,
    [XM_WAIT_BLK]     = Handle_WaitBlk,
    [XM_WAIT_BLK_INV] = Handle_WaitBlkInv,
    [XM_WAIT_DATA]    = Handle_WaitData,
    [XM_WAIT_CRC1]    = Handle_WaitCrc1,
    [XM_WAIT_CRC2]    = Handle_WaitCrc2
};

/* ---------------- API 实现 ---------------- */

/**
 * @brief  Xmodem 协议初始化
 * @param  addr: Flash 写入起始地址
 */
void OTA_XmodemInit(uint32_t addr)
{
    OTA_MemSet((uint8_t *)&xm, 0, sizeof(xmodem_t));
    xm.state = XM_WAIT_START;
    xm.expected_blk = 1; // Xmodem协议通常从包号1开始
    RecComp_Flag = REC_FLAG_IDLE;
    
    FlashHandle_Init(addr);
}

/**
 * @brief  获取 Xmodem 状态
 * @return Xmodem 状态
 */
xm_state_t OTA_GetXmodemState(void)
{
    return xm.state;
}

/**
 * @brief  获取接收完成标志
 * @return 接收标志（空闲/工作中/完成/中断）
 */
uint8_t OTA_XmodemRevCompFlag(void)
{
    return RecComp_Flag;
}

/**
 * @brief  Xmodem 字节接收处理主函数
 * @param  ch: 接收到的字节
 */
void OTA_XmodemRevByte(uint8_t ch)
{
    if (xm.state < XM_STATE_MAX && xm_state_handlers[xm.state] != NULL)
    {
		xm_state_handlers[xm.state](ch);
    }
    else
    {
        // 异常状态复位
        xm.state = XM_WAIT_START;
    }
}

/* ---------------- 状态处理函数实现 ---------------- */

/**
 * @brief  等待控制符阶段处理
 * @param  ch: 接收到的控制字符
 */
static void Handle_WaitStart(uint8_t ch)
{
    if (ch == XM_SOH) {            // SOH 128字节包
        xm.data_len = 128;
        xm.state = XM_WAIT_BLK;
		RecComp_Flag = REC_FLAG_WORKING;
		OTA_DebugSend("[OTA]:SOH MODE\r\n");
		return;
    }
    else if (ch == XM_STX) {       // STX 1024字节包
        xm.data_len = 1024;
        xm.state = XM_WAIT_BLK;
		RecComp_Flag = REC_FLAG_WORKING;
		OTA_DebugSend("[OTA]:STX MODE\r\n");
		return;
    }
    else if (ch == XM_EOT) {       // EOT 传输结束
        OTA_SendByte(XM_ACK);      // ACK
        
        // 若镜像区还有写回的数据
        if(Flash_GetPageOffset() > 0)
        {
            Flash_Write();
        }
        RecComp_Flag = REC_FLAG_FINISH;
		return;
    }else if (ch == XM_CAN)
	{
		RecComp_Flag = REC_FLAG_INT;
		xm.state = XM_WAIT_START;
		OTA_DebugSend("[OTA][Error]:Transmission interrupted.\r\n");
		return;
	}
	OTA_DebugSend("[OTA][Error]:An Vnknown Character Was Read.\r\n");
	RecComp_Flag = REC_FLAG_IDLE;
}

/**
 * @brief  等待包序号阶段处理
 * @param  ch: 接收到的包序号
 */
static void Handle_WaitBlk(uint8_t ch)
{
    xm.blk = ch;
    xm.state = XM_WAIT_BLK_INV;
	OTA_DebugSend("[OTA]:GET PACK NUM\r\n");
}

/**
 * @brief  等待包序号反码阶段处理
 * @param  ch: 接收到的包序号反码
 */
static void Handle_WaitBlkInv(uint8_t ch)
{
    xm.blk_inv = ch;
    // 校验：包号 + 包号反码 必须等于 0xFF
    if ((uint8_t)(xm.blk + xm.blk_inv) != (uint8_t)0xFF) {
		OTA_DebugSend("[OTA][Error]:Mismatched Package Serial Numbers And Inverse Codes\r\n");
        OTA_SendByte(XM_NAK);        // NAK
        xm.state = XM_WAIT_START;
    } else {
        xm.data_cnt = 0;
        xm.state = XM_WAIT_DATA;
    }
}

/**
 * @brief  等待包数据阶段处理
 * @param  ch: 接收到的数据字节
 */
static void Handle_WaitData(uint8_t ch)
{	
    xm.data_buf[xm.data_cnt++] = ch;
    // 完成小包的接收
    if (xm.data_cnt == xm.data_len) {
        xm.state = XM_WAIT_CRC1;
    }
}

/**
 * @brief  等待 CRC 高字节阶段处理
 * @param  ch: 接收到的 CRC 高字节
 */
static void Handle_WaitCrc1(uint8_t ch)
{
	OTA_DebugSend("[OTA]:Get Crc1\r\n");
    xm.crc_recv = ((uint16_t)ch) << 8;
    xm.state = XM_WAIT_CRC2;
}

/**
 * @brief  等待 CRC 低字节阶段处理
 * @param  ch: 接收到的 CRC 低字节
 */
static void Handle_WaitCrc2(uint8_t ch)
{
	OTA_DebugSend("[OTA]:Get Crc2\r\n");
    xm.crc_recv |= ch;

    xm.crc_calc = XmodemCrc16(xm.data_buf, xm.data_len);

    // 1. CRC校验通过
    if (xm.crc_calc == xm.crc_recv)
    {
        // 情况A: 正常的顺序包
        if (xm.blk == xm.expected_blk)
        {
            // 检查flash镜像是否还能容纳本小包数据
            if(OTA_FLASH_PAGE_SIZE - Flash_GetPageOffset() < xm.data_len)
            {
                // 先进行一次flash写入，更新镜像
                if (Flash_Write() != 0) {
                    xm.state = XM_WAIT_START;
                    return; 
                }
            }

			// 把接收到的包存进当前Flash页的镜像中，等待写入Flash
            U8ArryCopy(&(Flash_GetMirr()[Flash_GetPageOffset()]), xm.data_buf, xm.data_len);
            
            // 更新状态
            xm.expected_blk++;
			Flash_SetPageOffset(Flash_GetPageOffset() + xm.data_len);
            
            OTA_SendByte(XM_ACK);     // ACK
        }
        // 情况B: 发送端重发了上一个已写入的包
        else if (xm.blk == (uint8_t)(xm.expected_blk - 1))
        {
            OTA_SendByte(XM_ACK);
        }
        // 情况C: 包号完全对不上
        else
        {
			OTA_DebugSend("[OTA][Error]:Packet Order Confusion\r\n");
            OTA_SendByte(XM_NAK);     // 取消传输或请求重发
			return;
        }
    }
    // 2. CRC校验失败
    else {
		OTA_DebugSend("[OTA][Error]:Crc16 Is Inconsistent\r\n");
        OTA_SendByte(XM_NAK);     // NAK
    }

    xm.state = XM_WAIT_START; // 回到开始等待下一个包头
}
