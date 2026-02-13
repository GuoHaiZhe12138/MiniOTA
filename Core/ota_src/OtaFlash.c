/**
 ******************************************************************************
 * @file    OtaFlash.c
 * @author  MiniOTA Team
 * @brief   Flash 驱动层抽象
 *          提供 Flash 页缓冲管理、读写校验及地址映射功能
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */

#include "OtaInterface.h"
#include "OtaFlash.h"
#include "OtaPort.h"
#include "OtaUtils.h"
#include "OtaFlashIfoDef.h"

/** Flash 句柄，全局唯一 */
static OTA_FLASH_HANDLE flash;

/** Flash 布局全局缓存 */
static const MiniOTA_FlashLayout *s_flash_layout = OTA_NULL;

typedef struct
{
    uint32_t sector_start;
    uint32_t sector_size;
} OTA_FLASH_SECTOR_INFO;

/**
 * @brief  懒加载获取布局指针
 */
static const MiniOTA_FlashLayout* OTA_FlashGetLayoutInternal(void)
{
    if (s_flash_layout == OTA_NULL)
    {
        s_flash_layout = MiniOTA_GetLayout();
        if (s_flash_layout == OTA_NULL)
        {
            OTA_DebugSend("[OTA][Error]: Flash layout is NULL.\r\n");
        }
        else
        {
            /* 简单一致性检查：起始地址与总大小需与 OtaInterface 中配置一致 */
            if ((s_flash_layout->start_addr != OTA_FLASH_START_ADDRESS) ||
                (s_flash_layout->total_size != OTA_FLASH_SIZE))
            {
                OTA_DebugSend("[OTA][Warn]: Flash layout mismatch with OtaInterface settings.\r\n");
            }
        }
    }
    return s_flash_layout;
}

/**
 * @brief  根据地址在布局中定位所在物理扇区
 * @param  addr  目标地址
 * @param  info  输出扇区信息（起始地址与大小）
 * @return OTA_TRUE: 成功, OTA_FALSE: 失败/越界
 */
static uint8_t OTA_FlashLocateSector(uint32_t addr, OTA_FLASH_SECTOR_INFO *info)
{
    const MiniOTA_FlashLayout *layout = OTA_FlashGetLayoutInternal();
    if (layout == OTA_NULL)
    {
        return OTA_FALSE;
    }

    uint32_t start = layout->start_addr;
    uint32_t end   = layout->start_addr + layout->total_size;

    if ((addr < start) || (addr >= end))
    {
        OTA_DebugSend("[OTA][Error]: Address out of flash layout range.\r\n");
        return OTA_FALSE;
    }

    uint32_t curr = start;
    for (uint32_t gi = 0U; gi < layout->group_count; ++gi)
    {
        const MiniOTA_SectorGroup *g = &layout->groups[gi];
        uint32_t group_size = g->count * g->size;

        if (addr < (curr + group_size))
        {
            uint32_t offset_in_group = addr - curr;
            uint32_t index_in_group  = offset_in_group / g->size;
            uint32_t sector_start    = curr + index_in_group * g->size;

            if (info != OTA_NULL)
            {
                info->sector_start = sector_start;
                info->sector_size  = g->size;
            }
            return OTA_TRUE;
        }

        curr += group_size;
    }

    OTA_DebugSend("[OTA][Error]: Failed to locate sector.\r\n");
    return OTA_FALSE;
}

/**
 * @brief  获取当前 Flash 操作地址
 * @return 当前地址
 */
uint32_t OTA_FlashGetCurAddr(void)
{
	return flash.curr_addr;
}

/**
 * @brief  设置当前 Flash 操作地址
 * @param  ch: 目标地址
 */
void OTA_FlashSetCurAddr(uint32_t ch)
{
	flash.curr_addr = ch;
}

/**
 * @brief  获取当前页内偏移
 * @return 页内偏移
 */
uint16_t OTA_FlashGetPageOffset(void)
{
	return flash.page_offset;
}

/**
 * @brief  设置当前页内偏移
 * @param  ch: 目标偏移
 */
void OTA_FlashSetPageOffset(uint16_t ch)
{
	flash.page_offset = ch;
}

/**
 * @brief  获取页缓冲区的指针
 * @return 页缓冲区指针
 */
uint8_t *OTA_FlashGetMirr(void)
{
	return flash.page_buf;
}

/**
 * @brief  将数据复制到页缓冲区
 * @param  mirr: 源数据指针
 * @param  length: 复制长度
 */
void OTA_FlashSetMirr(const uint8_t *mirr, uint16_t length)
{
	OTA_MemCopy(flash.page_buf, mirr, length);
}

/**
 * @brief  初始化 Flash 句柄，并预读当前页内容
 * @param  addr: 起始地址
 */
void OTA_FlashHandleInit(uint32_t addr)
{
    flash.curr_addr   = addr;
    flash.page_offset = 0;
    flash.sector_start = 0;
    flash.sector_size  = 0;
    flash.sector_valid = 0;

    (void)OTA_FlashGetLayoutInternal(); /* 确保布局已初始化（用于后续模式判断与校验） */

    /* 预读取当前页内容，以便进行 Read-Modify-Write 操作
     * 自动/手动模式下，此处行为一致：先读整页作为镜像缓冲。 */
    OTA_DrvRead(addr, flash.page_buf, OTA_FLASH_PAGE_SIZE);
}

/**
 * @brief  自动模式下的页写入实现（适用于均匀页 Flash）
 * @return 0: 成功, 1: 失败
 */
static int OTA_FlashWrite_Auto(void)
{
    /* 打开 Flash（解锁） */
    if (OTA_FlashUnlock() != 0)
    {
        OTA_DebugSend("[OTA][Error]:Flash UnLock Failed\r\n");
        return 1;
    }

    /* 擦当前页（在均匀布局下，页即擦除单元） */
    if (OTA_ErasePage(flash.curr_addr) != 0)
    {
        OTA_DebugSend("[OTA][Error]:Flash Erase Failed\r\n");
        if (OTA_FlashLock() != 0)
        {
            OTA_DebugSend("[OTA][Error]:Flash Lock Failed\r\n");
        }
        return 1;
    }

    /* 写整页（按半字编程） */
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i += 2)
    {
        uint16_t hw = (uint16_t)(flash.page_buf[i] | (flash.page_buf[i + 1] << 8));
        if (OTA_DrvProgramHalfword(flash.curr_addr + (uint32_t)i, hw) != 0)
        {
            if (OTA_FlashLock() != 0)
            {
                OTA_DebugSend("[OTA][Error]:Flash Lock Failed\r\n");
            }
            return 1;
        }
    }

    /* 写后读回校验（逐字节对比） */
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i++)
    {
        uint8_t flash_byte = *(volatile uint8_t *)(flash.curr_addr + (uint32_t)i);

        if (flash_byte != flash.page_buf[i])
        {
            /* Flash 中的内容与接收到的镜像不一致 */
            OTA_DebugSend("[OTA][Error]:Flash Verify Error ,Data Mismatch\r\n");
            return 1;
        }
    }

    OTA_FlashLock();

    /* 地址前移一页，偏移清零 */
    flash.page_offset = 0U;
    flash.curr_addr  += OTA_FLASH_PAGE_SIZE;

    return 0;
}

/**
 * @brief  手动模式下的页写入实现（适用于非均匀扇区）
 *         这里的“手动模式”主要体现在：
 *           - 通过 MiniOTA_FlashLayout 精确定位物理扇区
 *           - 对同一物理扇区仅擦除一次，随后多次按页写入
 *
 *         注意：当前实现并未引入“缓冲扇区”拷贝算法，只假定
 *               MiniOTA 管理的 APP 区域独占若干完整扇区。
 * @return 0: 成功, 1: 失败
 */
static int OTA_FlashWrite_Manual(void)
{
    OTA_FLASH_SECTOR_INFO info;

    /* 根据当前写入地址定位所在物理扇区 */
    if (OTA_FlashLocateSector(flash.curr_addr, &info) == OTA_FALSE)
    {
        return 1;
    }

    /* 如有必要，更新扇区信息 */
    if ((flash.sector_valid == 0U) ||
        (flash.sector_start != info.sector_start) ||
        (flash.sector_size  != info.sector_size))
    {
        flash.sector_start = info.sector_start;
        flash.sector_size  = info.sector_size;
        flash.sector_valid = 1U;

        /* 进入新扇区时执行一次扇区擦除 */
        if (OTA_FlashUnlock() != 0)
        {
            OTA_DebugSend("[OTA][Error]:Flash UnLock Failed\r\n");
            return 1;
        }

        if (OTA_ErasePage(flash.sector_start) != 0)
        {
            OTA_DebugSend("[OTA][Error]:Flash Erase Failed (sector)\r\n");
            if (OTA_FlashLock() != 0)
            {
                OTA_DebugSend("[OTA][Error]:Flash Lock Failed\r\n");
            }
            return 1;
        }
    }
    else
    {
        /* 扇区已擦除过，只需解锁以便编程 */
        if (OTA_FlashUnlock() != 0)
        {
            OTA_DebugSend("[OTA][Error]:Flash UnLock Failed\r\n");
            return 1;
        }
    }

    /* 在当前（已擦除的）扇区内按页写入 */
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i += 2)
    {
        uint16_t hw = (uint16_t)(flash.page_buf[i] | (flash.page_buf[i + 1] << 8));
        if (OTA_DrvProgramHalfword(flash.curr_addr + (uint32_t)i, hw) != 0)
        {
            if (OTA_FlashLock() != 0)
            {
                OTA_DebugSend("[OTA][Error]:Flash Lock Failed\r\n");
            }
            return 1;
        }
    }

    /* 写后读回校验（逐字节对比） */
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i++)
    {
        uint8_t flash_byte = *(volatile uint8_t *)(flash.curr_addr + (uint32_t)i);

        if (flash_byte != flash.page_buf[i])
        {
            /* Flash 中的内容与接收到的镜像不一致 */
            OTA_DebugSend("[OTA][Error]:Flash Verify Error ,Data Mismatch\r\n");
            return 1;
        }
    }

    OTA_FlashLock();

    /* 地址前移一页，偏移清零 */
    flash.page_offset = 0U;
    flash.curr_addr  += OTA_FLASH_PAGE_SIZE;

    return 0;
}

/**
 * @brief  将页缓冲区写入 Flash，包含擦除、编程、校验全流程
 * @return 0: 成功, 1: 失败
 */
int OTA_FlashWrite(void)
{
    /* 根据编译期配置选择自动/手动模式
     *  - OTA_FLASH_MODE_AUTO  : 适用于均匀页 Flash（如 F1），保持旧行为
     *  - OTA_FLASH_MODE_MANUAL: 适用于非均匀扇区（如 F411），按扇区粒度擦除 */
#if (OTA_FLASH_MODE == OTA_FLASH_MODE_MANUAL)
    return OTA_FlashWrite_Manual();
#else
    return OTA_FlashWrite_Auto();
#endif
}
