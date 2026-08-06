# AGENTS.md

STM32 通用 Boot 固件升级项目。核心架构:`Mod/` 平台无关纯 C 模板 + `boot_update_test/boot/Mod/` STM32F103 移植(BootLoader 工程)。`boot_update_test/` 下分为 `boot/`(串口 YMODEM IAP BootLoader)与 `app/`(APP 演示工程)两个独立 Keil 工程,共享 `boot_update_test/Drivers/`。所有源文件为 UTF-8 无 BOM。

## 验证命令

- `python tool/check_mod_consistency.py` — 11 项一致性检查,修改后必须全部 PASS(退出码 0)。检查对象:`Mod/`(模板)vs `boot_update_test/boot/Mod/`(移植版)。
- `python tool/check_compile.py` — gcc/clang 语法检查。**没有编译器时打印 `WARNING: No C compiler found` 并仍以退出码 0 "PASS" 退出**,勿据此认定编译通过,需确认输出无 WARNING。
- 无单元测试框架。Keil 工程:`boot_update_test/boot/MDK-ARM/boot.uvprojx`(Boot,烧录 0x08000000)与 `boot_update_test/app/MDK-ARM/app.uvprojx`(APP,0x08010000,构建后输出 `app_update.bin` 供 YMODEM 上传)。

## 架构与关键约束

- `Mod/` 禁止引入 HAL/平台特定代码;`boot_update_test/boot/Mod/` 含 STM32 HAL 实现。`app.c` / `app.h` / `bsp_flash.c` / `bsp_flash.h` 在两边必须一致,由检查脚本强制。
- **以下差异是预期的,不要"修复"**:`api_flash_port.c` 允许不同(模板空壳 vs 平台实现)、`bsp_flash_read` 实现允许不同、`boot_update_test` 版多出 `bsp_flash_test()`。
- 修改 `Mod/` 核心函数后,先同步到 `boot_update_test/boot/Mod/`,再跑一致性检查。
- **boot 工程只编译 `Mod` 的 `api_flash_port.c` + `bsp_flash.c`**;`App/app.c` 与 `BOOT1.c` 仅为满足一致性检查而保留在 `boot/Mod/` 磁盘上,未加入 boot 工程构建(它们依赖旧 `bin_buf` 符号,勿加回,否则链接失败)。boot 的 IAP 写入逻辑在 `boot/Ymodem/ymodem.c` + `Core/Src/main.c` 中。
- 宏约定:`HAL_BAND_WIDTH`(1/2/4/8)决定 `FlashBandwidthType_t` typedef;`HAL_MIN_WRITE_BAYE` 是最小写入粒度(地址/长度计算须乘 `sizeof`,历史上有越界 bug);`ENABLE_GOTO_FLAG` 条件编译包裹 GOTO_FLAG 配置;`APP_START_ADDR`/`BOOT_START_ADDR` 未定义会 `#error`。
- **Flash 分区(STM32F103ZE,512KB,页 2KB)**:Boot `0x08000000`-`0x0800FFFF`(64KB),APP 起始 `0x08010000`(448KB)。定义在 `boot_update_test/boot/Mod/App/app.h`(`APP_START_ADDR`)与 `boot_update_test/boot/Mod/External_Interfaces/api_flash_port.h`(`HAL_FLASH_PAGE_SIZE`=2048、`HAL_MIN_WRITE_BAYE`=4、`HAL_BAND_WIDTH`=4)。
- **YMODEM 模块**(`boot/Ymodem/ymodem.c/.h`)为平台无关协议实现,通过回调挂接 UART/HAL 与 `bsp_flash_*`(见 boot `main.c`);`ymodem_receive()` 仅在起始与 EOT 后发 `'C'`,数据阶段只等待,CRC16 为 XMODEM 变体。
- **简易上位机** `tool/ymodem_upload.py`(需 pyserial)为 YMODEM 发送端,与 boot 配对测试过(含 >256 块序号回绕);使用:`python tool/ymodem_upload.py -p COM5 -f app_update.bin`。
- 固件数组契约(`app.c` 中 extern,由 `tool/bin2c/bin2c.py` 生成):`extern const FlashBandwidthType_t bin_buf[]` + `extern const unsigned long long bin_buf_elem_len`,类型不可改动(一致性检查会拦截)。
- `tool/bin2c/bin2c.py` 是**交互式**脚本(选 8/16/32bit 与端序),自动取目录下第一个 `.bin` 输出 `BOOT.c`。

## 仓库约定

- 编码必须保持 UTF-8 无 BOM(历史有 GBK / UTF-8-BOM 中文乱码事故)。
- 提交信息用中文、带 conventional 前缀(feat/fix/refactor/docs/chore/sync/tool)。
- `.gitignore` 白名单只保留 `*.uvprojx` / `*.h` / `*.c` / `*.ioc`;Keil 构建产物(`*.axf` `*.bin` `*.hex` `*.uvoptx` `*.__i` `*._ia` 等)已取消跟踪,勿重新加入。
