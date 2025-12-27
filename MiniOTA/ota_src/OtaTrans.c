/*******************************************************************************
  * @file           : OtaTrans.c
  * @brief          : Xmodem Table-Driven Refactored
  ******************************************************************************/
#include "OtaTrans.h"
#include "OtaInterface.h"
#include "OtaPort.h"
#include "string.h"

#include "stm32f10x.h"
#include "stdio.h"

static xmodem_t xm = {0};
static flashHandle_t flash = {0};
static uint8_t RecComp_Flag = 0;

/* 内部函数声明 */
int Flash_Write(flashHandle_t *h);
void FlashHandle_Init(flashHandle_t *h, uint32_t start_addr);
void U8ArryCopy(uint8_t *dst, const uint8_t *src, uint32_t len);
static uint16_t xmodem_crc16(const uint8_t *buf, uint32_t len);

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

// 调试：
void UART2_PrintHex8(uint8_t data)
{
    uint8_t high;
    uint8_t low;

    high = (data >> 4) & 0x0F;
    low  = data & 0x0F;

    if (high < 10)
        USART_SendData(USART2,'0' + high);
    else
        USART_SendData(USART2,'A' + high - 10);

    if (low < 10)
        USART_SendData(USART2,'0' + low);
    else
        USART_SendData(USART2,'A' + low - 10);
}

/* ---------------- API 实现 ---------------- */

// Xmodem 初始化
void OTA_XmodemInit(void)
{
    memset(&xm, 0, sizeof(xmodem_t));
    xm.state = XM_WAIT_START;
    xm.expected_blk = 1; // Xmodem协议通常从包号1开始
    RecComp_Flag = 0;
    
    FlashHandle_Init(&flash, OTA_APP_START_ADDRESS);
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

// 主接收函数：通过查表调用对应的状态处理函数
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
		RecComp_Flag = 1;
		OTA_DebugSend("[OTA]:SOH MODE\r\n");
    }
    else if (ch == XM_STX) {       // STX 1024字节包
        xm.data_len = 1024;
        xm.state = XM_WAIT_BLK;
		RecComp_Flag = 1;
		OTA_DebugSend("[OTA]:STX MODE\r\n");
    }
    else if (ch == XM_EOT) {       // EOT 传输结束
        OTA_SendByte(XM_ACK);      // ACK
        
        // 若镜像区还有写回的数据
        if(flash.page_offset > 0)
        {
            Flash_Write(&flash);
        }
        RecComp_Flag = 2;
    }
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
    if ((uint8_t)(xm.blk + xm.blk_inv) != 0xFF) {
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

// 等待后八位crc阶段 (逻辑修复重点区域)
static void Handle_WaitCrc2(uint8_t ch)
{
	OTA_DebugSend("[OTA]:Get Crc2\r\n");
    xm.crc_recv |= ch;

    xm.crc_calc = xmodem_crc16(xm.data_buf, xm.data_len);

    // 1. CRC校验通过
    if (xm.crc_calc == xm.crc_recv)
    {
        // 情况A: 正常的顺序包
        if (xm.blk == xm.expected_blk)
        {
            // 检查flash镜像是否还能容纳本小包数据
            if(OTA_FLASH_PAGE_SIZE - flash.page_offset < xm.data_len)
            {
                // 先进行一次flash写入，更新镜像
                if (Flash_Write(&flash) != 0) {
                    // 写入失败处理，通常可以发送CAN或无响应，此处发送NAK重试
                    OTA_SendByte(XM_NAK);
                    xm.state = XM_WAIT_START;
                    return; 
                }
            }

            U8ArryCopy(&(flash.page_buf[flash.page_offset]), xm.data_buf, xm.data_len);
            
            // 更新状态
            xm.expected_blk++;
            flash.page_offset += xm.data_len;
            
            OTA_SendByte(XM_ACK);     // ACK
        }
        // 情况B: 发送端没收到ACK，重发了上一个包 (Duplicate Packet)
        // 修复：必须处理这种情况，否则发送端会重试直到超时
        else if (xm.blk == (uint8_t)(xm.expected_blk - 1))
        {
            OTA_SendByte(XM_ACK);     // 直接ACK，但不写入Flash
        }
        // 情况C: 包号完全对不上
        else
        {
            OTA_SendByte(XM_NAK);     // 取消传输或请求重发
        }
    }
    // 2. CRC校验失败
    else {
        OTA_SendByte(XM_NAK);     // NAK
    }

    xm.state = XM_WAIT_START; // 回到开始等待下一个包头
}

/* ---------------- 辅助函数实现 ---------------- */

// FlashHandle初始化
void FlashHandle_Init(flashHandle_t *h, uint32_t start_addr)
{
    h->curr_addr   = start_addr;
    h->page_offset = 0;
    // 预读取当前页内容，以便进行 Read-Modify-Write 操作
    OTA_DrvRead(start_addr, h->page_buf, OTA_FLASH_PAGE_SIZE);
}

// CRC16/XMODEM (poly=0x1021, init=0)
static uint16_t xmodem_crc16(const uint8_t *buf, uint32_t len)
{
    uint16_t crc = 0;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)buf[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

// Flash写入整页函数
int Flash_Write(flashHandle_t *h)
{
	OTA_DebugSend("[OTA]:Flashing\r\n");
    /* 打开 Flash（解锁） */
    if (OTA_FlashUnlock() != 0)
        return 1;

    /* 擦当前页 */
    if (OTA_ErasePage(h->curr_addr) != 0)
    {
        OTA_FlashLock();
        return 1;
    }

    /* 写整页 */
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i += 2)
    {
        // 注意：这里假定系统是小端模式，或者OTA_DrvProgramHalfword内部处理了
        uint16_t hw = h->page_buf[i] | (h->page_buf[i + 1] << 8);
        if (OTA_DrvProgramHalfword(h->curr_addr + i, hw) != 0)
        {
            OTA_FlashLock();
            return 1;
        }
    }

    OTA_FlashLock();
    
    // 更新镜像内容
    h->page_offset = 0;
    h->curr_addr += OTA_FLASH_PAGE_SIZE;
    return 0;
}

// uint8数组复制
void U8ArryCopy(uint8_t *dst, const uint8_t *src, uint32_t len)
{
    if (dst == 0 || src == 0) return;

    while (len--)
    {
        *dst++ = *src++;
    }
}
