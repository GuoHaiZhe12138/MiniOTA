#include "OtaFlashIfoDef.h"
#include "OtaInterface.h"

static const MiniOTA_SectorGroup F103_Low_Groups[] = {
    {16, 1024},  // 16KB
    {16, 1024},  // 32KB
    {32, 1024},  // 64KB
    {64, 1024},  // 128KB
    {64, 2048}, // 256KB
    {64, 2048}, // 384KB
    {64, 2048}, // 512KB
    {128, 2048}, // 768KB
    {128, 2048}  // 1MB
};

static const MiniOTA_FlashLayout F103_Low_Layout = {
    .start_addr = OTA_FLASH_START_ADDRESS,
    .total_size = OTA_FLASH_SIZE,
    .is_uniform = 1,
    .group_count = (OTA_FLASH_SIZE <= 16 * 1024)  ? 1 :
                   (OTA_FLASH_SIZE <= 32 * 1024)  ? 2 :
                   (OTA_FLASH_SIZE <= 64 * 1024)  ? 3 :
                   (OTA_FLASH_SIZE <= 128 * 1024) ? 4 :
                   (OTA_FLASH_SIZE <= 256 * 1024) ? 5 :
                   (OTA_FLASH_SIZE <= 384 * 1024) ? 6 :
                   (OTA_FLASH_SIZE <= 512 * 1024) ? 7 :
                   (OTA_FLASH_SIZE <= 768 * 1024) ? 8 : 9,
    .groups = F103_Low_Groups
};

const MiniOTA_FlashLayout* MiniOTA_GetLayout(void) {
    return &F103_Low_Layout;
}