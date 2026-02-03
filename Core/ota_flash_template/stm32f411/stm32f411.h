#include "OtaInterface.h"
#include "OtaFlashIfoDef.h"
#include "OtaUtils.h"

static const MiniOTA_SectorGroup F411Ser[] = {
    {4, 16*1024},   // 64KB
    {1, 64*1024},   // 128KB
    {1, 128*1024},  // 256KB
    {1, 128*1024},  // 384KB
    {1, 128*1024},  // 512KB
};
static const MiniOTA_FlashLayout F411_B_Layout = {
    .start_addr = OTA_FLASH_START_ADDRESS,
    .total_size = OTA_FLASH_SIZE,
    .is_uniform = OTA_TRUE,
    .group_count = (OTA_FLASH_SIZE <= 256 * OTA_1KB) ? 3 : 5
    .groups = F411Ser
};
const MiniOTA_FlashLayout* MiniOTA_GetLayout(void)
{ 
    return &F411_B_Layout;
}