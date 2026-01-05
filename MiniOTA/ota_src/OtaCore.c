/*******************************************************************************
  * @file           : 
  * @brief          : 
  ******************************************************************************
  * @attention
  *
  *
  *****************************************************************************/
#include "OtaInterface.h"
#include "OtaXmodem.h"
#include "OtaPort.h"
#include "OtaJump.h"
#include "OtaUtils.h"
#include "OtaFlash.h"

// 验证 App Slot 的完整性
// 返回 1 表示有效，0 表示无效
static int Verify_App_Slot(uint32_t slot_addr) {
	
    AppImgHeader_t *header = (AppImgHeader_t*)slot_addr;

    // 1. 检查魔数
    if (header->magic != APP_MAGIC_NUM) {
        return 0; // 头部无效
    }

    // 2. 简单的范围检查
    if (header->img_size == 0 || header->img_size > OTA_APP_SLOT_SIZE) {
        return 0; // 大小异常
    }

    // 3. 计算固件体的 CRC (注意：固件体紧跟在 Header 后面)
    const uint8_t *bin_start = (const uint8_t *)(slot_addr + sizeof(AppImgHeader_t));
    uint16_t cal_crc = XmodemCrc16(bin_start, header->img_size);

    if (cal_crc != header->img_crc16) {
        return 0; // CRC 校验失败
    }

    return 1; // 验证通过
}

// 封装：保存 Meta 信息到 Flash
static void OTA_SaveMeta(OtaMeta_t *pMeta) {
    uint8_t flashPage[OTA_FLASH_PAGE_SIZE];
    OTA_MemSet(flashPage, 0xFF, OTA_FLASH_PAGE_SIZE);
    OTA_MemCopy(flashPage, (uint8_t *)pMeta, sizeof(OtaMeta_t));
    Flash_SetCurAddr(OTA_META_ADDR);
    Flash_SetMirr(flashPage, OTA_FLASH_PAGE_SIZE);
    Flash_Write();
}

static void OTA_UpdateMeta(OtaMeta_t *pMeta)
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

static uint32_t OTA_GetJumpTar(OtaMeta_t *pMeta)
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

    /* MiniOTA用友的flash空间必须大于一个 Flash 页 */
    if (app_max_size < OTA_FLASH_PAGE_SIZE)
    {
		OTA_DebugSend("[OTA][Error]:In OtaInterface - MiniOTA and Yonyou's flash storage space must be larger than one Flash page.\r\n");
        return OTA_ERR_SIZE;
    }

    return OTA_OK;
}

static void OTA_RunIAP(uint32_t
 	addr)
{
	OTA_DebugSend("[OTA]:IAPing...\r\n");
	OTA_XmodemInit(addr);
	while(1)
	{
		/* 周期为1s得检查传输是否未开始 */
		if(OTA_GetXmodemHandle()->state == XM_WAIT_START && OTA_XmodemRevCompFlag() == REC_FLAG_IDLE)
		{
			OTA_SendByte(0x43);
			//OTA_DebugSend("[OTA]Send C\r\n");
			
			uint8_t i;
			for(i = 0; i < 100 ; i++ )
			{
				OTA_Delay1ms();
			}
		}
		
		if(OTA_XmodemRevCompFlag() == REC_FLAG_FINISH)
		{
			JumpToApp(addr + sizeof(AppImgHeader_t));
		}
		else if(OTA_XmodemRevCompFlag() == REC_FLAG_INT)
		{
			return;
		}
	}
}

// ---------------- 核心初始化函数 ----------------

void OTA_Run(void) {
    OtaMeta_t meta;
    uint32_t target_addr;
	
	while(1)
	{
		// 检查用户参数设置合理性
		if(OTA_IsUserSetingsValid() != OTA_OK)
		{
			return;
		}
	
		// 读取 Meta 信息
		meta = *(OtaMeta_t *)OTA_META_ADDR;
	
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
			uint32_t tarAddr = (meta.active_slot == SLOT_A) ? OTA_APP_B_ADDR : OTA_APP_A_ADDR;
			OTA_DebugSend("[OTA]:Selecting IAP... \r\n");
			OTA_DebugSend("[OTA]:Please set the IOM address to : \r\n");
			OTA_PrintHex32(tarAddr + sizeof(AppImgHeader_t));
			OTA_DebugSend("\r\n");
			
			meta.active_slot = (meta.active_slot == SLOT_A) ? SLOT_B : SLOT_A;
			meta.state = (meta.active_slot == SLOT_A) ? OTA_STATE_UPDATING_B : OTA_STATE_UPDATING_A;
			OTA_SaveMeta(&meta);
			OTA_RunIAP(tarAddr);
		}
		
		// 确定跳转目标地址
		target_addr = OTA_GetJumpTar(&meta);
		if(target_addr != U32_INVALID)
		{
			// 跳转到目标地址
			JumpToApp(target_addr + sizeof(AppImgHeader_t));
		}
	
		// 无可用固件，尝试接收新固件
		meta.active_slot = SLOT_A;
		meta.state = OTA_STATE_UPDATING_A;
		// 发送IOM信息
		OTA_DebugSend("[OTA]:Please set the IOM address to : \r\n");
		OTA_PrintHex32(OTA_APP_A_ADDR + sizeof(AppImgHeader_t));
		OTA_DebugSend("\r\n");
		
		OTA_SaveMeta(&meta);
		
		OTA_RunIAP(OTA_APP_A_ADDR);
	}
}
	


