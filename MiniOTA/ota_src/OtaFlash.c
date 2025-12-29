
#include "OtaInterface.h"
#include "OtaFlash.h"
#include "OtaPort.h"

static flashHandle_t flash = {0};

uint32_t Flash_GetCurAddr(void)
{
	return flash.curr_addr;
}

uint16_t Flash_GetPageOffset(void)
{
	return flash.page_offset;
}

void Flash_SetPageOffset(uint16_t ch)
{
	flash.page_offset = ch;
}

uint8_t *Flash_GetMirr(void)
{
	return flash.page_buf;
}

// FlashHandle初始化
void FlashHandle_Init(void)
{
    flash.curr_addr   = OTA_APP_START_ADDRESS;
    flash.page_offset = 0;
    // 预读取当前页内容，以便进行 Read-Modify-Write 操作
    OTA_DrvRead(OTA_APP_START_ADDRESS, flash.page_buf, OTA_FLASH_PAGE_SIZE);
}

// Flash写入整页函数
int Flash_Write(void)
{
    /* 打开 Flash（解锁） */
    if (OTA_FlashUnlock() != 0)
        return 1;

    /* 擦当前页 */
    if (OTA_ErasePage(flash.curr_addr) != 0)
    {
        OTA_FlashLock();
        return 1;
    }

    /* 写整页 */
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i += 2)
    {
        // 注意：这里假定系统是小端模式，或者OTA_DrvProgramHalfword内部处理了
        uint16_t hw = flash.page_buf[i] | (flash.page_buf[i + 1] << 8);
        if (OTA_DrvProgramHalfword(flash.curr_addr + i, hw) != 0)
        {
            OTA_FlashLock();
            return 1;
        }
    }

    OTA_FlashLock();
    
    // 更新镜像内容
    flash.page_offset = 0;
    flash.curr_addr += OTA_FLASH_PAGE_SIZE;
    return 0;
}
