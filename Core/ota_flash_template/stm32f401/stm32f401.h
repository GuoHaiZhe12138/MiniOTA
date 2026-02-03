#include "OtaInterface.h"
#include "OtaFlashIfoDef.h"
#include "OtaUtils.h"

static const MiniOTA_SectorGroup F401Ser[] = {
    {4, 64 * OTA_1KB},   // 64KB
    {1, 64 * OTA_1KB},   // 128KB
    {1, 128 * OTA_1KB},  // 256KB
    {1, 128 * OTA_1KB},  // 384KB
    {1, 128 * OTA_1KB},  // 512KB
};
static const MiniOTA_FlashLayout F401_B_Layout = {
    .start_addr = OTA_FLASH_START_ADDRESS,
    .total_size = OTA_FLASH_SIZE,
    .is_uniform = OTA_TRUE,
    .group_count = (OTA_FLASH_SIZE <= 128 * OTA_1KB) ? 2
                  : (OTA_FLASH_SIZE <= 256 * OTA_1KB) ? 3
                  : (OTA_FLASH_SIZE <= 384 * OTA_1KB) ? 4 : 5,
    .groups = F401Ser
};
const MiniOTA_FlashLayout* MiniOTA_GetLayout(void)
{ 
    return &F401_B_Layout; 
}