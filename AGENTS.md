# AGENTS.md

STM32 通用 Boot 固件升级项目。核心架构:`Mod/` 平台无关纯 C 模板 + `boot_update_test/app-brp/Mod/` STM32F103 移植。`boot_update_test/` 下有 `app-brp/`(Mod 框架 APP 工程:上电调 `boot_update()` 自动升级 BOOT)、`app/`(普通 APP 演示)与 `boot/`(**移植前参考,保留不编译**),共享 `boot_update_test/Drivers/`。所有源文件为 UTF-8 无 BOM。

## 验证命令

- `python tool/check_mod_consistency.py` — 11 项一致性检查,修改后必须全部 PASS(退出码 0)。检查对象:`Mod/`(模板)vs `boot_update_test/app-brp/Mod/`(移植版)。
- `python tool/check_compile.py` — gcc/clang 语法检查。**没有编译器时打印 `WARNING: No C compiler found` 并仍以退出码 0 "PASS" 退出**,勿据此认定编译通过,需确认输出无 WARNING。
- 无单元测试框架。Keil 工程:`boot_update_test/app-brp/MDK-ARM/app-brp.uvprojx`(APP@0x08010000,烧录后上电自动升级 BOOT)与 `boot_update_test/app/MDK-ARM/app.uvprojx`(普通 APP demo)。

## 架构与关键约束

- `Mod/` 禁止引入 HAL/平台特定代码;`boot_update_test/app-brp/Mod/` 含 STM32 HAL 实现。`app.c` / `app.h` / `bsp_flash.c` / `bsp_flash.h` 在两边必须一致,由检查脚本强制。
- **以下差异是预期的,不要"修复"**:`api_flash_port.c` 允许不同(模板空壳 vs 平台实现)、`bsp_flash_read` 实现允许不同、移植版多出 `bsp_flash_test()`。
- 修改 `Mod/` 核心函数后,先同步到 `boot_update_test/app-brp/Mod/`,再跑一致性检查。
- **app-brp 工程编译 `Mod` 的 `app.c` + `api_flash_port.c` + `bsp_flash.c` + `BOOT1.c`**;`BOOT1.c` 由 `tool/bin2c/bin2c.py` 从 boot 镜像(如 `boot/` 构建的 BOOT.bin)生成,提供 `bin_buf`/`bin_buf_elem_len`(生成时必须选 32bit 小端,与 `HAL_BAND_WIDTH=4` 一致)。app 上电调用 `boot_update()` 即擦除 BOOT 区→写 `bin_buf`→校验。
- **`boot_update_test/boot/` 为移植前参考**:其 `Mod/` 已移除(工程无法编译),保留作对照,勿修改。
- 宏约定:`HAL_BAND_WIDTH`(1/2/4/8)决定 `FlashBandwidthType_t` typedef;`HAL_MIN_WRITE_BAYE` 是最小写入粒度,且 `api_flash_write` 单次必须写满这么多字节(地址/长度计算须乘 `sizeof`,历史上有越界 bug);`ENABLE_GOTO_FLAG` 条件编译包裹 GOTO_FLAG 配置;`APP_START_ADDR`/`BOOT_START_ADDR` 未定义会 `#error`。
- **Flash 分区(STM32F103ZE,512KB,页 2KB)**:Boot `0x08000000`-`0x0800FFFF`(64KB),APP 起始 `0x08010000`(448KB)。定义在 `boot_update_test/app-brp/Mod/App/app.h`(`APP_START_ADDR`/`BOOT_START_ADDR`)与 `boot_update_test/app-brp/Mod/External_Interfaces/api_flash_port.h`(`HAL_FLASH_BASE_ADDR`=FLASH_BASE、`HAL_FLASH_END_ADDR`=FLASH_BANK1_END、`HAL_FLASH_PAGE_SIZE`=2048、`HAL_MIN_WRITE_BAYE`=4、`HAL_BAND_WIDTH`=4)。
- **简易上位机** `tool/ymodem_upload.py`(需 pyserial)为 YMODEM 发送端,用于 `app/` 工程经 Boot 串口升级;使用:`python tool/ymodem_upload.py -p COM5 -f app_update.bin`。
- 固件数组契约(`app.c` 中 extern,由 `tool/bin2c/bin2c.py` 生成):`extern const FlashBandwidthType_t bin_buf[]` + `extern const unsigned long long bin_buf_elem_len`,类型不可改动(一致性检查会拦截)。
- `tool/bin2c/bin2c.py` 是**交互式**脚本(选 8/16/32bit 与端序),自动取目录下第一个 `.bin` 输出 `BOOT.c`。

## 仓库约定

- 编码必须保持 UTF-8 无 BOM(历史有 GBK / UTF-8-BOM 中文乱码事故)。
- 提交信息用中文、带 conventional 前缀(feat/fix/refactor/docs/chore/sync/tool)。
- `.gitignore` 白名单只保留 `*.uvprojx` / `*.h` / `*.c` / `*.ioc`;Keil 构建产物(`*.axf` `*.bin` `*.hex` `*.uvoptx` `*.__i` `*._ia` 等)已取消跟踪,勿重新加入。
