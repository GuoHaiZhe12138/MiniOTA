/**
 ******************************************************************************
 * @file    OtaCore.c
 * @author  MiniOTA Team
 * @brief   MiniOTA 核心逻辑实现
 *          包含 OTA 状态机、分区验证、IAP 流程控制及固件跳转
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
#include "OtaPort.h"
#include "OtaJump.h"
#include "OtaUtils.h"
#include "OtaFlash.h"

/**
 * @brief  验证 App 分区的完整性和有效性
 * @param  slot_addr: 分区起始地址
 * @return 1: 分区有效, 0: 分区无效
 */
static int Verify_App_Slot(uint32_t slot_addr) {
	
    OTA_APP_IMG_HEADER_E *header = (OTA_APP_IMG_HEADER_E*)slot_addr;

    // 1. 检查魔数
    if (header->magic != APP_MAGIC_NUM) {
        return 0; // 头部无效
    }

    // 2. 简单的范围检查
    if (header->img_size == 0 || header->img_size > OTA_APP_SLOT_SIZE) {
        return 0; // 大小异常
    }

    // 3. 计算固件体的 CRC (注意：固件体紧跟在 Header 后面)
    const uint8_t *bin_start = (const uint8_t *)(slot_addr + sizeof(OTA_APP_IMG_HEADER_E));
    uint16_t cal_crc = OTA_GetCrc16(bin_start, header->img_size);

    if (cal_crc != header->img_crc16) {
        return 0; // CRC 校验失败
    }

    return 1; // 验证通过
}

/**
 * @brief  将 Meta 信息保存到 Flash 状态区
 * @param  pMeta: 指向 Meta 结构体的指针
 */
static void OTA_SaveMeta(OTA_META_DATA_E *pMeta) {
    uint8_t flashPage[OTA_FLASH_PAGE_SIZE];
    OTA_MemSet(flashPage, 0xFF, OTA_FLASH_PAGE_SIZE);
    OTA_MemCopy(flashPage, (uint8_t *)pMeta, sizeof(OTA_META_DATA_E));
    OTA_FlashSetCurAddr(OTA_META_ADDR);
    OTA_FlashSetMirr(flashPage, OTA_FLASH_PAGE_SIZE);
    OTA_FlashWrite();
}

static void OTA_UpdateMeta(OTA_META_DATA_E *pMeta)
{
	uint8_t isSlotAValid = Verify_App_Slot(OTA_APP_A_ADDR);
	uint8_t isSlotBValid = Verify_App_Slot(OTA_APP_B_ADDR);
	
	/* state = unconfirmed */
	if(pMeta->slotAStatus == SLOT_STATE_UNCONFIRMED)
	{
		if(isSlotAValid)
		{
			pMeta->slotAStatus = SLOT_STATE_VALID;
		}
		else
		{
			pMeta->slotAStatus = SLOT_STATE_INVALID;
		}
	}
	
	if(pMeta->slotBStatus == SLOT_STATE_UNCONFIRMED)
	{
		if(isSlotBValid)
		{
			pMeta->slotBStatus = SLOT_STATE_VALID;
		}
		else
		{
			pMeta->slotBStatus = SLOT_STATE_INVALID;
		}
	}
	
	OTA_SaveMeta(pMeta);
}

static uint32_t OTA_GetJumpTar(OTA_META_DATA_E *pMeta)
{
	
	if(pMeta->active_slot == SLOT_A)
	{
		if((pMeta->slotAStatus == SLOT_STATE_UNCONFIRMED || pMeta->slotAStatus == SLOT_STATE_VALID))
		{
			if(Verify_App_Slot(OTA_APP_A_ADDR))
			{
				return OTA_APP_A_ADDR;
			}
			else if(pMeta->slotBStatus == SLOT_STATE_VALID)
			{
				return OTA_APP_B_ADDR;
			}
		}
	}
	
	if(pMeta->active_slot == SLOT_B)
	{
		if((pMeta->slotBStatus == SLOT_STATE_UNCONFIRMED || pMeta->slotBStatus == SLOT_STATE_VALID))
		{
			if(Verify_App_Slot(OTA_APP_B_ADDR))
			{
				return OTA_APP_B_ADDR;
			}
			else if(pMeta->slotAStatus == SLOT_STATE_VALID)
			{
				return OTA_APP_A_ADDR;
			}
		}
	}
	
	// 无可用固件
	return U32_INVALID;
}

/**
 * @brief  检查用户配置参数是否合理
 * @return OTA_OK: 配置有效, 其他: 错误码
 */
static uint8_t OTA_IsUserSetingsValid(void)
{
    uint32_t flash_end = OTA_FLASH_START_ADDRESS + OTA_FLASH_SIZE - 1;
    uint32_t app_max_size = OTA_APP_MAX_SIZE;

    /* 分配给MiniOTA的flash空间必须位于 Flash 内 */
    if (OTA_TOTAL_START_ADDRESS < OTA_FLASH_START_ADDRESS ||
        OTA_TOTAL_START_ADDRESS > flash_end)
    {
		OTA_DebugSend("[OTA][Error]:In OtaInterface - The flash space allocated to MiniOTA must be located within the Flash memory..\r\n");
        return OTA_ERR_FLASH_RANGE;
    }

    /* MiniOTA起始地址必须按 Flash 页对齐 */
    if ((OTA_TOTAL_START_ADDRESS % OTA_FLASH_PAGE_SIZE) != 0)
    {
		OTA_DebugSend("[OTA][Error]:In OtaInterface - MiniOTA's starting address must be aligned to the Flash page.\r\n");
        return OTA_ERR_ALIGN;
    }

    /* MiniOTA可用的flash空间必须大于一个 Flash 页 */
    if (app_max_size < OTA_FLASH_PAGE_SIZE)
    {
		OTA_DebugSend("[OTA][Error]:In OtaInterface - MiniOTA available flash storage space must be larger than one Flash page.\r\n");
        return OTA_ERR_SIZE;
    }

    return OTA_OK;
}

static OTA_REC_FLAG_STATE_E OTA_RunIAP(uint32_t addr)
{
	OTA_DebugSend("[OTA]:IAPing...\r\n");
	OTA_XmodemInit(addr);
	while(1)
	{
		/* 周期为1s得检查传输是否未开始 */
		if(OTA_GetXmodemState() == XM_WAIT_START && OTA_XmodemRevCompFlag() == REC_FLAG_IDLE)
		{
			OTA_SendByte(0x43);
			
			uint8_t i;
			for(i = 0; i < 100 ; i++ )
			{
				OTA_Delay1ms();
			}
		}
		
		if(OTA_XmodemRevCompFlag() == REC_FLAG_FINISH)
		{
			return REC_FLAG_FINISH;
		}
		else if(OTA_XmodemRevCompFlag() == REC_FLAG_INT)
		{
			return REC_FLAG_INT;
		}
	}
}

/**
 * @brief  MiniOTA 主入口函数
 *         执行 OTA 状态检查、分区验证、IAP 流程控制
 *         此函数应在 main 函数开始时调用
 */
void OTA_Run(void) {
    OTA_META_DATA_E meta;
    uint32_t target_addr;
	
	while(1)
	{
		// 检查用户参数设置合理性
		if(OTA_IsUserSetingsValid() != OTA_OK)
		{
			return;
		}
	
		// 读取 Meta 信息
		meta = *(OTA_META_DATA_E *)OTA_META_ADDR;
	
		// 检查 Meta 是否合法
		if (meta.magic != OTA_MAGIC_NUM) {
			/* Meta 无效：
				忽略OTA_ShouldEnterIap接口
				将Slot_A作为目标slot，进行IAP
			*/
			meta.magic = OTA_MAGIC_NUM;
			meta.seq_num = 0UL;
			meta.active_slot = SLOT_A;
			meta.slotAStatus = SLOT_STATE_EMPTY;
			meta.slotBStatus = SLOT_STATE_EMPTY;
			
			// 保存meta分区状态
			OTA_SaveMeta(&meta);
		}
		
		// 根据固件头更新meta信息
		OTA_UpdateMeta(&meta);
		
		// 如果用户需要刷入新的固件
		if(OTA_ShouldEnterIap())
		{
			// 发送IOM信息
			uint32_t tarAddr;
			// 首次写入flash特例
			if(meta.active_slot == SLOT_A && meta.slotAStatus == SLOT_STATE_EMPTY)
			{
				tarAddr = OTA_APP_A_ADDR;
			}
			else
			{
				tarAddr = (meta.active_slot == SLOT_A) ? OTA_APP_B_ADDR : OTA_APP_A_ADDR;
			}
			
			OTA_DebugSend("[OTA]:Selecting IAP... \r\n");
			OTA_DebugSend("[OTA]:Please set the IOM address to : \r\n");
			OTA_PrintHex32(tarAddr + sizeof(OTA_APP_IMG_HEADER_E));
			OTA_DebugSend("\r\n");
			
			if(OTA_RunIAP(tarAddr) == REC_FLAG_FINISH)
			{
				if(meta.active_slot == SLOT_A)
				{
					// 首次写入flash特例
					if(meta.slotAStatus == SLOT_STATE_EMPTY)
					{
						meta.slotAStatus = SLOT_STATE_UNCONFIRMED;
						meta.active_slot = SLOT_A;
					}
					else
					{
						meta.slotBStatus = SLOT_STATE_UNCONFIRMED;
						meta.active_slot = SLOT_B;
					}

				}
				else
				{
					meta.slotAStatus = SLOT_STATE_UNCONFIRMED;
					meta.active_slot = SLOT_A;
				}
				
				OTA_SaveMeta(&meta);
			
				OTA_JumpToApp(tarAddr + sizeof(OTA_APP_IMG_HEADER_E));
			}
			continue;
		}
		
		// 确定跳转目标地址
		target_addr = OTA_GetJumpTar(&meta);
		if(target_addr != U32_INVALID)
		{
			// 跳转到目标地址
			OTA_JumpToApp(target_addr + sizeof(OTA_APP_IMG_HEADER_E));
		}
	
		// 无可用固件，默认尝试使用slot_a接收新固件
		// 发送IOM信息
		OTA_DebugSend("[OTA]:Please set the IOM address to : \r\n");
		OTA_PrintHex32(OTA_APP_A_ADDR + sizeof(OTA_APP_IMG_HEADER_E));
		OTA_DebugSend("\r\n");
		
		if(OTA_RunIAP(OTA_APP_A_ADDR) == REC_FLAG_FINISH)
		{
			meta.active_slot = SLOT_A;
			meta.slotAStatus = SLOT_STATE_UNCONFIRMED;
			meta.slotBStatus = SLOT_STATE_EMPTY;
			OTA_SaveMeta(&meta);
		
			OTA_JumpToApp(OTA_APP_A_ADDR + sizeof(OTA_APP_IMG_HEADER_E));
		}
		
		return;
	}
}
