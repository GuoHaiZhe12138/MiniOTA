/*******************************************************************************
  * @file           : OtaInterface.h
  * @brief          : MiniOTA 全局配置与接口定义(Global configuration and interface definition)
  ******************************************************************************
  * @attention
  * 包含 App 起始地址、Flash 页大小及全局 OTA 运行接口定义
  * (Includes the App's starting address, Flash page size, and global OTA runtime interface definition)
  *****************************************************************************
*/
#ifndef  OTAINTERFACE_H
#define  OTAINTERFACE_H

#include <stdint.h>
#include <stddef.h>

// Flash大小
#define OTA_FLASH_SIZE            0x8000

/* Flash 起始地址 */
#define OTA_FLASH_START_ADDRESS   0x08000000UL

/* 分配给MiniOta空间的起始地址 */
#define OTA_TOTAL_START_ADDRESS   0x08003000UL

/* 分配给MiniOta空间的最大大小(字节) */
#define OTA_APP_MAX_SIZE        OTA_FLASH_SIZE - (OTA_TOTAL_START_ADDRESS - OTA_FLASH_START_ADDRESS)

/* flash页大小 */
#define OTA_FLASH_PAGE_SIZE     1024

/* ==================================== MiniOTA内部宏定义 ===================================== */
// 状态区大小
#define OTA_META_SIZE       	OTA_FLASH_PAGE_SIZE
// 状态区地址
#define OTA_META_ADDR      		OTA_TOTAL_START_ADDRESS
// APP_A + APP_B 分区起始地址
#define OTA_APP_REGION_ADDR 	(OTA_META_ADDR + OTA_META_SIZE)
// APP_A + APP_B 分区起始大小
#define OTA_APP_REGION_SIZE 	(OTA_FLASH_SIZE - (OTA_APP_REGION_ADDR - OTA_FLASH_START_ADDRESS))
// 单个APP分区的大小
#define OTA_APP_SLOT_SIZE   	((OTA_APP_REGION_SIZE / 2) / OTA_FLASH_PAGE_SIZE * OTA_FLASH_PAGE_SIZE)
// APP_A分区地址
#define OTA_APP_A_ADDR      	OTA_APP_REGION_ADDR
// APP_B分区地址
#define OTA_APP_B_ADDR      	(OTA_APP_A_ADDR + OTA_APP_SLOT_SIZE)


void OTA_Run(void);
void OTA_StartIAP(void);

#endif
