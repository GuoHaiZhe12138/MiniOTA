/*******************************************************************************
  * @file           : 
  * @brief          : 
  ******************************************************************************
  * @attention
  *
  *
  *****************************************************************************/
#include "OtaPort.h"
#include "OtaXmodem.h"
#include "string.h"
#include "stm32f10x_flash.h"

void OTA_ReceiveTask(uint8_t byte)
{
	OTA_XmodemRevByte(byte);
}

// Flash页解锁
uint8_t OTA_FlashUnlock(void)
{
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    return 0;
}

// Flash页上锁
uint8_t OTA_FlashLock(void)
{
	FLASH_Lock();
    return 0;
}

// Flash页擦除
int OTA_ErasePage(uint32_t addr)
{
    return (FLASH_ErasePage(addr) == FLASH_COMPLETE) ? 0 : 1;
}

// Flash页半字写入
int OTA_DrvProgramHalfword(uint32_t addr, uint16_t data)
{
    return (FLASH_ProgramHalfWord(addr, data) == FLASH_COMPLETE) ? 0 : 1;
}
// Flash页读取
void OTA_DrvRead(uint32_t addr, uint8_t *buf, uint16_t len)
{
    memcpy(buf, (uint8_t *)addr, len);
}


/**
 * @brief  发送 Xmodem 控制字节（ACK / NAK / CAN / 'C'）
 * @param  ctrl  要发送的控制字节
 * @return 1 发送成功，0 默认返回 0
 */
uint8_t OTA_SendByte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, byte);
    return 0; 
}


/**
 * @brief 发送调试信息到串口
 * @param data   指向要发送的数据缓冲区
 * @return 1 发送成功，0 默认返回 0
 */
uint8_t OTA_DebugSend(const char *data)
{
    if (!data) return 0;
    while (*data)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, *data++);
    }

    return 1;
}

/**
 * @brief 延时函数
 */
void OTA_Delay1ms(void)
{
    volatile uint32_t count = 8000;
    while (count--)
    {
        __NOP();
    }
}

