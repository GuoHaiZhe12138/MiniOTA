/*******************************************************************************
  * @file           : OtaTrans.c
  * @brief          : Xmodem Table-Driven Refactored
  ******************************************************************************/
#include "OtaXmodem.h"
#include "OtaInterface.h"
#include "OtaPort.h"
#include "OtaUtils.h"
#include "OtaFlash.h"
#include "string.h"

#include "stm32f10x.h"
#include "stdio.h"

static xmodem_t xm;

static RecFlagState RecComp_Flag;
static uint8_t RecFlag_cnt = 0;

/* 状态处理函数声明 */
static void Handle_WaitStart(uint8_t ch);
static void Handle_WaitBlk(uint8_t ch);
static void Handle_WaitBlkInv(uint8_t ch);
static void Handle_WaitData(uint8_t ch);
static void Handle_WaitCrc1(uint8_t ch);
static void Handle_WaitCrc2(uint8_t ch);

// 表驱动结构：状态处理函数数组
static const xm_state_fn_t xm_state_handlers[XM_STATE_MAX] = {
    [XM_WAIT_START]   = Handle_WaitStart,
    [XM_WAIT_BLK]     = Handle_WaitBlk,
    [XM_WAIT_BLK_INV] = Handle_WaitBlkInv,
    [XM_WAIT_DATA]    = Handle_WaitData,
    [XM_WAIT_CRC1]    = Handle_WaitCrc1,
    [XM_WAIT_CRC2]    = Handle_WaitCrc2
};

/* ---------------- API 实现 ---------------- */

// Xmodem 初始化
void OTA_XmodemInit(uint32_t addr)
{
    memset(&xm, 0, sizeof(xmodem_t));
    xm.state = XM_WAIT_START;
    xm.expected_blk = 1; // Xmodem协议通常从包号1开始
    RecComp_Flag = REC_FLAG_IDLE;
    
    FlashHandle_Init(addr);
}

// 获取xmodemHandle
xmodem_t* OTA_GetXmodemHandle(void)
{
    return &xm;
}

uint8_t OTA_XmodemRevCompFlag(void)
{
    return RecComp_Flag;
}

// 主接收函数
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

// 等待控制符阶段
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
		RecFlag_cnt ++;
		if(RecFlag_cnt >= 2)
		{
			RecComp_Flag = REC_FLAG_INT;
			RecFlag_cnt = 0;
		}
	}
	OTA_DebugSend("[OTA][Error]:An Vnknown Character Was Read.\r\n");
	RecComp_Flag = REC_FLAG_IDLE;
}

// 等待包序号阶段
static void Handle_WaitBlk(uint8_t ch)
{
    xm.blk = ch;
    xm.state = XM_WAIT_BLK_INV;
	OTA_DebugSend("[OTA]:GET PACK NUM\r\n");
}

// 等待包序号反码阶段
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

// 等待包数据阶段
static void Handle_WaitData(uint8_t ch)
{	
    xm.data_buf[xm.data_cnt++] = ch;
    // 完成小包的接收
    if (xm.data_cnt == xm.data_len) {
        xm.state = XM_WAIT_CRC1;
    }
}

// 等待前八位crc阶段
static void Handle_WaitCrc1(uint8_t ch)
{
	OTA_DebugSend("[OTA]:Get Crc1\r\n");
    xm.crc_recv = ((uint16_t)ch) << 8;
    xm.state = XM_WAIT_CRC2;
}

// 等待后八位crc阶段
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

