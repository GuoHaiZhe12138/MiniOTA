# MiniOTA

## —适用于Cortex-M3内核的轻量级OTA框架

## 📖 项目简介

MiniOTA 是一个专为 Cortex-M3 内核微控制器设计的轻量级升级框架。它提供了完整的固件更新解决方案，包括串口通信、Xmodem协议传输、AB分区备份、失败回滚和完整性校验等功能。

该框架采用模块化设计，代码简洁高效，适合资源受限的嵌入式环境。经过严格的测试，已在实际项目中稳定运行。

## ✨ 核心特性

- **📶 字节流Xmodem IAP**：基于标准Xmodem协议，支持128/1024字节数据包，CRC16校验
- **🔄 AB分区双备份**：采用最久未使用（LRU）分区更新逻辑，确保系统可靠性
- **🛡️ 传输失败回滚**：完整的异常处理机制，传输中断时自动回滚到上一有效版本
- **🔒 分区内CRC校验**：固件完整性验证，防止数据损坏
- **⚡ 快速启动验证**：启动时自动验证分区有效性，选择最优固件启动
- **🔧 硬件抽象层**：提供统一的硬件接口，易于移植到不同MCU平台
- **📊 状态机驱动**：Xmodem传输采用表驱动状态机，代码清晰可靠

## 📁 项目结构

```
MiniOTA/
├── ota_interface/          # OTA接口层
│   ├── OtaInterface.h      # 全局配置与Flash布局定义
│   └── OtaPort.h           # 硬件抽象层接口定义
├── ota_src/                # OTA核心实现
│   ├── OtaCore.c           # OTA主状态机与逻辑控制
│   ├── OtaFlash.c          # Flash驱动抽象层
│   ├── OtaJump.c           # 应用跳转与向量表检查
│   ├── OtaPort.c           # 硬件平台适配实现（STM32F10x示例）
│   ├── OtaUtils.c          # 工具函数（CRC、内存操作等）
│   ├── OtaXmodem.c         # Xmodem协议状态机实现
│   └── 对应头文件
└── README.md               # 项目说明文档
```

## 🚀 快速开始

## Ⅰ.Bootloader集成

### 1. 集成到现有项目

将 MiniOTA 目录复制到您的项目中，并在编译配置中包含相关文件：

```c
// 在 main.c 中添加
#include "MiniOTA/ota_interface/OtaInterface.h"

// 在 main() 函数开始处调用
int main(void) {
    // 硬件初始化...
    
    // 启动 OTA 检查
    OTA_Run();
    
    while(1) {
        // 错误处理或等待用户干预
    }
}
```

### 2. 配置Flash布局

修改 `OtaInterface.h` 中的宏定义以匹配您的硬件：

```c
// 示例：stm32f103c8t6
/* Flash 总大小 */
#define OTA_FLASH_SIZE            0x8000      	/* 32KB */

/* Flash 物理起始地址 */
#define OTA_FLASH_START_ADDRESS   0x08000000UL

/* 分配给 MiniOTA 的起始地址 */
#define OTA_TOTAL_START_ADDRESS   0x08003000UL

/* Flash 页大小 */
#define OTA_FLASH_PAGE_SIZE       1024
```

### 3. 实现硬件抽象层

根据您的MCU平台实现 `OtaPort.h` 中定义的接口：

```c
// 在 OtaPort.c 中实现以下函数：
uint8_t OTA_ShouldEnterIap(void);          // 进入IAP模式判断
void OTA_ReceiveTask(uint8_t byte);        // 串口接收回调
void OTA_PeripheralsDeInit(void);          // 外设逆初始化
uint8_t OTA_FlashUnlock(void);             // Flash解锁
uint8_t OTA_FlashLock(void);               // Flash上锁
int OTA_ErasePage(uint32_t addr);          // 页擦除
int OTA_DrvProgramHalfword(uint32_t addr, uint16_t data); // 半字编程
void OTA_DrvRead(uint32_t addr, uint8_t *buf, uint16_t len); // 读取
uint8_t OTA_SendByte(uint8_t byte);        // 串口发送
uint8_t OTA_DebugSend(const char *data);   // 调试输出
void OTA_Delay1ms(void);                   // 延时函数
```

### 4. 在串口(或其他字节流)的中断回调函数中调用"OTA_ReceiveTask()"

```c
// 示例：stm32f103c8t6
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
		uint8_t ch = (uint8_t)USART_ReceiveData(USART1);
		
		OTA_ReceiveTask(ch);
    }
}
```

* MiniOTA暂不支持报文类型的传输协议，但已将其提上日程

## Ⅱ.生成并刷入APP固件

### 1.在您的ide或.ld链接脚本中设置IOM为MiniOTA的debug串口输出的地址

示例(开发环境keil5)：

①debug串口输出：[OTA]:Please set the IOM address to : **0x08005810**

②keil5 -> Options for Target -> Linker -> R/O Base = **0x08005810**

### 2.将您的bootloader工程编译为.bin文件

keil5 -> Options for Target -> User -> After build/Rebuild:[✔️]Run #1 fromelf --bin --output ".\Objects\project.bin" ".\Objects\project.axf"

* 请根据您的工程文件结构调整

### 3.使用MiniOTA中的exe工具为bin文件添加固件头

根据软件内提示进行即可

### 4.通过串口或其他字节流协议，使用XMODEM向mcu发送固件头即可



### 注意事项

- 确保Flash操作函数与硬件匹配
- 调整串口波特率以适应实际硬件
- 根据CPU频率调整延时函数
- 验证中断处理与现有系统的兼容性



## 📋 API参考

### 核心函数

| 函数 | 说明 |
|------|------|
| `void OTA_Run(void)` | OTA主入口函数，检查升级状态并决定跳转或进入IAP |

### 硬件抽象接口

| 函数 | 说明 |
|------|------|
| `uint8_t OTA_ShouldEnterIap(void)` | 判断是否应强制进入IAP模式（如按键检测） |
| `void OTA_PeripheralsDeInit(void)` | 跳转前清理外设状态 |
| `uint8_t OTA_FlashUnlock(void)` | Flash解锁 |
| `uint8_t OTA_FlashLock(void)` | Flash上锁 |
| `int OTA_ErasePage(uint32_t addr)` | 擦除指定页 |
| `int OTA_DrvProgramHalfword(uint32_t addr, uint16_t data)` | 编程半字数据 |
| `void OTA_DrvRead(uint32_t addr, uint8_t *buf, uint16_t len)` | 读取Flash数据 |

## ⚙️ 配置说明

### Flash分区布局

MiniOTA 使用三级存储结构：

```
+-------------------+
|     Bootloader    |  用户引导程序
+-------------------+
|   OTA Meta区域    |  状态信息（1page）
+-------------------+
|     APP A分区     |  应用程序A(含固件头)
+-------------------+
|     APP B分区     |  应用程序B(含固件头)
+-------------------+
```

### 分区状态管理

系统维护以下状态信息：

- **Meta区域**：存储当前激活分区、分区状态、序列号等
- **分区状态**：
  - `SLOT_STATE_EMPTY`：分区为空/已擦除
  - `SLOT_STATE_UNCONFIRMED`：新固件写入，未经验证
  - `SLOT_STATE_VALID`：已验证的有效固件
  - `SLOT_STATE_INVALID`：验证失败

### 自定义硬件适配示例

```c
// OtaPort.c 中的STM32F10x实现示例

uint8_t OTA_ShouldEnterIap(void)
{
	return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
}

void OTA_ReceiveTask(uint8_t byte)
{
	OTA_XmodemRevByte(byte);
}

uint8_t OTA_FlashUnlock(void)
{
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    return 0;
}

uint8_t OTA_FlashLock(void)
{
	FLASH_Lock();
    return 0;
}

int OTA_ErasePage(uint32_t addr)
{
    return (FLASH_ErasePage(addr) == FLASH_COMPLETE) ? 0 : 1;
}

int OTA_DrvProgramHalfword(uint32_t addr, uint16_t data)
{
    return (FLASH_ProgramHalfWord(addr, data) == FLASH_COMPLETE) ? 0 : 1;
}

void OTA_DrvRead(uint32_t addr, uint8_t *buf, uint16_t len)
{
	OTA_MemCopy(buf,(uint8_t *)addr, len);
}

uint8_t OTA_SendByte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, byte);
    return 0; 
}

uint8_t OTA_DebugSend(const char *data)
{
    if (!data) return 0;
    while (*data)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, *data++);
    }

    return 1;
}

void OTA_Delay1ms(void)
{
    volatile uint32_t count = 8000;
    while (count--)
    {
        __NOP();
    }
}
```

## 📊 性能指标

- **代码体积**：约 6-8KB（取决于配置和优化）
- **RAM使用**：约 2KB（包含Xmodem缓冲区）
- **Flash占用**：约 4KB（OTA框架本身）
- **支持最大固件**：取决于分区大小配置
- **CRC校验速度**：软件实现，适用于低速串口

## ✅ 支持的MCU内核

MiniOTA 框架基于标准 ARM Cortex-M 内核架构设计，支持所有具有 **VTOR（向量表偏移寄存器）** 的 Cortex-M 内核。只要目标 MCU 支持以下跳转流程，即可使用 MiniOTA：

### 支持的ARM Cortex-M内核

| 内核系列 | 支持状态 | 说明 |
|----------|----------|------|
| **Cortex-M3** | ✅ 完全支持 | 默认目标内核，完全兼容 |
| **Cortex-M4** | ✅ 完全支持 | 与 M3 完全兼容，具有 FPU 的型号也可使用 |
| **Cortex-M7** | ✅ 完全支持 | 与 M3 完全兼容，具有 Cache 和更高性能 |
| **Cortex-M23** | ✅ 完全支持 | 具有 VTOR 寄存器，完全兼容 |
| **Cortex-M33** | ✅ 完全支持 | 具有 VTOR 寄存器，完全兼容 |

> **注意**：Cortex-M0/M0+ 内核**不**在支持范围内，因为它们缺少 VTOR 寄存器，需要特殊的向量表处理。

### 跳转流程兼容性分析

MiniOTA 的核心跳转函数 `OTA_JumpToApp` 使用以下标准 Cortex-M 特性，这些特性在上述所有支持的内核中都存在：

```c
void OTA_JumpToApp(uint32_t des_addr) {
    // 1. 关中断 - 使用 __disable_irq()，所有 Cortex-M 内核通用
    __disable_irq();
    
    // 2. 关闭外设 - 用户实现的硬件相关函数
    OTA_PeripheralsDeInit();
    
    // 3. 关闭内核定时器 - 操作 SysTick 寄存器，所有 Cortex-M 内核通用
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    
    // 4. 读取应用向量表的 SP 和 Reset_Handler - 标准 ARM 向量表格式
    uint32_t app_sp    = *(uint32_t *)des_addr;
    uint32_t app_reset = *(uint32_t *)(des_addr + 4);
    
    // 5. 检查应用的有效性（可选）
    OTA_IsAppValid(app_sp, app_reset);
    
    // 6. 设置中断向量表到 App 区 - 使用 SCB->VTOR（Cortex-M3及以上内核支持）
    SCB->VTOR = des_addr;
    
    // 7. 设置主栈指针 - 使用 __set_MSP()，所有 Cortex-M 内核通用
    __set_MSP(app_sp);
    
    // 8. 跳转到应用复位处理函数 - 标准函数指针调用
    pFunction app_entry = (pFunction)app_reset;
    app_entry();
}
```

### 关键兼容性要求

1. **VTOR 寄存器**：必须存在并可用，用于重定位中断向量表
2. **标准向量表布局**：向量表前两个字必须为 SP 和 PC（Reset_Handler）
3. **SysTick 定时器**：必须存在且可访问
4. **标准中断控制**：支持 `__disable_irq()` 和 `__enable_irq()` 指令
5. **栈指针操作**：支持 `__set_MSP()` 指令

### 已验证的MCU平台

| MCU系列 | 具体型号 | 测试状态 | 使用内核 |
|---------|----------|----------|----------|
| STM32F1 | STM32F103C8T6 | ✅ 完全测试 | Cortex-M3 |

### 如何确认兼容性

要确认您的 MCU 是否兼容，请检查以下条件：

1. **ARM Cortex-M3/M4/M7/M23/M33 内核**：确认 MCU 使用这些内核之一
2. **VTOR 寄存器支持**：检查芯片参考手册，确认存在 SCB->VTOR 寄存器
3. **Flash 编程接口**：MCU 必须提供 Flash 擦写接口
4. **足够的 Flash 空间**：至少需要 12KB Flash 用于 OTA 框架和双分区

### 不兼容的情况

以下情况可能需要额外适配工作或无法直接使用：

- **Cortex-M0/M0+ 内核**：不支持 VTOR，需要重写向量表处理逻辑
- **非 ARM 架构 MCU**（如 RISC-V, 8051 等）：需要完全重写跳转和向量表处理逻辑
- **自定义中断控制器**：可能需要修改中断管理代码
- **特殊的 Flash 架构**：如需要特殊解锁序列或页大小不同

## 📅 更新日程

### 已实现功能

MiniOTA 当前版本已完整实现以下核心功能：

| 功能模块 | 状态 | 说明 |
|----------|------|------|
| **AB分区与回滚机制** | ✅ 已实现 | 双分区备份、LRU更新策略、传输失败自动回滚 |
| **基于Xmodem的字节流IAP** | ✅ 已实现 | 支持128/1024字节包、CRC16校验、表驱动状态机 |
| **APP分区CRC校验** | ✅ 已实现 | 固件完整性验证、启动时自动检查 |
| **APP固件头实现** | ✅ 已实现 | 包含魔数、版本、大小、CRC等元数据 |

### 未来计划

以下功能按计划开发顺序排列：

| 优先级 | 功能 | 目标 | 预计时间 |
|--------|------|------|----------|
| 1 | **提高用户接口宽容度** | 支持用户自定义Flash编程粒度（半字/全字）、更灵活的分区配置 | 2026 Q2 |
| 2 | **兼容报文传输** | 支持基于帧的传输协议（如Ymodem、自定义协议），提升传输效率 | 2026 Q3 |
| 3 | **更精简的镜像部署方式** | 减小OTA框架体积，优化内存占用，支持压缩传输 | 2026 Q4 |
| 4 | **更多安全策略** | 增加Flash写入回读校验、数字签名验证、防回滚攻击等 | 2027 Q1 |
| 5 | **断点续传** | 支持传输中断后从断点恢复，提升大文件传输可靠性 | 2027 Q2 |

**开发路线图说明：**
- **短期目标（2026上半年）**：提升框架的易用性和灵活性
- **中期目标（2026下半年）**：扩展传输协议支持，优化资源占用
- **长期目标（2027年）**：增强安全性，提升传输可靠性

欢迎社区贡献代码和提出建议，共同完善 MiniOTA 框架！

## 📄 许可证

本项目采用 MIT 许可证，详情请见 LICENSE 文件。

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request 来帮助改进 MiniOTA。

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📞 联系与支持

- 提交 Issue 报告问题
- 通过email联系我们：ghz2985715538@gmail.com
- 通过 Pull Request 贡献代码
- 欢迎 Star 项目支持开发

---

**MiniOTA** - 让嵌入式设备升级更简单、更可靠！
