
#include "OtaInterface.h"
#include "OtaFlash.h"
#include "OtaPort.h"
#include "OtaUtils.h"

static flashHandle_t flash = {0};

uint32_t Flash_GetCurAddr(void)
{
	return flash.curr_addr;
}

void Flash_SetCurAddr(uint32_t ch)
{
	flash.curr_addr = ch;
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

void Flash_SetMirr(const uint8_t *mirr, uint16_t length)
{
	OTA_MemCopy(flash.page_buf, mirr, length);
}

// FlashHandle初始化
void FlashHandle_Init(uint32_t addr)
{
    flash.curr_addr   = addr;
    flash.page_offset = 0;
    // 预读取当前页内容，以便进行 Read-Modify-Write 操作
    OTA_DrvRead(addr, flash.page_buf, OTA_FLASH_PAGE_SIZE);
}

// Flash写入整页函数
int Flash_Write(void)
{
    /* 打开 Flash（解锁） */
    if (OTA_FlashUnlock() != 0)
	{
		OTA_DebugSend("[OTA][Error]:Flash UnLock Faild\r\n");
		return 1;
	}

    /* 擦当前页 */
    if (OTA_ErasePage(flash.curr_addr) != 0)
    {
		OTA_DebugSend("[OTA][Error]:Flash Erase Faild\r\n");
        if(OTA_FlashLock() != 0)
		{
			OTA_DebugSend("[OTA][Error]:Flash Lock Faild\r\n");
		}
        return 1;
    }

    /* 写整页 */
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i += 2)
    {
        uint16_t hw = flash.page_buf[i] | (flash.page_buf[i + 1] << 8);
        if (OTA_DrvProgramHalfword(flash.curr_addr + i, hw) != 0)
        {
            if(OTA_FlashLock() != 0)
			{
				OTA_DebugSend("[OTA][Error]:Flash Lock Faild\r\n");
			}
            return 1;
        }
    }

	/* 写后读回校验（逐字节对比） */
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i++)
    {
        uint8_t flash_byte = *(volatile uint8_t *)(flash.curr_addr + i);

        if (flash_byte != flash.page_buf[i])
        {
            /* Flash 中的内容与接收到的镜像不一致 */
			OTA_DebugSend("[OTA][Error]:Flash Verify Error ,Data Mismatch\r\n");
            return 1;
        }
    }

	
    OTA_FlashLock();
    
    // 更新镜像内容
    flash.page_offset = 0;
    flash.curr_addr += OTA_FLASH_PAGE_SIZE;
    return 0;
}
