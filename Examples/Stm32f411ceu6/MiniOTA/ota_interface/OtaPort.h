/**
 ******************************************************************************
 * @file    OtaPort.h
 * @author  MiniOTA Team
 * @brief   硬件底层适配接口定义
 *          用户需要根据具体的 MCU 平台实现本文件中的接口
 ******************************************************************************
 */

#ifndef OTAPORT_H
#define OTAPORT_H

#include <OtaInterface.h>

/**
 * @brief  判断是否应强制进入 IAP 模式
 * @return 1: 进入 IAP, 0: 尝试正常启动
 */
uint8_t OTA_ShouldEnterIap(void);

/**
 * @brief  系统外设逆初始化
 *         在跳转至 App 前，应关闭所有中断并复位外设状态
 */
void OTA_PeripheralsDeInit(void);

/**
 * @brief  解锁 Flash 写入权限
 * @return 1: 成功, 0: 失败
 */
uint8_t OTA_FlashUnlock(void);

/**
 * @brief  上锁 Flash 写入权限
 * @return 1: 成功, 0: 失败
 */
uint8_t OTA_FlashLock(void);

/**
 * @brief  擦除指定地址所在的 Flash 页
 * @param  addr: 目标页地址
 * @return 0: 成功, 其他: 失败
 */
int OTA_ErasePage(uint32_t addr);

/**
 * @brief  写入半字数据到 Flash
 * @param  addr: 目标地址
 * @param  data: 16位数据
 * @return 0: 成功, 其他: 失败
 */
int OTA_DrvProgramHalfword(uint32_t addr, uint16_t data);

/**
 * @brief  从 Flash 读取数据
 * @param  addr: 起始地址
 * @param  buf: 存储缓冲区
 * @param  len: 读取长度
 */
void OTA_DrvRead(uint32_t addr, uint8_t *buf, uint16_t len);

/**
 * @brief  向外部发送一个字节 (用于 Xmodem 协议应答)
 * @param  byte: 待发送字节
 * @return 1: 成功, 0: 失败
 */
uint8_t OTA_SendByte(uint8_t byte);

/**
 * @brief  发送调试信息 (例如字符串日志)
 * @param  data: 字符串指针
 * @return 1: 成功, 0: 失败
 */
uint8_t OTA_DebugSend(const char *data);

/**
 * @brief  从传输缓冲区读取一个字节
 * @return 接收到的字节
 */
uint8_t OTA_TransReadByte(void);

/**
 * @brief  查询传输缓冲区是否为空
 * @return 1: 为空, 0: 有数据
 */
uint8_t OTA_IsTransEmpty(void);

/**
 * @brief  毫秒级延时函数
 */
void OTA_Delay1ms(void);

#endif
