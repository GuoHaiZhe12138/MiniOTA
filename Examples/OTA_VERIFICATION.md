# MiniOTA 示例工程板级验证说明

## 编译

- **Stm32f103c8t6**：使用现有工程（Keil 等）编译，确保包含路径含 `Core`、`Core/ota_src`，以及本示例的 `MiniOTA/ota_interface`、`MiniOTA/ota_src`，并包含布局模板路径（如 `Core/ota_flash_template/stm32f103`）。
- **Stm32f411ceu6**：同上，布局模板路径为 `Core/ota_flash_template/stm32f411`，并定义 `OTA_BUFFER_SECTOR_START`、`OTA_BUFFER_SECTOR_SIZE`（已在示例 `OtaInterface.h` 中配置）。

## F1 实板验证（自动模式）

1. 上电无 IAP 请求：不拉高 PB11，应从当前有效 Slot 启动或提示设置 IOM 地址。
2. 强制 IAP：拉高 PB11 后复位，应打印 IOM 地址，用 Xmodem 发送固件，完成后跳转至新固件。
3. 多轮升级：再次拉高 PB11，写入另一 Slot，重启后应从新 Slot 启动；确认 Meta、Slot A/B 状态与预期一致。

## F4 实板验证（手动模式 + 缓冲扇区）

1. 分区与缓冲：确认 `OtaInterface.h` 中 `OTA_TOTAL_START_ADDRESS`、`OTA_APP_*`、`OTA_BUFFER_SECTOR_START`（如 0x08060000）与硬件及链接脚本一致。
2. 上电无 IAP：不拉高 PB11，应从有效 Slot 启动或进入等待 IAP。
3. 强制 IAP：拉高 PB11 后复位，按提示设置 IOM 地址，Xmodem 发送固件；观察是否按扇区拷贝→擦除→按页写回，无异常复位或校验错误。
4. 多轮升级与回退：多次 IAP 切换 Slot A/B，重启后确认当前运行 Slot 与 Meta 一致，并可回退到另一 Slot。

## 常见问题

- **布局不匹配**：若出现 “Flash layout mismatch”，检查 `ota_flash_template` 中 `total_size`、`start_addr` 与 `OtaInterface.h` 中 `OTA_FLASH_SIZE`、`OTA_FLASH_START_ADDRESS` 一致。
- **F4 擦除失败**：确认 `OTA_ErasePage(addr)` 中地址与 F411 扇区划分一致，且未对缓冲扇区误擦除。
- **IOM 地址**：烧录/生成 APP 时使用的 ROM 起始地址应为 `OTA_APP_A_ADDR + sizeof(OTA_APP_IMG_HEADER_E)`（或对应 Slot），与 BootLoader 打印一致。
