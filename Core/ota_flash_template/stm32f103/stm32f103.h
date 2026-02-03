#include "OtaFlashIfoDef.h"
#include "OtaInterface.h"
#include "OtaUtils.h"

static const MiniOTA_SectorGroup F103Ser[9] = {
    {16, OTA_1KB},  // 16KB
    {16, OTA_1KB},  // 32KB
    {32, OTA_1KB},  // 64KB
    {64, OTA_1KB},  // 128KB
    {64, 2 * OTA_1KB},  // 256KB
    {64, 2 * OTA_1KB},  // 384KB
    {64, 2 * OTA_1KB},  // 512KB
    {128, 2 * OTA_1KB}, // 768KB
    {128, 2 * OTA_1KB}  // 1MB
};

static const MiniOTA_FlashLayout F103_Low_Layout = {
    .start_addr = OTA_FLASH_START_ADDRESS,
    .total_size = OTA_FLASH_SIZE,
    .is_uniform = OTA_FALSE,
    .group_count = (OTA_FLASH_SIZE <= 16 * OTA_1KB)  ? 1 :
                   (OTA_FLASH_SIZE <= 32 * OTA_1KB)  ? 2 :
                   (OTA_FLASH_SIZE <= 64 * OTA_1KB)  ? 3 :
                   (OTA_FLASH_SIZE <= 128 * OTA_1KB) ? 4 :
                   (OTA_FLASH_SIZE <= 256 * OTA_1KB) ? 5 :
                   (OTA_FLASH_SIZE <= 384 * OTA_1KB) ? 6 :
                   (OTA_FLASH_SIZE <= 512 * OTA_1KB) ? 7 :
                   (OTA_FLASH_SIZE <= 768 * OTA_1KB) ? 8 : 9,
    .groups = F103Ser
};

const MiniOTA_FlashLayout* MiniOTA_GetLayout(void) 
{
    return &F103_Low_Layout;
}