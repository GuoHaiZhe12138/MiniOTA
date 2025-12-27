/*******************************************************************************
  * @file           : 
  * @brief          : 
  ******************************************************************************
  * @attention
  *
  *
  *****************************************************************************/
#ifndef OTAPORT_H
#define OTAPORT_H

#include <stdint.h>

// 中断输入函数
void OTA_ReceiveTask(uint8_t byte);
	
// Flash页解锁
uint8_t OTA_FlashUnlock(void);

// Flash页上锁
uint8_t OTA_FlashLock(void);

// Flash页擦除
int OTA_ErasePage(uint32_t addr);

// Flash页半字写入
int OTA_DrvProgramHalfword(uint32_t addr, uint16_t data);

// Flash页读取
void OTA_DrvRead(uint32_t addr, uint8_t *buf, uint16_t len);

// 向bin文件发送方发送字节
uint8_t OTA_SendByte(uint8_t byte);


/************** Debug Message 发送接口 **************/
/**
 * @brief 发送调试信息到串口（例如 printf 日志）
 * @param data   指向要发送的数据缓冲区
 * @return 1 发送成功，0 发送失败
 */
uint8_t OTA_DebugSend(const char *data);

/************** App Code 接收接口 **************/

/**
 * @brief 从串口接收 App Code 数据
 * @return 实际接收到的字节
 */
uint8_t OTA_TransReadByte(void);

/**
 * @brief 查询串口是否有可读的 App Code 数据
 * @return 1 表示有数据可读，0 表示没有数据
 */
uint8_t OTA_IsTransEmpty(void);

/**
 * @brief 延时函数
 */
void OTA_Delay1ms(void);

#endif
