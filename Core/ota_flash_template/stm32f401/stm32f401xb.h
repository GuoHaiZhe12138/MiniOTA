#include "OtaInterface.h"
#include "OtaFlashIfoDef.h"

static const MiniOTA_SectorGroup F4_Physical_Max_Layout[] = {
    {4, 16384},   // 64KB
    {1, 65536},   // 128KB
    {1, 131072},  // 256KB
    {1, 131072},  // 384KB
    {1, 131072},  // 512KB
};
static const MiniOTA_FlashLayout F401_B_Layout = {
    .start_addr = OTA_FLASH_START_ADDRESS,
    .total_size = OTA_FLASH_SIZE,
    .is_uniform = 0,
    .group_count = (OTA_FLASH_SIZE <= 128 * 1024) ? 1
                  : (OTA_FLASH_SIZE <= 256 * 1024) ? 2
                  : (OTA_FLASH_SIZE <= 384 * 1024) ? 3 : 4,
    .groups = F4_Physical_Max_Layout
};
const MiniOTA_FlashLayout* MiniOTA_GetLayout(void) { return &F401_B_Layout; }