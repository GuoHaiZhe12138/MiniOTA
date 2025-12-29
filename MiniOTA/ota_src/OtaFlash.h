#ifndef OTAFLASH_H
#define OTAFLASH_H

#include "OtaInterface.h"
/* ---------------- Flash Handle ---------------- */
typedef struct {
    uint32_t curr_addr;
    uint16_t page_offset;
    uint8_t  page_buf[OTA_FLASH_PAGE_SIZE];
} flashHandle_t;

uint32_t Flash_GetCurAddr(void);
uint16_t Flash_GetPageOffset(void);
void Flash_SetPageOffset(uint16_t ch);
uint8_t *Flash_GetMirr(void);
void FlashHandle_Init(void);
int Flash_Write(void);

#endif
