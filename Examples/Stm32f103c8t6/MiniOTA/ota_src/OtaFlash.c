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
 * @brief  获取布局指针
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
    for (uint32_t i = 0U; i < layout->group_count; i++)
    {
        const MiniOTA_SectorGroup *g = &layout->groups[i];
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

#if (OTA_FLASH_FORMAT == 1) && defined(OTA_BUFFER_SECTOR_START)
/**
 * @brief  将指定扇区整扇区拷贝到缓冲扇区（非均匀Flash模式 + 配置了缓冲扇区时使用）
 * @param  sector_start  源扇区起始地址
 * @param  sector_size   扇区大小（字节）
 * @return 0: 成功, 1: 失败
 */
static int OTA_FlashCopySectorToBuffer(uint32_t sector_start, uint32_t sector_size)
{
    uint32_t buf_start = OTA_BUFFER_SECTOR_START;
    uint32_t off;

    if (OTA_FlashUnlock() != 0)
    {
        return 1;
    }

    // 1. 擦除缓冲扇区，以便写入拷贝数据
    if (OTA_ErasePage(buf_start) != 0)
    {
        OTA_FlashLock();
        OTA_DebugSend("[OTA][Error]: Buffer sector erase failed.\r\n");
        return 1;
    }

    // 2. 按页拷贝 (每页 1KB)
    for (off = 0U; off < sector_size; off += OTA_FLASH_PAGE_SIZE)
    {
        // 2.1. 从原扇区读取 1KB
        OTA_DrvRead(sector_start + off, flash.page_buf, OTA_FLASH_PAGE_SIZE);
        for (uint32_t i = 0U; i < OTA_FLASH_PAGE_SIZE; i += 2U)
        {
            // 2.2. 写入缓冲扇区
            uint16_t hw = (uint16_t)(flash.page_buf[i] | (flash.page_buf[i + 1U] << 8));
            if (OTA_DrvProgramHalfword(buf_start + off + i, hw) != 0)
            {
                OTA_FlashLock();
                OTA_DebugSend("[OTA][Error]: Buffer sector program failed.\r\n");
                return 1;
            }
        }
    }

    OTA_FlashLock();
    return 0;
}

/**
 * @brief  从缓冲扇区将一页读入页缓冲（用于下一轮的 R-M-W）
 */
static void OTA_FlashLoadPageFromBuffer(uint32_t addr_in_sector_offset)
{
    uint32_t buf_addr = OTA_BUFFER_SECTOR_START + addr_in_sector_offset;
    OTA_DrvRead(buf_addr, flash.page_buf, OTA_FLASH_PAGE_SIZE);
}
#endif

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
    // 1. 初始化句柄
    flash.curr_addr    = addr;
    flash.page_offset  = 0;
    flash.sector_start = 0;
    flash.sector_size  = 0;
    flash.sector_valid = 0;

    //(void)OTA_FlashGetLayoutInternal();

#if (OTA_FLASH_FORMAT == 1) && defined(OTA_BUFFER_SECTOR_START)
    {
        OTA_FLASH_SECTOR_INFO info;
        // 2. 定位当前扇区
        if (OTA_FlashLocateSector(addr, &info) == OTA_TRUE)
        {
            flash.sector_start = info.sector_start;
            flash.sector_size  = info.sector_size;
            flash.sector_valid = 1U;
            // 3. 拷贝整个扇区到缓冲区
            if (OTA_FlashCopySectorToBuffer(info.sector_start, info.sector_size) != 0)
            {
                OTA_DebugSend("[OTA][Error]: Copy sector to buffer failed at init.\r\n");
            }
            // 4. 擦除原扇区
            else if (OTA_FlashUnlock() == 0)
            {
                (void)OTA_ErasePage(info.sector_start);
                OTA_FlashLock();
            }
            // 5. 从缓冲区预读当前页，供 Xmodem 打补丁 
            {
                uint32_t page_off = ((addr - info.sector_start) / OTA_FLASH_PAGE_SIZE) * OTA_FLASH_PAGE_SIZE;
                OTA_FlashLoadPageFromBuffer(page_off);
            }
        }
    }
#else
    /* 均匀Flash模式或未配置缓冲：直接从 Flash 预读当前页 */
    OTA_DrvRead(addr, flash.page_buf, OTA_FLASH_PAGE_SIZE);
#endif
}

/**
 * @brief  均匀Flash模式下的页写入实现（适用于均匀页 Flash）
 * @return 0: 成功, 1: 失败
 */
static int OTA_FlashWrite_Uniform(void)
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
 * @brief  非均匀Flash模式下的页写入实现（适用于非均匀扇区）
 *         若定义了 OTA_BUFFER_SECTOR_START：进入新扇区时先整扇区拷贝到缓冲、再擦除原扇区；
 *         按页写回原扇区后，从缓冲预读下一页供 Xmodem 打补丁。
 * @return 0: 成功, 1: 失败
 */
static int OTA_FlashWrite_Not_Uniform(void)
{
    OTA_FLASH_SECTOR_INFO info;

    // 1. 定位当前地址所在扇区
    if (OTA_FlashLocateSector(flash.curr_addr, &info) == OTA_FALSE)
    {
        return 1;
    }

    // 2. 判断是否进入新扇区
    if ((flash.sector_valid == 0U) ||
        (flash.sector_start != info.sector_start) ||
        (flash.sector_size  != info.sector_size))
    {
        flash.sector_start = info.sector_start;
        flash.sector_size  = info.sector_size;
        flash.sector_valid = 1U;

#if (OTA_FLASH_FORMAT == 1) && defined(OTA_BUFFER_SECTOR_START)
        // 3. 进入新扇区：先拷贝到缓冲再擦除原扇区，避免未覆盖区域丢失
        if (OTA_FlashCopySectorToBuffer(info.sector_start, info.sector_size) != 0)
        {
            OTA_DebugSend("[OTA][Error]: Copy sector to buffer failed.\r\n");
            return 1;
        }
#endif

        // 4. 解锁flash并擦除新扇区
        if (OTA_FlashUnlock() != 0)
        {
            OTA_DebugSend("[OTA][Error]:Flash UnLock Failed\r\n");
            return 1;
        }

        if (OTA_ErasePage(flash.sector_start) != 0)
        {
            OTA_DebugSend("[OTA][Error]:Flash Erase Failed (sector)\r\n");
            OTA_FlashLock();
            return 1;
        }
    }
    /*
    else
    {
        if (OTA_FlashUnlock() != 0)
        {
            OTA_DebugSend("[OTA][Error]:Flash UnLock Failed\r\n");
            return 1;
        }
    }
    */

    // 5. 写入当前页 (1KB)
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i += 2)
    {
        uint16_t hw = (uint16_t)(flash.page_buf[i] | (flash.page_buf[i + 1] << 8));
        if (OTA_DrvProgramHalfword(flash.curr_addr + (uint32_t)i, hw) != 0)
        {
            OTA_FlashLock();
            return 1;
        }
    }

    // 6. 校验写入数据
    for (int i = 0; i < OTA_FLASH_PAGE_SIZE; i++)
    {
        uint8_t flash_byte = *(volatile uint8_t *)(flash.curr_addr + (uint32_t)i);
        if (flash_byte != flash.page_buf[i])
        {
            OTA_DebugSend("[OTA][Error]:Flash Verify Error ,Data Mismatch\r\n");
            OTA_FlashLock();
            return 1;
        }
    }

    OTA_FlashLock();

    // 7. 移动到下一页
    flash.page_offset = 0U;
    flash.curr_addr  += OTA_FLASH_PAGE_SIZE;

#if (OTA_FLASH_FORMAT == 1) && defined(OTA_BUFFER_SECTOR_START)
    // 8. 预读下一页 (如果还在同一扇区)，供下一轮 Xmodem 打补丁
    if (flash.curr_addr < flash.sector_start + flash.sector_size)
    {
        uint32_t next_page_off = (flash.curr_addr - flash.sector_start);
        next_page_off = (next_page_off / OTA_FLASH_PAGE_SIZE) * OTA_FLASH_PAGE_SIZE;
        OTA_FlashLoadPageFromBuffer(next_page_off);
    }
#endif

    return 0;
}

/**
 * @brief  将页缓冲区写入 Flash，包含擦除、编程、校验全流程
 * @return 0: 成功, 1: 失败
 */
int OTA_FlashWrite(void)
{
    /* 根据编译期配置选择均匀/非均匀Flash
     *  - OTA_FLASH_FORMAT_UNIFORM: 适用于均匀页 Flash（如 F1），保持旧行为
     *  - OTA_FLASH_FORMAT_NOT_UNIFORM: 适用于非均匀扇区（如 F411），按扇区粒度擦除 */
#if (OTA_FLASH_FORMAT == 1)
    return OTA_FlashWrite_Not_Uniform();
#else
    return OTA_FlashWrite_Uniform();
#endif
}
