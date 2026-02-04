#include "OtaInterface.h"
#include "OtaFlashIfoDef.h"
#include "OtaUtils.h"

static const MiniOTA_SectorGroup F405_415_Ser[] = {
    {4, 16*1024},   // 64KB
    {1, 64*1024},   // 128KB
    {1, 128*1024},  // 256KB
    {1, 128*1024},  // 384KB
    {1, 128*1024},  // 512KB
    {1, 128*1024},  // 640KB
    {1, 128*1024},  // 768KB
    {1, 128*1024},  // 896KB
    {1, 128*1024},  // 1024KB
};
static const MiniOTA_FlashLayout F405_B_Layout = {
    .start_addr = OTA_FLASH_START_ADDRESS,
    .total_size = OTA_FLASH_SIZE,
    .is_uniform = OTA_TRUE,
    .group_count = (OTA_FLASH_SIZE <= 512 * OTA_1KB) ? 5 : 9
    .groups = F405_415_Ser
};
const MiniOTA_FlashLayout* MiniOTA_GetLayout(void)
{ 
    return &F405_B_Layout;
}