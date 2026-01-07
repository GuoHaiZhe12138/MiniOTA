/**
 ******************************************************************************
 * @file    OtaJump.c
 * @author  MiniOTA Team
 * @brief   应用跳转功能实现
 *          包含应用向量表检查、外设清理及安全跳转逻辑
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */

#include "stdint.h"
#include "stm32f10x.h"

#include "OtaInterface.h"
#include "OtaUtils.h"
#include "OtaPort.h"

/**
 * @brief  检查应用向量表的 SP 和 PC 是否有效
 * @param  app_sp: 应用栈指针值
 * @param  app_pc: 应用程序计数器值
 * @return 检查结果枚举
 */
OTA_APP_CHECK_RESULT_E OTA_IsAppValid(uint32_t app_sp, uint32_t app_pc);

/**
 * @brief  跳转到目标应用
 * @param  des_addr: 目标应用起始地址（向量表地址）
 */
void OTA_JumpToApp(uint32_t des_addr)
{
    uint32_t app_sp;
    uint32_t app_reset;
    pFunction app_entry;
	
	// 1. 关中断
    __disable_irq();
	// 2. 关闭外设
	OTA_PeripheralsDeInit();
	// 3. 关闭内核定时器
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
	
	// 4. 读取应用向量表的 SP 和 Reset_Handler
    app_sp    = *(uint32_t *)des_addr;
    app_reset = *(uint32_t *)(des_addr + 4);
	
	// 5. 检查应用的有效性（可选，用于调试）
	OTA_IsAppValid(app_sp, app_reset);
	
	// 6. 设置中断向量表到 App 区
    SCB->VTOR = des_addr;
	// 7. 设置主栈指针
    __set_MSP(app_sp);
	// 8. 跳转到应用复位处理函数
    app_entry = (pFunction)app_reset;
    app_entry();
}

/* 检查 App 向量表 SP/PC 是否有效 */
OTA_APP_CHECK_RESULT_E OTA_IsAppValid(uint32_t app_sp, uint32_t app_pc)
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
