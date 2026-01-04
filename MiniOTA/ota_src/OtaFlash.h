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
uint8_t *Flash_GetMirr(void);
void Flash_SetCurAddr(uint32_t ch);
void Flash_SetPageOffset(uint16_t ch);
void Flash_SetMirr(const uint8_t *mirr, uint16_t length);
void FlashHandle_Init(uint32_t addr);
int Flash_Write(void);

#endif
