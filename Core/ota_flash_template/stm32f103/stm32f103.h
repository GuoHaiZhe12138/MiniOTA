#include "OtaFlashIfoDef.h"
#include "OtaInterface.h"
#include "OtaUtils.h"

/**
 * 对于 STM32F103，主 Flash 以固定页大小均匀分布（通常为 1KB/2KB 页擦除），
 * 在 MiniOTA 中可以直接视为“均匀页 Flash”：
 *  - （扇区/页）大小 = OTA_FLASH_PAGE_SIZE
 *  - （扇区/页）数量 = OTA_FLASH_SIZE / OTA_FLASH_PAGE_SIZE
 *
 * 因此这里将布局简化为单一均匀分组，便于自动模式直接按页计算。
 */
static const MiniOTA_SectorGroup F103Ser[] = {
    { OTA_FLASH_SIZE / OTA_FLASH_PAGE_SIZE, OTA_FLASH_PAGE_SIZE },
};

static const MiniOTA_FlashLayout F103_Low_Layout = {
    .start_addr = OTA_FLASH_START_ADDRESS,
    .total_size = OTA_FLASH_SIZE,
    .is_uniform = OTA_TRUE,
    .group_count = (uint32_t)(sizeof(F103Ser) / sizeof(F103Ser[0])),
    .groups = F103Ser,
};

static inline const MiniOTA_FlashLayout* MiniOTA_GetLayout(void)
{
    return &F103_Low_Layout;
}
