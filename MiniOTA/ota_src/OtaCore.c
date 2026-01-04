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

void OTA_RunIAP(uint32_t addr);
void OTA_SelectIAP(void);

// 验证 App Slot 的完整性
// 返回 1 表示有效，0 表示无效
int Verify_App_Slot(uint32_t slot_addr) {
	
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

// ---------------- 核心初始化函数 ----------------

void OTA_Run(void) {
    OtaMeta_t meta;
    uint32_t target_addr;
    uint32_t backup_addr;

    // 1. 读取 Meta 信息
    meta = *(OtaMeta_t *)OTA_META_ADDR;

    // 2. 检查 Meta 是否合法
    if (meta.magic != OTA_MAGIC_NUM) {
        // Meta 无效，默认初始化为 Slot A,并开始iap
		meta.magic = OTA_MAGIC_NUM;
		meta.seq_num = 1UL;
        meta.active_slot = SLOT_A;
        meta.state = OTA_STATE_UPDATING;
		
		// 保存meta分区状态
		uint8_t flashMetaPage[OTA_FLASH_PAGE_SIZE];
		OTA_MemSet(flashMetaPage, 0XFF, OTA_FLASH_PAGE_SIZE);
		OTA_MemCopy(flashMetaPage, (uint8_t *)(&meta), OTA_FLASH_PAGE_SIZE);
		Flash_SetCurAddr(OTA_META_ADDR);
		Flash_SetMirr(flashMetaPage, OTA_FLASH_PAGE_SIZE);
		Flash_Write();
    }
	
	// 如果需要刷入固件
	if(OTA_ShouldEnterIap())
	{
		OTA_DebugSend("[OTA]:Selecting IAP... \r\n");
		OTA_SelectIAP();
	}

    // 3. 确定目标地址和备份地址
    if (meta.active_slot == SLOT_A) {
        target_addr = OTA_APP_A_ADDR;
        backup_addr = OTA_APP_B_ADDR;
    } else {
        target_addr = OTA_APP_B_ADDR;
        backup_addr = OTA_APP_A_ADDR;
    }

    // 4. 验证目标 App
    if (Verify_App_Slot(target_addr)) {
        // 目标有效，准备跳转
        // 可以将状态改为 IDLE 并写回 Meta (Commit)
        if (meta.state == OTA_STATE_UPDATING) {
            meta.state = OTA_STATE_IDLE;
            meta.seq_num++; 
			
			uint8_t flashMetaPage[OTA_FLASH_PAGE_SIZE];
			OTA_MemSet(flashMetaPage, 0XFF, OTA_FLASH_PAGE_SIZE);
			OTA_MemCopy(flashMetaPage, (uint8_t *)(&meta), OTA_FLASH_PAGE_SIZE);
            Flash_SetCurAddr(OTA_META_ADDR);
			Flash_SetMirr(flashMetaPage, OTA_FLASH_PAGE_SIZE);
			Flash_Write();
        }
        
        JumpToApp(target_addr + sizeof(AppImgHeader_t)); // 跳转到 Header 之后的向量表
    } 
    else {
        // 5. 目标无效 (更新失败或固件损坏)，尝试回滚
        if (Verify_App_Slot(backup_addr)) {
            // 备份分区是好的，回滚
            
            // 更新 Meta 指向备份分区
            meta.active_slot = (meta.active_slot == SLOT_A) ? SLOT_B : SLOT_A;
            meta.state = OTA_STATE_ERROR; // 标记曾发生错误
            
			uint8_t flashMetaPage[OTA_FLASH_PAGE_SIZE];
			OTA_MemSet(flashMetaPage, 0XFF, OTA_FLASH_PAGE_SIZE);
			OTA_MemCopy(flashMetaPage, (uint8_t *)(&meta), OTA_FLASH_PAGE_SIZE);
            Flash_SetCurAddr(OTA_META_ADDR);
			Flash_SetMirr(flashMetaPage, OTA_FLASH_PAGE_SIZE);
			Flash_Write();
            
            JumpToApp(backup_addr + sizeof(AppImgHeader_t));
        }
    }	
    // 6. 如果走到这里，说明 Target 和 Backup 都无法启动
    // 死循环接收新的固件
	meta.active_slot == SLOT_A? OTA_RunIAP(OTA_APP_A_ADDR) : OTA_RunIAP(OTA_APP_B_ADDR);
}
	
void OTA_SelectIAP(void)
{
	// 读取meta区信息
	OtaMeta_t meta = *(OtaMeta_t *)OTA_META_ADDR;
	// Meta 无效，默认初始化为 Slot A,并开始iap
	if (meta.magic != OTA_MAGIC_NUM) {
        OTA_DebugSend("[OTA][Error]:The META is corrupted. Please call the OTA_Run() function to restore the META\r\n");
    }
	
	OTA_DebugSend("[OTA][Prompt]:Please Set App IOM to  ");
	if(Verify_App_Slot(OTA_APP_A_ADDR))
	{
		OTA_PrintHex32(OTA_APP_B_ADDR + sizeof(AppImgHeader_t));
		OTA_DebugSend("  \r\n");
		OTA_RunIAP(OTA_APP_B_ADDR);
	}
	else
	{
		OTA_PrintHex32(OTA_APP_A_ADDR + sizeof(AppImgHeader_t));
		OTA_DebugSend("  \r\n");
		OTA_RunIAP(OTA_APP_A_ADDR);
	}
}

void OTA_RunIAP(uint32_t addr)
{
	OTA_DebugSend("[OTA]:IAPing...\r\n");
	OTA_XmodemInit(addr);
	while(1)
	{
		/* 周期为1s得检查传输是否未开始 */
		if(OTA_GetXmodemHandle()->state == XM_WAIT_START && OTA_XmodemRevCompFlag() == 0)
		{
			OTA_SendByte(0x43);
			//OTA_DebugSend("[OTA]Send C\r\n");
			
			uint8_t i;
			for(i = 0; i < 100 ; i++ )
			{
				OTA_Delay1ms();
			}
		}
		
		if(OTA_XmodemRevCompFlag() == 2)
		{
			JumpToApp(addr + sizeof(AppImgHeader_t));
		}
	}
}


uint8_t OTA_IsUserSetingsValid(void)
{
    uint32_t flash_end = OTA_FLASH_START_ADDRESS + OTA_FLASH_SIZE - 1;
    uint32_t app_max_size = OTA_APP_MAX_SIZE;

    /* 1. App 区间必须完全位于 Flash 内 */
    if (OTA_TOTAL_START_ADDRESS < OTA_FLASH_START_ADDRESS ||
        OTA_TOTAL_START_ADDRESS > flash_end)
    {
        return OTA_ERR_FLASH_RANGE;
    }

    /* 2. App 起始地址必须按 Flash 页对齐 */
    if ((OTA_TOTAL_START_ADDRESS % OTA_FLASH_PAGE_SIZE) != 0)
    {
        return OTA_ERR_ALIGN;
    }

    /* 3. App 区域最大空间必须 >= 一个 Flash 页 */
    if (app_max_size < OTA_FLASH_PAGE_SIZE)
    {
        return OTA_ERR_SIZE;
    }

    return OTA_OK;
}
