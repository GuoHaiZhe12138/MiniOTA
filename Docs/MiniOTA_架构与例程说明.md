# MiniOTA 架构与例程说明

本文档梳理 MiniOTA 核心源码结构、F103/F411 两个例程的改动与差异，并给出测试验证步骤及可能遇到的问题。

---

## 一、Core 源码结构概览

MiniOTA 的 **Core** 层与芯片无关，只依赖「接口约定」和「Flash 布局抽象」，具体硬件由例程侧的 **OtaPort** 与 **OtaInterface** 提供。

### 1.1 目录与职责

| 路径 | 职责 |
|------|------|
| `Core/ota_interface/` | 模板级接口定义（占位，实际由例程覆盖） |
| `Core/ota_interface/OtaInterface.h` | 全局配置模板：CMSIS 设备头占位、Flash 宏、布局头占位、`OTA_Run` / `OTA_ReceiveTask` 声明 |
| `Core/ota_interface/OtaPort.h` | 硬件适配接口：Flash 解锁/擦除/编程/读、串口发送、接收缓冲、延时、是否进入 IAP |
| `Core/ota_src/OtaFlashIfoDef.h` | Flash 分区抽象：`MiniOTA_SectorGroup`、`MiniOTA_FlashLayout`、`MiniOTA_GetLayout()` 声明 |
| `Core/ota_src/OtaUtils.h` | 工具与公共定义：魔数、Flash 模式宏、Meta/APP 地址宏、`OTA_META_DATA_E`、`OTA_APP_IMG_HEADER_E`、CRC/内存工具 |
| `Core/ota_src/OtaUtils.c` | 工具函数实现 |
| `Core/ota_src/OtaCore.c` | **核心逻辑**：配置校验、Meta 读写、Slot 验证、跳转目标选择、IAP 流程、`OTA_Run()` 主循环 |
| `Core/ota_src/OtaFlash.c` | Flash 抽象层：布局解析、扇区定位、**自动/手动**两种写入模式（页缓冲、缓冲扇区 R-M-W） |
| `Core/ota_src/OtaFlash.h` | Flash 句柄与页缓冲 API |
| `Core/ota_src/OtaXmodem.c/h` | Xmodem 协议收发与状态机 |
| `Core/ota_src/OtaJump.c/h` | 应用跳转：关中断、清外设、VTOR、MSP、跳转到 App |
| `Core/ota_src/OtaPort.c` | **不在此目录**：由各例程在 `Examples/xxx/MiniOTA/ota_src/OtaPort.c` 实现 |
| `Core/ota_flash_template/` | 各芯片 Flash 布局模板（stm32f103、stm32f411 等），供例程拷贝或引用 |

### 1.2 关键抽象：Flash 模式与布局

- **OTA_FLASH_MODE_AUTO**：页大小固定、按页擦除（如 STM32F1）。Core 按「当前地址 → 页对齐 → 擦除 → 写页缓冲」一气呵成。
- **OTA_FLASH_MODE_MANUAL**：非均匀扇区（如 F411）。Core 通过 `MiniOTA_GetLayout()` 做「地址 → 扇区」映射，大扇区采用「整扇区拷贝到缓冲扇区 → 在缓冲扇区 R-M-W → 再写回」避免误擦邻区。

布局由**例程侧**的 `stm32f103.h` / `stm32f411.h` 提供：定义 `MiniOTA_FlashLayout` 与 `static inline MiniOTA_GetLayout(void)`，供 Core 的 `OtaFlash.c` / `OtaCore.c` 使用。

### 1.3 用户可见入口

- **OTA_Run()**：在 `main()` 开头调用，完成配置检查、Meta 读取与更新、是否进入 IAP、选择跳转目标或进入 Xmodem 接收。
- **OTA_ReceiveTask(uint8_t byte)**：在串口 RX 中断里调用，把收到的字节交给 Xmodem。

用户只需维护**例程内的** `OtaInterface.h`（及可选布局头），无需改 Core 源码。

---

## 二、例程与 Core 的衔接方式

每个例程都自带一份 **MiniOTA** 子目录，其中：

- **ota_interface/**  
  - `OtaInterface.h`：包含本机 CMSIS 设备头、本机 Flash 宏、`#include "stm32f103.h"` 或 `"stm32f411.h"`。  
  - `stm32f103.h` / `stm32f411.h`：本机 Flash 布局 + `static inline MiniOTA_GetLayout()`，可自 Core 的 `ota_flash_template` 拷贝后微调。  
  - `OtaPort.h`：与 Core 一致，仅声明。
- **ota_src/**  
  - 与 Core 同名的 `.c/.h`（OtaCore、OtaFlash、OtaUtils、OtaXmodem、OtaJump、OtaFlashIfoDef）一般**直接使用或从 Core 拷贝**，保证与 Core 逻辑一致。  
  - **OtaPort.c**：本机独有，实现 `OtaPort.h` 中声明的 Flash、串口、GPIO、延时等。

工程里通过包含路径把「例程的 `ota_interface` + `ota_src`」作为 MiniOTA 实现；Core 仅作参考与模板，不强制参与编译。

---

## 三、F103 与 F411 例程改动与差异

### 3.1 F103（Stm32f103c8t6）— 均匀页、自动模式

- **Flash**：32KB 主存，1KB 页，均匀擦除。  
- **OtaInterface.h**  
  - `OTA_FLASH_SIZE = 0x8000`，`OTA_FLASH_START_ADDRESS = 0x08000000`，`OTA_FLASH_PAGE_SIZE = 1024`。  
  - `OTA_FLASH_MODE = OTA_FLASH_MODE_AUTO`。  
  - `OTA_TOTAL_START_ADDRESS = 0x08003000`（BootLoader 约占 12KB）。  
  - `#include "stm32f103.h"` 提供均匀布局与 `MiniOTA_GetLayout()`。  
- **stm32f103.h**  
  - 单组均匀扇区：`count = OTA_FLASH_SIZE/OTA_FLASH_PAGE_SIZE`，`size = OTA_FLASH_PAGE_SIZE`。  
  - `MiniOTA_GetLayout()` 为 `static inline`，避免多重定义。  
- **OtaPort.c**  
  - `OTA_ErasePage` → `FLASH_ErasePage(addr)`（F1 标准库）。  
  - `OTA_FlashUnlock` / `OTA_FlashLock`、`OTA_DrvProgramHalfword`、`OTA_DrvRead`、`OTA_SendByte`（USART1）、`OTA_DebugSend`（USART2）、`OTA_Delay1ms`、`OTA_ShouldEnterIap`（PB11）。

### 3.2 F411（Stm32f411ceu6）— 非均匀扇区、手动模式

- **Flash**：512KB 主存，扇区 0~3 各 16KB、扇区 4 为 64KB、扇区 5~7 各 128KB。  
- **OtaInterface.h**  
  - `OTA_FLASH_SIZE = 512*1024`，`OTA_FLASH_PAGE_SIZE = 1024`。  
  - `OTA_FLASH_MODE = OTA_FLASH_MODE_MANUAL`。  
  - `OTA_TOTAL_START_ADDRESS = 0x08004000`（Sector0 仅 BootLoader，16KB）。  
  - `OTA_META_SIZE = 16*1024`（Meta 独占 Sector1），`OTA_APP_REGION_SIZE = 352*1024`（Sector2~6，Sector7 作缓冲）。  
  - `OTA_BUFFER_SECTOR_START = 0x08060000`，`OTA_BUFFER_SECTOR_SIZE = 128*1024`。  
  - `#include "stm32f411.h"` 提供非均匀布局与 `MiniOTA_GetLayout()`。  
- **stm32f411.h**  
  - 多组扇区：16K×4、64K×1、128K×3（与 F411 手册一致）。  
  - `MiniOTA_GetLayout()` 为 `static inline`。  
- **OtaPort.c**  
  - 含 `stm32f4xx_flash.h`，实现「地址 → 扇区索引」与「扇区索引 → FLASH_Sector_x」。  
  - `OTA_ErasePage(addr)` → 按地址擦除对应扇区，**禁止擦除 Sector0**（保护 BootLoader）。  
  - F4 专用 Flash 标志位清除、`OTA_Delay1ms` 等与 F1 类似但 API 不同。

### 3.3 差异小结

| 项目 | F103 | F411 |
|------|------|------|
| Flash 模式 | AUTO（均匀页） | MANUAL（非均匀扇区） |
| 布局头 | stm32f103.h | stm32f411.h |
| Meta 区 | 默认 1 页（1KB） | 16KB（整扇区） |
| 缓冲扇区 | 无 | Sector7 专用于 R-M-W |
| 擦除 API | FLASH_ErasePage | FLASH_EraseSector，且禁止 Sector0 |
| 工程/启动 | 例程 main 在根目录 | 例程 main 在 User/main.c |

---

## 四、测试验证步骤

### 4.1 编译

1. 使用 Keil 打开对应例程：  
   - F103：`Examples/Stm32f103c8t6/project.uvprojx`  
   - F411：`Examples/Stm32f411ceu6/project.uvprojx`  
2. 选择目标，执行 **Rebuild All**，确认 0 Error（若有 `OTA_FlashWrite_Manual` / `OTA_FlashWrite_Auto` 未使用告警可忽略，与自动/手动二选一有关）。

### 4.2 烧录 BootLoader 与 首版 APP

1. 将生成的 **BootLoader**（例程 axf/hex）烧录到芯片：  
   - F103：通常从 `0x08000000` 开始；  
   - F411：从 `0x08000000` 开始，且 BootLoader 应不超过 16KB（Sector0）。  
2. **APP 工程**需要与 BootLoader 约定一致：  
   - **IROM1 Start** 必须与 MiniOTA 分配的 Slot 起始 + 固件头长度一致。  
   - 进入 IAP 时，BootLoader 会在串口打印类似：  
     `[OTA]:Please set the IOM address to :` 和 **一个十六进制地址**。  
   - 该地址 = `OTA_APP_A_ADDR + sizeof(OTA_APP_IMG_HEADER_E)` 或当前目标 Slot 的「固件体」起始（即向量表地址）。  
3. 在 APP 工程中把 **IROM1 Start** 设为该打印的地址，编译生成 APP 的 bin。  
4. APP 的 bin 需带 **MiniOTA 固件头**：前 16 字节为 `OTA_APP_IMG_HEADER_E`（magic=0x424C4150、img_size、version、img_crc16 等），后面紧跟固件体。若工具链不自动加头，需用脚本或工具在 bin 前拼接头并重算 CRC。

### 4.3 上电与 IAP 流程

1. **不进入 IAP**：PB11 为低，上电后 BootLoader 应读取 Meta，校验当前 Slot 固件，若有效则跳转到 APP。  
2. **进入 IAP**：将 PB11 拉高后复位，BootLoader 应打印 IOM 地址并等待 Xmodem。用串口工具（如 Xmodem 协议）向 **USART1** 发送带头的 APP bin；传输完成后 BootLoader 写 Meta、跳转到新 APP。  
3. **多轮升级与 A/B 切换**：再次拉高 PB11 进入 IAP，发送新固件，应写入另一 Slot（A/B 交替），并更新 Meta 的 active_slot；重启后应从新 Slot 启动。

### 4.4 建议验证项

- [ ] BootLoader 单独上电，无 APP 时打印 IOM 地址并能完成一次 Xmodem 接收。  
- [ ] 首次烧录带正确 IOM 与固件头的 APP 后，不按 IAP 键上电可正常进入 APP。  
- [ ] 按 IAP 键上电，发送第二版 APP，能写入另一 Slot 并重启后从新 Slot 启动。  
- [ ] F411：确认不会擦除 Sector0（BootLoader 区）且 Meta/APP 仅在配置的扇区内。

---

## 五、可能遇到的问题

| 现象 | 可能原因 | 建议处理 |
|------|----------|----------|
| 链接报错 `MiniOTA_GetLayout multiply defined` | 布局头里用非 inline 函数定义 | 确保 `MiniOTA_GetLayout()` 在布局头中为 `static inline`，且仅在布局头中定义一次。 |
| 上电后不跳转 APP，或 HardFault | APP 的 IROM1 Start 与 BootLoader 给出的 IOM 不一致 | 按串口打印的 IOM 地址设置 APP 的 IROM1 Start，并保证 APP 带 16 字节固件头。 |
| APP 校验失败（Slot 被标 INVALID） | 固件头 CRC 与固件体不一致、或 magic 错误 | 检查打包工具是否按 `OTA_APP_IMG_HEADER_E` 写 magic/img_size/img_crc16，且 CRC 覆盖固件体。 |
| F411 擦除或写入异常 | 擦到 Sector0、或缓冲扇区未配置 | 确认 `OtaPort.c` 禁止擦 Sector0；确认 `OTA_BUFFER_SECTOR_*` 与 `stm32f411.h` 布局一致。 |
| 编译告警 `OTA_FlashWrite_Manual` / `OTA_FlashWrite_Auto` 未引用 | 当前工程只用了 AUTO 或只用了 MANUAL | 属预期，可忽略或在该 .c 内局部屏蔽未使用函数告警。 |
| Meta 区/APP 区越界或布局不一致 | OtaInterface.h 与布局头/实际 Flash 不符 | 核对 `OTA_TOTAL_START_ADDRESS`、`OTA_META_SIZE`、`OTA_APP_REGION_SIZE` 与芯片手册及 `MiniOTA_GetLayout()` 描述一致。 |

---

## 六、文档与版本

- 本文档基于当前 **Core** 与 **Examples/Stm32f103c8t6、Stm32f411ceu6** 的代码整理。  
- 若后续调整 Flash 分区或增加芯片，请同步更新各例程的 `OtaInterface.h` 与对应 `stm32fxxx.h` 布局头，并复测上述步骤。
