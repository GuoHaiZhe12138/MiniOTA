
#ifndef OTAUTILS_H
#define OTAUTILS_H

// ---------------- 配置区域 ----------------
#define OTA_MAGIC_NUM       0x5A5A0001  // 用于识别 Meta 数据有效性
#define APP_MAGIC_NUM       0x424C4150  // "BLAP" - BootLoader APp

// 用户设置参数合理性检查结果定义
typedef enum
{
    OTA_OK = 0,               /* 参数合法 */
    OTA_ERR_FLASH_RANGE,      /* App 区间超出 Flash 范围 */
    OTA_ERR_ALIGN,            /* App 起始地址未对齐 Flash 页 */
    OTA_ERR_SIZE,             /* App 区域大小不合法 */
} OTA_Status_t;

/* Flash内APP代码检查结果枚举 */
typedef enum
{
    APP_CHECK_OK = 0,        /* App 合法 */
    APP_CHECK_SP_INVALID,    /* 初始堆栈指针无效 */
    APP_CHECK_PC_INVALID     /* 复位向量非法（非 Thumb 地址） */
} AppCheckResult_t;

// OTA 状态枚举
typedef enum {
    OTA_STATE_IDLE = 0,     // 正常状态
    OTA_STATE_UPDATING,     // 正在更新
    OTA_STATE_ERROR         // 发生错误
} OtaState_e;

// 目标插槽枚举
typedef enum {
    SLOT_A = 0,
    SLOT_B = 1
} ActiveSlot_e;

// ---------------- 结构体定义 ----------------

//#pragma pack(push, 1)

// 固件头部结构 (放在每个 Slot 的头部)
typedef struct {
    uint32_t magic;         // 固定魔数，用于快速校验头部是否存在
    uint32_t img_size;      // 固件实际大小 (不含头)
    uint32_t version;       // 版本号 (用于比较新旧)
	uint16_t img_crc16;     // 固件数据的 CRC32 校验值
    uint8_t  reserved[2];   // 以此保证结构体16byte对齐
} AppImgHeader_t;

// Meta 分区结构 (全局状态)
typedef struct {
    uint32_t     magic;       // Meta 数据有效性魔数
    uint32_t     seq_num;     // 序列号 (每次更新+1，用于简单的寿命均衡或版本追踪)
    ActiveSlot_e active_slot; // 当前应启动的插槽 (A 或 B)
    OtaState_e   state;       // 当前 OTA 状态
	uint8_t  reserved[7];     // 保证结构体32byte对齐
} OtaMeta_t;

// App函数指针定义
typedef void (*pFunction)(void);

void U8ArryCopy(uint8_t *dst, const uint8_t *src, uint32_t len);
uint16_t XmodemCrc16(const uint8_t *buf, uint32_t len);
// 固件合理性判断

void OTA_MemSet(uint8_t *dst, uint8_t val, uint32_t len);
void OTA_MemCopy(uint8_t *dst, const uint8_t *src, uint32_t len);
void OTA_PrintHex32(uint32_t value);

//#pragma pack(pop)
#endif
