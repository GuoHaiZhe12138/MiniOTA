/*******************************************************************************
  * @file           : 
  * @brief          : 
  ******************************************************************************
  * @attention
  *
  *
  *****************************************************************************
*/
#ifndef  OTAINTERFACE_H
#define  OTAINTERFACE_H

#include <stdint.h>
#include <stddef.h>


/* App 起始地址 */
#define OTA_APP_START_ADDRESS   0x08003000UL

/* Flash 结束地址 */
#define OTA_FLASH_END_ADDRESS   0x08007FFFUL

/* App 最大大小（字节） */
#define OTA_APP_MAX_SIZE        (OTA_FLASH_END_ADDRESS - OTA_APP_START_ADDRESS + 1)

/* flash页大小 */
#define OTA_FLASH_PAGE_SIZE   1024

/* bootloader中接收app的缓冲区大小 */
#define OTA_BUFFER_SIZE   512


void OTA_RunOTA(void);

#endif
