/*******************************************************************************
  * @file           : OtaJump.c
  * @brief          : Xmodem Table-Driven Refactored
  ******************************************************************************/

#include "stdint.h"
#include "stm32f10x.h"

#include "OtaInterface.h"
#include "OtaUtils.h"
#include "OtaPort.h"


AppCheckResult_t OTA_IsAppValid(uint32_t app_sp, uint32_t app_pc);

void JumpToApp(uint32_t des_addr)
{
    uint32_t app_sp;
    uint32_t app_reset;
    pFunction app_entry;
	
	// 关中断
    __disable_irq();
	// 关外设
	OTA_PeripheralsDeInit();
	// 关内核定时器
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
	
	// 计算sp指针和中断向量表地址
    app_sp    = *(uint32_t *)des_addr;
    app_reset = *(uint32_t *)(des_addr + 4);
	
	OTA_IsAppValid(app_sp, app_reset);
	
	// 设置中断向量表到 App 区
    SCB->VTOR = des_addr;
	// 设置MSP指针
    __set_MSP(app_sp);
	// 跳转
    app_entry = (pFunction)app_reset;
    app_entry();
}

/* 检查 App 向量表 SP/PC 是否有效 */
AppCheckResult_t OTA_IsAppValid(uint32_t app_sp, uint32_t app_pc)
{

    /* 1. 检查 SP 是否在 SRAM 范围内 */
    if (app_sp < OTA_FLASH_START_ADDRESS || app_sp > (OTA_FLASH_START_ADDRESS + OTA_FLASH_SIZE))
    {
        return APP_CHECK_SP_INVALID;
    }

    /* 2. 检查 PC 是否为 Thumb 地址 */
    if ((app_pc & 0x1) == 0)
    {
        return APP_CHECK_PC_INVALID;
    }

    return APP_CHECK_OK;
}

