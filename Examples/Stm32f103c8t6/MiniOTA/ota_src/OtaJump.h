/**
 ******************************************************************************
 * @file    OtaJump.h
 * @author  MiniOTA Team
 * @brief   应用跳转功能头文件
 *          定义跳转函数接口及函数指针类型
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */

#ifndef OTAJUMP_H
#define OTAJUMP_H

#include "OtaUtils.h"   /* 包含公共定义，包括 OTA_APP_CHECK_RESULT_E 和 pFunction */

/**
 * @brief  跳转到目标应用
 * @param  des_addr: 目标应用起始地址（向量表地址）
 */
void OTA_JumpToApp(uint32_t des_addr);

/**
 * @brief  检查应用向量表的 SP 和 PC 是否有效
 * @param  app_sp: 应用栈指针值
 * @param  app_pc: 应用程序计数器值
 * @return 检查结果枚举
 */
OTA_APP_CHECK_RESULT_E OTA_IsAppValid(uint32_t app_sp, uint32_t app_pc);
	
#endif
