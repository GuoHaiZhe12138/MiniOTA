#include "OtaFlashIfoDef.h"
#include "OtaInterface.h"
#include "OtaUtils.h"

/**
 * STM32F411CEU6 Flash 扇区布局（主存 512KB）：
 *  - Sector 0~3 : 4 x 16KB  =  64KB
 *  - Sector 4   : 1 x 64KB  =  64KB
 *  - Sector 5~7 : 3 x 128KB = 384KB
 *  共 8 个扇区，擦除粒度为“扇区”，非均匀。
 *
 * 其中：
 *  - 当 OTA_FLASH_SIZE <= 256KB 时，可视为使用前 3 组（到 256KB 结束）；
 *  - 否则使用全部 5 组（完整 512KB）。
 */
static const MiniOTA_SectorGroup F411Ser[] = {
    {4, 16 * OTA_1KB},   // 64KB
    {1, 64 * OTA_1KB},   // 128KB
    {1, 128 * OTA_1KB},  // 256KB
    {1, 128 * OTA_1KB},  // 384KB
    {1, 128 * OTA_1KB},  // 512KB
};

static const MiniOTA_FlashLayout F411_B_Layout = {
    .start_addr = OTA_FLASH_START_ADDRESS,
    .total_size = OTA_FLASH_SIZE,
    .is_uniform = OTA_FALSE,
    .group_count = (OTA_FLASH_SIZE <= 256 * OTA_1KB) ? 3 : 5,
    .groups = F411Ser
};

static inline const MiniOTA_FlashLayout* MiniOTA_GetLayout(void)
{
    return &F411_B_Layout;
}
