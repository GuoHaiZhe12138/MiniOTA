/**
 ******************************************************************************
 * @file    OtaPort.c
 * @author  MiniOTA Team
 * @brief   硬件平台适配层实现
 *          基于 STM32F10x 系列 MCU 的特定实现
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */
#include "OtaInterface.h"
#include "OtaXmodem.h"
#include "OtaUtils.h"

/**
 * @brief  判断是否应进入 IAP 模式
 * @return 1: GPIOB Pin11 为高电平，进入 IAP; 0: 正常启动
 */
uint8_t OTA_ShouldEnterIap(void)
{
	
}

/**
 * @brief  外设逆初始化（跳转前清理）
 *         根据具体应用需要，可在此关闭所有外设时钟、中断等
 */
void OTA_PeripheralsDeInit(void)
{
	// 用户可在此添加特定外设清理代码
}

/**
 * @brief  串口接收中断回调函数
 * @param  byte: 接收到的字节
 */
void OTA_ReceiveTask(uint8_t byte)
{
	OTA_XmodemRevByte(byte);
}

/**
 * @brief  Flash 解锁并清除标志位
 * @return 0: 成功
 */
uint8_t OTA_FlashUnlock(void)
{
    
}

/**
 * @brief  Flash 上锁
 * @return 0: 成功
 */
uint8_t OTA_FlashLock(void)
{
	
}

/**
 * @brief  擦除指定地址所在的 Flash 页
 * @param  addr: 目标页地址
 * @return 0: 成功, 1: 失败
 */
int OTA_ErasePage(uint32_t addr)
{
    
}

/**
 * @brief  写入半字数据到 Flash
 * @param  addr: 目标地址
 * @param  data: 16位数据
 * @return 0: 成功, 1: 失败
 */
int OTA_DrvProgramHalfword(uint32_t addr, uint16_t data)
{
    
}

/**
 * @brief  从 Flash 读取数据到缓冲区
 * @param  addr: 源地址
 * @param  buf: 目标缓冲区
 * @param  len: 读取长度
 */
void OTA_DrvRead(uint32_t addr, uint8_t *buf, uint16_t len)
{
	
}

/**
 * @brief  发送一个字节到 USART1（用于 Xmodem 协议应答）
 * @param  byte: 待发送字节
 * @return 0: 发送成功（硬件发送完成即返回）
 */
uint8_t OTA_SendByte(uint8_t byte)
{
    
}

/**
 * @brief  发送调试信息到 USART2
 * @param  data: 字符串指针
 * @return 1: 成功, 0: 参数无效
 */
uint8_t OTA_DebugSend(const char *data)
{
    
}

/**
 * @brief  毫秒级延时函数（基于空操作循环）
 *         注意：此延时精度依赖于 CPU 频率
 */
void OTA_Delay1ms(void)
{
    
}
