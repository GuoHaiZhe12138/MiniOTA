/**
 ******************************************************************************
 * @file    OtaFlashIfoDef.h
 * @author  MiniOTA Team
 * @brief   应用 Flash 分区信息定义
 *          包含 Flash 分区布局及扇区组定义
 ******************************************************************************
 * @attention
 * 
 * Copyright (c) 2026 MiniOTA.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef __MINI_OTA_LAYOUT_H__
#define __MINI_OTA_LAYOUT_H__

#include "OtaInterface.h"
#include "OtaUtils.h"

typedef struct {
    uint32_t count;
    uint32_t size;
} MiniOTA_SectorGroup;

typedef struct {
    uint32_t start_addr;    // 起始地址
    uint32_t total_size;    // 总大小
    OTA_BOOL  is_uniform;    // 是否均匀分布
    uint32_t group_count;   // 扇区组数量
    const MiniOTA_SectorGroup *groups;
} MiniOTA_FlashLayout;

// 所有模板必须实现这个函数
const MiniOTA_FlashLayout* MiniOTA_GetLayout(void);

#endif