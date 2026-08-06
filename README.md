# STM32 Boot Firmware Upgrade System (通用APP升级Boot固件系统)

[![License](https://img.shields.io/badge/License-Proprietary-blue.svg)](LICENSE)

一种基于嵌入式软件的通用APP升级Boot固件的方法及系统。提供平台无关的Boot固件升级模板，支持跨MCU平台移植，已在STM32F103上完成参考实现。

---

## 目录

- [概述](#概述)
- [特性](#特性)
- [架构](#架构)
- [目录结构](#目录结构)
- [快速开始](#快速开始)
- [移植指南](#移植指南)
- [工具链](#工具链)
- [验证](#验证)
- [已知限制](#已知限制)
- [许可](#许可)

---

## 概述

本项目实现了一套**通用嵌入式Boot固件升级方案**，包含：

- 平台无关的 **Boot升级模板库**（`Mod/`）
- 基于 **STM32F103** 的参考实现（`boot_update_test/`）
- 配套的 **bin2c工具** 和 **自动化验证脚本**

核心流程：擦除Flash → 写入新固件 → 校验 → 设置跳转标志 → 系统复位，完成APP升级。

---

## 特性

### 核心功能
| 特性 | 说明 |
|------|------|
| 🔄 **固件升级** | 擦除→写入→校验→跳转标志→复位的完整升级流程 |
| ✅ **写入校验** | `bsp_flash_write()` 后自动调用 `bsp_cmp_flash()` 验证数据一致性 |
| 🚩 **GOTO_FLAG机制** | 条件编译控制（`ENABLE_GOTO_FLAG`），通过Flash标志位触发Boot跳转到APP |
| 📏 **带宽抽象** | `HAL_BAND_WIDTH` 配置支持 8/16/32/64-bit 带宽，适配不同MCU |
| 🔒 **地址越界保护** | 所有Flash操作均包含地址有效性检查 |

### 可移植性
- **零依赖模板**：`Mod/` 目录为纯C模板，不含任何HAL或平台特定代码
- **接口抽象层**：通过 `api_flash_port.h` / `.c` 隔离平台差异
- **参考实现**：`boot_update_test/app-brp/Mod/` 提供完整的STM32F103移植示例

### 配套工具
- **bin2c.py**：将二进制固件文件自动转为C数组，支持 8/16/32-bit 和大/小端
- **check_mod_consistency.py**：11项一致性检查，确保模板与移植版同步
- **check_compile.py**：自动C语法检查（gcc/clang）

---

## 架构

```
┌──────────────────────────────────────────────────┐
│                 用户应用程序                       │
│              (User Application)                    │
└────────────────────┬─────────────────────────────┘
                     │ boot_update() 调用
┌────────────────────▼─────────────────────────────┐
│              Boot升级模块 (Mod/)                    │
│                                                    │
│  ┌──────────┐  ┌──────────────────────────────┐   │
│  │  app.c   │  │  bsp_flash.c / .h            │   │
│  │  boot_   │  │  ├─ bsp_flash_write()         │   │
│  │  update()│  │  ├─ bsp_flash_read()          │   │
│  └──────────┘  │  ├─ bsp_cmp_flash()           │   │
│                │  └─ bsp_flash_page_erase()    │   │
│                └──────────┬───────────────────┘   │
│                           │                       │
│  ┌───────────────────────▼────────────────────┐   │
│  │  api_flash_port.h / .c                    │   │
│  │  (平台抽象层 - 用户需实现以下接口)          │   │
│  │  ├─ api_flash_lock/unlock()               │   │
│  │  ├─ api_flash_write()                     │   │
│  │  ├─ api_flash_page_erase()                │   │
│  │  ├─ api_irq_enable/disable()              │   │
│  │  └─ api_flash_write_eepro/eepro(可选)    │   │
│  └───────────────────────────────────────────┘   │
└──────────────────────────────────────────────────┘
                     │
                     ▼ 移植到目标平台
           ┌─────────────────────────┐
           │   目标MCU (STM32/HAL等)  │
           │   底层Flash驱动          │
           └─────────────────────────┘
```

### 两个目录的定位

| 目录 | 角色 | 说明 |
|------|------|------|
| `Mod/` | **平台无关模板** | 完全可移植的纯C代码，不留存任何HAL依赖 |
| `boot_update_test/app-brp/Mod/` | **参考实现** | STM32F103 BootLoader 工程中的移植，包含 `bsp_flash_test()` 验证函数 |

两目录通过 `tool/check_mod_consistency.py` 保证核心函数一致性。

### 双工程结构（`boot_update_test/`）

| 工程 | 链接地址 | 工程文件 | 功能 |
|------|----------|----------|------|
| `app-brp/` | 0x08010000（448KB） | `app-brp/MDK-ARM/app-brp.uvprojx` | **Mod 框架 APP**：上电调用 `boot_update()` 擦除并写入 BOOT 区（`bin_buf` 由 bin2c 生成）→ 校验 → 升级 BOOT |
| `app/` | 0x08010000（448KB） | `app/MDK-ARM/app.uvprojx` | 普通 APP 演示程序（LED 闪烁 + 串口打印） |
| `boot/` | —— | —— | **移植前参考**：原始 Boot 工程（`Mod/` 已移除，不编译），保留作对照 |

> 注：`boot/` 为移植前参考，勿修改；如需 BootLoader 可基于它重新移植 Mod。

---

## 目录结构

```
├── Mod/                                    # 平台无关的Boot升级模板
│   ├── App/
│   │   ├── app.c                           # boot_update() 主函数
│   │   └── app.h                           # APP/BOOT起始地址定义
│   └── External_Interfaces/
│       ├── api_flash_port.h                # 平台抽象层 (宏定义 + 接口声明)
│       ├── api_flash_port.c                # 平台抽象层 (空壳实现，用户填充)
│       ├── bsp_flash.h                     # Flash操作头文件
│       └── bsp_flash.c                     # Flash写/读/比较/擦除实现
│
├── boot_update_test/                       # STM32F103 参考实现
│   ├── Drivers/                            # 共享 STM32F1xx HAL + CMSIS (两工程共用)
│   ├── app-brp/                           # Mod 框架 APP 工程 (0x08010000,上电升级 BOOT)
│   │   ├── Core/Src/main.c                 #   上电调 boot_update() 升级 BOOT
│   │   ├── Mod/                            #   STM32F103 移植版 Mod + BOOT1.c(bin_buf)
│   │   └── MDK-ARM/                        #   app-brp.uvprojx
│   ├── boot/                               # 【移植前参考】旧 Boot 工程(Mod 已移除,不编译)
│   └── app/                                # 普通 APP 演示工程 (0x08010000)
│       ├── Core/Src/main.c                 #   APP demo (LED 闪烁 + 串口打印)
│       └── MDK-ARM/                        #   app.uvprojx (输出 app_update.bin)
│
├── tool/
│   ├── bin2c/bin2c.py                      # 固件转C数组工具
│   ├── check_mod_consistency.py            # 模板一致性检查 (11项)
│   └── check_compile.py                    # C语法编译检查
│
├── doc/
│   ├── 移植指南.md                          # 跨平台移植文档
│   ├── 专利申请表/                          # 专利相关文档
│   └── 系统设计文档_v1.0.docx              # 系统设计说明书
│
└── README.md
```

---

## 快速开始

### 1. 生成C数组格式的固件

```bash
cd tool/bin2c
python bin2c.py
# 选择位数 (8/16/32) 和端序 (L/B)
# 输出: BOOT.bin → BOOT.c
```

`BOOT.c` 中包含：
```c
const FlashBandwidthType_t bin_buf[] = { /* ... */ };
const unsigned long long bin_buf_elem_len = 1024;  // 数组元素个数
```

### 2. 将生成的 `BOOT.c` 加入工程编译

### 3. 调用升级函数

```c
#include "app.h"

int main(void) {
    // 初始化...
    boot_update();   // 执行Boot固件升级
    // ...
}
```

### 4. 配置GOTO_FLAG（可选）

在编译选项中定义：
```c
#define ENABLE_GOTO_FLAG   // 启用跳转标志功能
```

并在 `api_flash_port.h` 中配置GOTO_FLAG地址：
```c
#define HAL_GOTO_FLAG_BASE_ADDR   (uint32_t)(0x0807FC00U)
#define HAL_GOTO_FLAG_OFFSET      1
#define HAL_GOTO_FLAG_PARAM       0x00000005
```

---

## YMODEM 串口升级（IAP）

Boot（`boot_update_test/boot/`）实现基于 YMODEM 协议的串口 IAP 升级：

1. **烧录**：先用 Keil 将 `boot.uvprojx` 编译的 boot 固件烧录到 0x08000000（J-Flash/ST-LINK 均可）。
2. **构建 APP**：用 Keil 打开 `app.uvprojx` 编译，构建后自动在 `app/MDK-ARM/` 生成 `app_update.bin`。
3. **升级**（两种方式任选）：
   - 简易上位机（推荐）：`python tool/ymodem_upload.py -p COM5 -f app_update.bin`（需 `pip install pyserial`；启动后请在 15 秒内复位目标板，使 Boot 处于等待升级状态）。
   - 通用串口工具：上电后 Boot 在串口（USART1，115200）打印提示并等待 5 秒。在等待窗口内用 Tera Term / SecureCRT 等选择 **YMODEM → Send File…** 上传 `app_update.bin`。
4. Boot 收到固件后：擦除 APP 区 → 逐块写入 → `bsp_cmp_flash` 校验 → 跳转 APP（0x08010000）。
5. 若 APP 区无有效固件，Boot 会持续等待升级，不会进入死循环。

**注意**：升级完成后需重新上电或复位才会再次进入升级等待窗口；正常运行时 Boot 直接跳转 APP。

---

## 移植指南

将本项目移植到新MCU平台的步骤：

### 步骤1：配置Flash参数

在 `api_flash_port.h` 中修改：
```c
#define HAL_FLASH_BASE_ADDR       0x08000000U   // Flash基地址
#define HAL_FLASH_END_ADDR        0x0807FFFFU   // Flash结束地址
#define HAL_FLASH_PAGE_SIZE       (1 * 2048)    // 页大小
#define HAL_MIN_WRITE_BAYE        8             // 最小写入粒度（字节）
#define HAL_BAND_WIDTH            4             // 带宽（字节）
```

### 步骤2：实现平台抽象层

在 `api_flash_port.c` 中填充5个核心接口：

| 接口 | 说明 |
|------|------|
| `api_flash_lock()` | Flash上锁（如HAL_FLASH_Lock） |
| `api_flash_unlock()` | Flash解锁（如HAL_FLASH_Unlock） |
| `api_flash_write(addr, data)` | 按最小粒度写入Flash |
| `api_flash_page_erase(addr)` | 擦除一页Flash |
| `api_irq_disable/enable()` | 临界区保护（中断开关） |

### 步骤3：设置APP/BOOT地址

在 `app.h` 中定义：
```c
#define APP_START_ADDR  0x08020000U    // APP起始地址
#define BOOT_START_ADDR 0x08000000U    // Boot起始地址
```

### 步骤4：使用一致性检查验证移植

```bash
# 将移植后的 Mod/ 复制为 boot_update_test/app-brp/Mod/
# 运行一致性检查
python tool/check_mod_consistency.py
# 所有 11 项检查均应 PASS
```

### 步骤5：编译验证

```bash
python tool/check_compile.py
# 检查 3 个核心源文件的 C 语法
```

详细移植步骤请参阅 [doc/移植指南.md](doc/%E7%A7%BB%E6%A4%8D%E6%8C%87%E5%8D%97.md)。

---

## 工具链

### bin2c.py — 固件转C数组

```bash
python tool/bin2c/bin2c.py

# 交互式流程：
# 1. 自动查找当前目录下第一个 .bin 文件
# 2. 选择输出位数：8 / 16 / 32-bit
# 3. 选择端序（16/32-bit）：Little / Big endian
# 4. 自动补齐到带宽对齐
# 5. 输出 .c 文件（数组名: bin_buf）
```

### check_mod_consistency.py — 一致性检查

检查 `Mod/` 和 `boot_update_test/app-brp/Mod/` 的一致性：

```
11项检查：
  1. extern 类型一致性
  2. include 路径正确性
  3. 无非法引用（mid_eeprom.h）
  4. bsp_cmp_flash 写后验证
  5. FlashBandwidthType_t 类型定义
  6-11. Mod/ 与 boot_update_test/app-brp/Mod/ 一致性
```

### check_compile.py — 编译检查

对 `Mod/` 下 3 个核心 C 文件进行语法检查：
- `Mod/App/app.c`
- `Mod/External_Interfaces/api_flash_port.c`
- `Mod/External_Interfaces/bsp_flash.c`

---

## 验证

### 自动验证流程

```bash
# 1. 一致性检查
python tool/check_mod_consistency.py

# 2. 编译检查
python tool/check_compile.py
```

### 验证状态

| 组件 | 状态 |
|------|------|
| Mod/ 平台无关模板 | ✅ 编译通过，无语法错误 |
| boot_update_test/ 参考实现 | ✅ MDK-ARM编译通过 |
| Mod/ ↔ boot_update_test/app-brp/Mod/ 一致性 | ✅ 11/11 检查全部PASS |
| 地址越界保护 | ✅ 所有Flash操作均含边界检查 |
| 写入校验 | ✅ bsp_flash_write后自动验证 |

---

## 已知限制

| 限制 | 说明 | 状态 |
|------|------|------|
| 无回滚机制 | 升级失败不自动回退到旧版本 | 文档记录 |
| 页粒度擦除 | 未实现部分页写入或更细粒度擦除 | 保持页对齐即可 |
| 无单元测试框架 | 验证依赖编译检查和手动测试 | 设计如此 |

---

## 许可

本项目包含专利相关材料：
- 《一种基于嵌入式软件的通用APP升级Boot固件的方法及系统》v1.0
- 系统设计说明书 v1.0

详见 `doc/专利申请表/` 目录。

---

## 相关资源

- **跨平台移植指南**：[doc/移植指南.md](doc/%E7%A7%BB%E6%A4%8D%E6%8C%87%E5%8D%97.md)
- **技术文档**：[doc/系统设计文档_v1.0.docx](doc/%E4%B8%80%E7%A7%8D%E5%9F%BA%E4%BA%8E%E5%B5%8C%E5%85%A5%E5%BC%8F%E8%BD%AF%E4%BB%B6%E7%9A%84%E9%80%9A%E7%94%A8APP%E5%8D%87%E7%BA%A7boot%E5%9B%BA%E4%BB%B6%E7%9A%84%E6%96%B9%E6%B3%95%E5%8F%8A%E7%B3%BB%E7%BB%9Fv1.0%E2%80%94%E2%80%94%E7%B3%BB%E7%BB%9F%E8%AE%BE%E8%AE%A1%E8%AF%B4%E6%98%8E%E4%B9%A6.docx)
- **参考实现平台**：STM32F103CBT6 / STM32F103ZETR
