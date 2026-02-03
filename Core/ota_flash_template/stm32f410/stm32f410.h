#include "OtaInterface.h"
#include "OtaFlashIfoDef.h"
#include "OtaUtils.h"

static const MiniOTA_SectorGroup F410Ser[] = {
    {4, 16*1024},   // 64KB
    {1, 64*1024},   // 128KB
};
static const MiniOTA_FlashLayout F410_B_Layout = {
    .start_addr = OTA_FLASH_START_ADDRESS,
    .total_size = OTA_FLASH_SIZE,
    .is_uniform = OTA_TRUE,
    .group_count = (OTA_FLASH_SIZE <= 64 * OTA_1KB) ? 1 : 2
    .groups = F410Ser
};
const MiniOTA_FlashLayout* MiniOTA_GetLayout(void)
{ 
    return &F410_B_Layout;
}