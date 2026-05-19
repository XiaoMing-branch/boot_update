# Boot固件升级系统重构与修复计划

## TL;DR

> **Quick Summary**: 修复嵌入式Boot固件升级系统中发现的4个CRITICAL和3个HIGH级别问题，统一Mod/目录为单一权威源码，建立自动化验证机制，并编写跨平台移植文档。
>
> **Deliverables**:
> - 修复后的 `Mod/` 核心模块（类型不匹配、extern声明、文件引用、缺少校验）
> - 同步后的 `boot_update_test/Mod/` 参考实现
> - `tool/check_mod_consistency.py` 一致性检查脚本
> - `tool/check_compile.py` 编译验证脚本
> - `doc/移植指南.md` 跨平台移植文档
> - 修复后的 `tool/bin2c/bin2c.py`
>
> **Estimated Effort**: Medium (10个实现任务 + 4个验证任务)
> **Parallel Execution**: YES - 3 waves
> **Critical Path**: T1→T6→T7→F1-F4

---

## Context

### Original Request
用户有一个嵌入式Boot固件升级系统，代码核心在 `Mod/` 目录下，STM32参考实现在 `boot_update_test/`。要求对该项目进行代码审查、问题修复、架构重构，并生成跨平台移植文档。

### Interview Summary
**Key Discussions**:
- 代码审查发现了4个CRITICAL和3个HIGH级别问题（类型不匹配、extern声明不一致、文件引用错误、缺少写入校验、两份代码不一致、路径大小写问题等）
- 用户选择3个目标：修复CRITICAL+HIGH问题 + 重构架构 + 生成移植文档
- 保留完整升级流程（擦除→写入→GOTO_FLAG→系统复位）
- GOTO_FLAG改为条件编译宏控制
- Mod/ 和 boot_update_test/Mod/ 都保留，通过一致性脚本同步
- 验证方式：自动编译验证 + 一致性检查脚本

**Research Findings**:
- 实际目标MCU为STM32F103（非仓库名所示的STM32H750XBH6）
- MDK工程编译的是 `boot_update_test/Mod/`，非根目录 `Mod/`
- bsp_cmp_flash已实现但从未被调用
- bin2c工具输出格式与app.c的extern声明不匹配

### Metis Review
**Identified Gaps** (addressed):
- MCU型号混淆（STM32F103 vs STM32H750XBH6）→ 已确认Mod/为平台无关代码
- GOTO_FLAG地址0x00400000不在STM32F103 Flash范围内 → 改为条件编译，地址由用户定义
- BOOT1.c链接关系需确认 → 已明确位于Mod/目录
- HAL_MIN_WRITE_BAYE默认值分歧（测试版=4，模板版=64）→ 采用用户可覆盖的默认值策略

---

## Work Objectives

### Core Objective
修复Boot固件升级系统中的4个CRITICAL和3个HIGH级别问题，统一Mod/为平台无关的权威源码，添加自动化验证机制，撰写跨平台移植文档。

### Concrete Deliverables
1. `Mod/App/app.c` — 修复extern声明类型，添加写入后校验，修复路径引用
2. `Mod/App/app.h` — 修复路径大小写，GOTO_FLAG条件编译
3. `Mod/External_Interfaces/api_flash_port.h` — 删除无效引用，标准化宏体系，添加eepro声明
4. `Mod/External_Interfaces/api_flash_port.c` — 添加eepro函数空壳
5. `Mod/External_Interfaces/bsp_flash.c` — 添加地址有效性检查，调用cmp_flash
6. `boot_update_test/Mod/*` — 同步为Mod/的参考实现
7. `tool/bin2c/bin2c.py` — 输出格式与FlashBandwidthType_t对齐
8. `tool/check_mod_consistency.py` — 一致性检查脚本
9. `doc/移植指南.md` — 跨平台移植文档

### Definition of Done
- [x] `python tool/check_mod_consistency.py` 返回exit code 0
- [x] 所有CRITICAL问题（C1-C4）的修复确认通过grep验证
- [x] 所有HIGH问题（H1-H3）的修复确认通过grep验证
- [x] `doc/移植指南.md` 覆盖所有必要内容

### Must Have
- 修复：app.c中 `bin_buf` 的extern类型必须与BOOT1.c定义匹配
- 修复：extern变量名统一（bin_buf_elem_len vs bin_buf_len）
- 修复：api_flash_port.h 删除 `#include "mid_eeprom.h"`
- 修复：app.h 中路径大小写正确
- 添加：`bsp_flash_write()` 后调用 `bsp_cmp_flash()` 验证
- 添加：GOTO_FLAG条件编译开关
- 添加：一致性检查脚本
- 添加：跨平台移植文档

### Must NOT Have (Guardrails)
- ❌ 不实现回滚机制（L3）- 文档记录为已知限制
- ❌ 不引入单元测试框架
- ❌ 不修改Drivers/、Core/等HAL层文件
- ❌ 不修改doc/专利申请表/中的专利文档
- ❌ 不添加bin2c新功能（端序检测、GUI等）
- ❌ 不修改LOW级别问题（擦除粒度、printf格式等）
- ❌ 不实现性能优化（只做正确性修复）

---

## Verification Strategy

> **ZERO HUMAN INTERVENTION** - ALL verification is agent-executed.

### Test Decision
- **Infrastructure exists**: NO (嵌入式项目，无单元测试框架)
- **Automated tests**: None + Agent QA（编译验证 + 一致性脚本检查）
- **Framework**: N/A (嵌入式C代码)

### QA Policy
- **一致性检查**: `python tool/check_mod_consistency.py` — 自动验证extern声明匹配、文件引用有效、类型一致
- **编译验证**: `python tool/check_compile.py` — 对Mod/核心代码做宿主机构建语法检查
- **Grep验证**: 对每项CRITICAL/HIGH修复，用grep确认修复生效
- **文件对比**: 用diff或Python脚本验证boot_update_test/Mod/与Mod/同步

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Start Immediately — Mod/核心模块修复，全并行):
├── T1: Mod/App/app.c & app.h 修复 [quick]
├── T2: Mod/External_Interfaces/api_flash_port.h 修复 [quick]
├── T3: Mod/External_Interfaces/api_flash_port.c 添加eepro存根 [quick]
├── T4: Mod/External_Interfaces/bsp_flash.c 添加校验和地址检查 [quick]
└── T5: tool/bin2c/bin2c.py 输出格式统一 [quick]

Wave 2 (After Wave 1 — 同步+验证+文档，部分并行):
├── T6: boot_update_test/Mod/ 同步T1-T4的修复 (depends: T1-T4) [unspecified-high]
├── T7: MDK-ARM工程编译验证 (depends: T6) [unspecified-high]
├── T8: 一致性检查脚本 tool/check_mod_consistency.py [writing]
├── T9: 编译验证脚本 tool/check_compile.py [writing]
└── T10: 跨平台移植指南 doc/移植指南.md (depends: T1-T5) [writing]

Wave FINAL (After ALL tasks — 4 parallel reviews, then user okay):
├── F1: Plan compliance audit (oracle)
├── F2: Code quality review (unspecified-high)
├── F3: Real manual QA - 验证脚本运行 (unspecified-high)
└── F4: Scope fidelity check (deep)
-> Present results -> Get explicit user okay

Critical Path: T1/T2/T3/T4/T5 → T6 → T7 → F1-F4 → user okay
Parallel Speedup: Wave 1全并行(5任务同时)，Wave 2大部分并行
Max Concurrent: 5 (Wave 1)
```

---

## TODOs

- [x] 1. 修复 Mod/App/app.c 和 app.h — extern声明、路径大小写、写入校验

  **What to do**:
  - **Mod/App/app.c**: 修改 `extern const unsigned long bin_buf[]` → `extern const FlashBandwidthType_t bin_buf[]`，修改 `extern const unsigned long bin_buf_elem_len` → `extern const unsigned long long bin_buf_elem_len`（匹配bin2c输出类型）
  - **Mod/App/app.c**: 在 `bsp_flash_write()` 成功后添加 `bsp_cmp_flash()` 调用，验证写入数据的完整性。失败则打印错误信息但不阻断后续流程
  - **Mod/App/app.c**: 将GOTO_FLAG相关逻辑包裹在 `#ifdef ENABLE_GOTO_FLAG` 条件编译块中
  - **Mod/App/app.h**: 修改 `#include "../mod/External_Interfaces/bsp_flash.h"` → `#include "../Mod/External_Interfaces/bsp_flash.h"`（大写M）
  - **Mod/App/app.h**: 用条件编译保护 BOOT_START_ADDR 和 APP_START_ADDR 的#error检查

  **Must NOT do**:
  - 不修改 boot_update_test/Mod/App/app.c/app.h（那是T6的任务）
  - 不修改 BOOT1.c
  - 不改动现有升级流程顺序

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: 单文件级文本替换和逻辑补充，无需深度设计
  - **Skills**: [] (无需特殊技能)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with T2, T3, T4, T5)
  - **Blocks**: T6 (boot_update_test sync)
  - **Blocked By**: None (can start immediately)

  **References**:
  - `Mod/App/app.c` - 当前文件，需要修改
  - `Mod/App/app.h` - 当前文件，需要修改
  - `tool/bin2c/bin2c.py` - bin2c输出格式参考，确保extern类型匹配

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Verify extern type fix for bin_buf
    Tool: Bash (grep)
    Preconditions: Mod/App/app.c modified
    Steps:
      1. grep "extern.*FlashBandwidthType_t.*bin_buf" Mod/App/app.c
    Expected Result: Line found showing `extern const FlashBandwidthType_t bin_buf[]`
    Failure Indicators: grep returns empty (still unsigned long)
    Evidence: .sisyphus/evidence/task-1-extern-binbuf.txt

  Scenario: Verify extern name fix for bin_buf_elem_len
    Tool: Bash (grep)
    Preconditions: Mod/App/app.c modified
    Steps:
      1. grep "extern.*bin_buf_elem_len" Mod/App/app.c
    Expected Result: Line found showing `unsigned long long bin_buf_elem_len`
    Failure Indicators: Variable name wrong or type mismatch
    Evidence: .sisyphus/evidence/task-1-extern-elemlen.txt

  Scenario: Verify path case fix
    Tool: Bash (grep)
    Preconditions: Mod/App/app.h modified
    Steps:
      1. grep "#include.*bsp_flash.h" Mod/App/app.h
    Expected Result: Path uses capital M: `../Mod/External_Interfaces/bsp_flash.h`
    Failure Indicators: Path contains lowercase `../mod/...`
    Evidence: .sisyphus/evidence/task-1-path-case.txt

  Scenario: Verify write verification added
    Tool: Bash (grep)
    Preconditions: Mod/App/app.c modified
    Steps:
      1. grep -n "bsp_cmp_flash" Mod/App/app.c
    Expected Result: Shows bsp_cmp_flash called after bsp_flash_write
    Failure Indicators: No bsp_cmp_flash call found
    Evidence: .sisyphus/evidence/task-1-write-verify.txt

  Scenario: Verify GOTO_FLAG conditional compilation
    Tool: Bash (grep)
    Preconditions: Mod/App/app.c modified
    Steps:
      1. grep -n "ENABLE_GOTO_FLAG\|#ifdef\|#endif" Mod/App/app.c
    Expected Result: GOTO_FLAG logic wrapped in #ifdef ENABLE_GOTO_FLAG ... #endif
    Failure Indicators: No conditional compilation guards found
    Evidence: .sisyphus/evidence/task-1-goto-flag-cond.txt
  ```

  **Evidence to Capture**:
  - [ ] task-1-extern-binbuf.txt
  - [ ] task-1-extern-elemlen.txt
  - [ ] task-1-path-case.txt
  - [ ] task-1-write-verify.txt
  - [ ] task-1-goto-flag-cond.txt

  **Commit**: YES (with T2, T3, T4, T5 — single commit for Wave 1)
  - Message: `fix(mod): repair type mismatches, extern declarations, file references and add write verification`

---

- [x] 2. 修复 Mod/External_Interfaces/api_flash_port.h — 删除无效引用，标准化宏

  **What to do**:
  - 删除 `#include "mid_eeprom.h"`（文件不存在）
  - 将 `HAL_GOTO_FLAG_BASE_ADDR`、`HAL_GOTO_FLAG_OFFSET`、`HAL_GOTO_FLAG_PARAM` 等GOTO_FLAG相关宏包裹在 `#ifdef ENABLE_GOTO_FLAG`条件块中
  - 统一 `HAL_GOTO_FLAG_BASE_ADDR` 默认值为可覆盖的宏（不硬编码0x00400000），添加注释提示用户必须根据平台修改
  - 保留 `#ifndef HAL_FLASH_BASE_ADDR` 等保护性#error检查
  - 保留 `HAL_BAND_WIDTH` 和 `FlashBandwidthType_t` 自动类型推导
  - 保留 `api_flash_write_eepro` 和 `api_flash_page_erase_eepro` 的声明
  - 添加注释说明：所有`HAL_xxx`宏都由用户根据实际平台定义

  **Must NOT do**:
  - 不引入STM32 HAL头文件(如main.h)到Mod/模板中
  - 不删除eepro函数声明
  - 不修改 FlashBandwidthType_t 的类型推导逻辑

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: 头文件宏调整和条件编译，工作单一
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with T1, T3, T4, T5)
  - **Blocks**: T6 (boot_update_test sync)
  - **Blocked By**: None

  **References**:
  - `Mod/External_Interfaces/api_flash_port.h` - 当前文件
  - `boot_update_test/Mod/External_Interfaces/api_flash_port.h` - 参考实现

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Verify mid_eeprom.h removed
    Tool: Bash (grep)
    Preconditions: api_flash_port.h modified
    Steps:
      1. grep -n "mid_eeprom" Mod/External_Interfaces/api_flash_port.h
    Expected Result: grep returns empty (no matches)
    Failure Indicators: Line with mid_eeprom.h found
    Evidence: .sisyphus/evidence/task-2-no-mid-eeprom.txt

  Scenario: Verify GOTO_FLAG conditional
    Tool: Bash (grep)
    Preconditions: api_flash_port.h modified
    Steps:
      1. grep -n "ENABLE_GOTO_FLAG\|HAL_GOTO_FLAG" Mod/External_Interfaces/api_flash_port.h
    Expected Result: GOTO_FLAG macros wrapped in #ifdef ENABLE_GOTO_FLAG
    Failure Indicators: HAL_GOTO_FLAG_xxx defined without conditional guard
    Evidence: .sisyphus/evidence/task-2-goto-flag-cond.txt

  Scenario: Verify FlashBandwidthType_t preserved
    Tool: Bash (grep)
    Preconditions: api_flash_port.h modified
    Steps:
      1. grep -n "FlashBandwidthType_t\|HAL_BAND_WIDTH" Mod/External_Interfaces/api_flash_port.h
    Expected Result: typedef FlashBandwidthType_t still present based on HAL_BAND_WIDTH
    Failure Indicators: FlashBandwidthType_t removed or broken
    Evidence: .sisyphus/evidence/task-2-bandwidth-type.txt
  ```

  **Evidence to Capture**:
  - [ ] task-2-no-mid-eeprom.txt
  - [ ] task-2-goto-flag-cond.txt
  - [ ] task-2-bandwidth-type.txt

  **Commit**: YES (with T1, T3, T4, T5 — Wave 1 commit)

---

- [x] 3. 修复 Mod/External_Interfaces/api_flash_port.c — 添加eepro函数存根

  **What to do**:
  - 添加 `api_flash_write_eepro()` 函数存根（返回RUN_ERROR）
  - 添加 `api_flash_page_erase_eepro()` 函数存根（返回RUN_ERROR）
  - 添加注释 `/* USER CODE BEGIN — implement for EEPROM/ external memory support */` 和 `/* USER CODE END */`
  - 删除原文件头部的 `#include "mid_eeprom.h"`（如果存在）

  **Must NOT do**:
  - 不实现eepro函数逻辑（留给用户实现）
  - 不修改其他已有函数

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: 添加两个空壳函数，简单明确

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with T1, T2, T4, T5)
  - **Blocks**: T6
  - **Blocked By**: None

  **References**:
  - `Mod/External_Interfaces/api_flash_port.h` - 函数声明
  - `Mod/External_Interfaces/api_flash_port.c` - 当前文件，参考现有空壳函数模式

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Verify eepro stubs added
    Tool: Bash (grep)
    Preconditions: api_flash_port.c modified
    Steps:
      1. grep -n "api_flash_write_eepro\|api_flash_page_erase_eepro" Mod/External_Interfaces/api_flash_port.c
    Expected Result: Both function definitions present
    Failure Indicators: One or both functions missing
    Evidence: .sisyphus/evidence/task-3-eepro-stubs.txt

  Scenario: Verify eepro functions return RUN_ERROR (empty stubs)
    Tool: Bash (grep)
    Preconditions: api_flash_port.c modified
    Steps:
      1. grep -A5 "api_flash_write_eepro" Mod/External_Interfaces/api_flash_port.c | grep "RUN_ERROR"
    Expected Result: Shows return RUN_ERROR
    Failure Indicators: Returns something else or is implemented
    Evidence: .sisyphus/evidence/task-3-eepro-stub-body.txt
  ```

  **Evidence to Capture**:
  - [ ] task-3-eepro-stubs.txt
  - [ ] task-3-eepro-stub-body.txt

  **Commit**: YES (with T1, T2, T4, T5 — Wave 1 commit)

---

- [x] 4. 修复 Mod/External_Interfaces/bsp_flash.c — 添加地址有效性检查

  **What to do**:
  - **bsp_flash_write()**: 添加地址范围检查 `if (addr + size > HAL_FLASH_BASE_ADDR + HAL_FLASH_SIZE)`，合并到现有的 `if ((size == 0) || (addr % HAL_MIN_WRITE_BAYE != 0))` 条件中
  - **bsp_flash_read()**: 添加地址范围检查 `if((HAL_FLASH_BASE_ADDR <= addr) && (addr <= HAL_FLASH_END_ADDR) && (size != 0) && ((addr + size - 1) <= HAL_FLASH_END_ADDR))`，从 boot_update_test 版本移植
  - **bsp_flash_page_erase()**: 添加地址范围检查 `if((HAL_FLASH_BASE_ADDR <= addr) && (addr <= HAL_FLASH_END_ADDR) && (addr % HAL_FLASH_PAGE_SIZE == 0))`，从 boot_update_test 版本移植
  - 保留 Mod/ 版本的 `volatile FlashBandwidthType_t*` 读取方式（带宽感知，比字节读取更正确）

  **Must NOT do**:
  - 不改动 bsp_flash_read 的读取实现方式（保留带宽感知读取）
  - 不添加 bsp_flash_test() 到 Mod/ 版本中（该函数仅测试平台专用）
  - 不修改错误处理逻辑（goto end 结构保持不变）

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: 从boot_update_test版本移植已写好的验证代码到Mod/版本

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with T1, T2, T3, T5)
  - **Blocks**: T6
  - **Blocked By**: None

  **References**:
  - `Mod/External_Interfaces/bsp_flash.c` - 当前文件
  - `boot_update_test/Mod/External_Interfaces/bsp_flash.c` - 参考已验证的地址检查实现

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Verify bsp_flash_write address validation
    Tool: Bash (grep)
    Preconditions: bsp_flash.c modified
    Steps:
      1. grep -n "HAL_FLASH_BASE_ADDR + HAL_FLASH_SIZE\|addr + size >" Mod/External_Interfaces/bsp_flash.c
    Expected Result: Shows address range check in bsp_flash_write
    Failure Indicators: No address validation found
    Evidence: .sisyphus/evidence/task-4-write-validation.txt

  Scenario: Verify bsp_flash_read address validation
    Tool: Bash (grep)
    Preconditions: bsp_flash.c modified
    Steps:
      1. grep -n "HAL_FLASH_BASE_ADDR <= addr" Mod/External_Interfaces/bsp_flash.c
    Expected Result: Shows address range check in bsp_flash_read
    Failure Indicators: No address validation found
    Evidence: .sisyphus/evidence/task-4-read-validation.txt

  Scenario: Verify bsp_flash_page_erase address validation
    Tool: Bash (grep)
    Preconditions: bsp_flash.c modified
    Steps:
      1. grep -n "HAL_FLASH_BASE_ADDR <= addr" Mod/External_Interfaces/bsp_flash.c | head -1
    Expected Result: Shows address range check in bsp_flash_page_erase
    Failure Indicators: No address validation in page_erase
    Evidence: .sisyphus/evidence/task-4-erase-validation.txt
  ```

  **Evidence to Capture**:
  - [ ] task-4-write-validation.txt
  - [ ] task-4-read-validation.txt
  - [ ] task-4-erase-validation.txt

  **Commit**: YES (with T1, T2, T3, T5 — Wave 1 commit)

---

- [x] 5. 修复 tool/bin2c/bin2c.py — 输出格式与 FlashBandwidthType_t 对齐

  **What to do**:
  - 当前 bin2c.py 生成的数组类型取决于用户选择的 bit_width（8→unsigned char, 16→unsigned short, 32→unsigned int），变量名为 `bin_buf_elem_len` (unsigned long long) 和 `bin_buf_byte_len`
  - 修改输出：生成的变量名固定为 `bin_buf_elem_len`（类型 `unsigned long long`，与实际计算值匹配）；如需字节数可以使用 `sizeof(bin_buf)`
  - 在生成的 .c 文件头部添加注释块，说明该数组应与 `FlashBandwidthType_t` 配合使用，`HAL_BAND_WIDTH` 必须与转换位宽一致
  - 添加注释示例：`/* Warning: Ensure HAL_BAND_WIDTH in api_flash_port.h matches the bit_width used during conversion */`

  **Must NOT do**:
  - 不添加新功能（端序自动检测、GUI等）
  - 不改变现有的交互式选择流程（用户仍需选择bit_width和端序）
  - 不删除 `bin_buf_byte_len` 生成（保留以兼容可能的使用场景）

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Python脚本输出格式调整，代码量小

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with T1, T2, T3, T4)
  - **Blocks**: None (独立)
  - **Blocked By**: None

  **References**:
  - `tool/bin2c/bin2c.py` - 当前文件
  - `Mod/App/app.c` - extern声明参考
  - `Mod/BOOT1.c` - 当前生成的输出参考

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Verify bin2c outputs correct variable names
    Tool: Bash
    Preconditions: bin2c.py modified, test with a small bin file
    Steps:
      1. Create a temporary test file test.bin with known content
      2. Run bin2c.py with 32-bit mode on test.bin
      3. Check output .c file for variable names
    Expected Result: Output contains `bin_buf_elem_len` as unsigned long long type
    Failure Indicators: Wrong variable name or type
    Evidence: .sisyphus/evidence/task-5-bin2c-output.txt

  Scenario: Verify bin2c includes HAL_BAND_WIDTH warning comment
    Tool: Bash (grep)
    Preconditions: bin2c.py modified
    Steps:
      1. grep -n "HAL_BAND_WIDTH" tool/bin2c/bin2c.py
    Expected Result: Shows comment or code referencing HAL_BAND_WIDTH compatibility
    Failure Indicators: No HAL_BAND_WIDTH mention
    Evidence: .sisyphus/evidence/task-5-bandwidth-warning.txt
  ```

  **Evidence to Capture**:
  - [ ] task-5-bin2c-output.txt
  - [ ] task-5-bandwidth-warning.txt

  **Commit**: YES (with T1, T2, T3, T4 — Wave 1 commit)

---

- [x] 6. 同步 boot_update_test/Mod/ — 使其与 Mod/ 保持一致

  **What to do**:
  - 将 T1-T4 的所有修改同步到 `boot_update_test/Mod/` 对应文件中
  - 具体来说：
    - `boot_update_test/Mod/App/app.c` — 同步 app.c 的修改（添加GOTO_FLAG逻辑、写入校验、使用正确的extern声明）
    - `boot_update_test/Mod/App/app.h` — 同步 app.h 的修改（路径、条件编译）
    - `boot_update_test/Mod/External_Interfaces/api_flash_port.h` — 同步头文件修改（GOTO_FLAG条件编译、标准化宏），但保留 `#include "main.h"`（STM32 HAL依赖）
    - `boot_update_test/Mod/External_Interfaces/api_flash_port.c` — 不同步！因为本文件是STM32 HAL实际实现（不是空壳），只添加eepro函数实现（如果有必要）
    - `boot_update_test/Mod/External_Interfaces/bsp_flash.c` — 同步 bsp_flash 的修改（地址检查已在测试版中存在，确认保留即可）
  - 运行 `diff Mod/App/app.h boot_update_test/Mod/App/app.h` 等对比命令确认一致
  - 注意：api_flash_port.c 不同步（测试版有实际STM32实现，Mod/版是空壳模板）

  **Must NOT do**:
  - 不覆盖 `boot_update_test/Mod/External_Interfaces/api_flash_port.c`（保持STM32 HAL实现）
  - 不删除 `bsp_flash_test()`（这是测试版特有的测试函数）
  - 不修改 MDK-ARM 工程文件（如果编译通过则不动）

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: 需要在两个目录间精确同步，理解哪些文件该同步、哪些不该

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on T1-T4)
  - **Parallel Group**: Wave 2
  - **Blocks**: T7 (build verification)
  - **Blocked By**: T1, T2, T3, T4 (T5 is independent)

  **References**:
  - All modified Mod/* files from T1-T4
  - All boot_update_test/Mod/* files for comparison

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Verify app.c is synced
    Tool: Bash (diff)
    Preconditions: T1-T4 completed, sync applied
    Steps:
      1. diff Mod/App/app.c boot_update_test/Mod/App/app.c
    Expected Result: diff shows NO meaningful differences (only expected ones like include paths)
    Failure Indicators: Significant logic differences remain
    Evidence: .sisyphus/evidence/task-6-sync-app-c.txt

  Scenario: Verify api_flash_port.c NOT overwritten
    Tool: Bash (grep)
    Preconditions: Sync applied
    Steps:
      1. grep "HAL_FLASH_Program\|HAL_FLASHEx_Erase" boot_update_test/Mod/External_Interfaces/api_flash_port.c
    Expected Result: Shows STM32 HAL calls still present (not replaced by empty stubs)
    Failure Indicators: No HAL calls found (empty stubs overwritten)
    Evidence: .sisyphus/evidence/task-6-stm32-impl-preserved.txt

  Scenario: Verify bsp_flash_test still present
    Tool: Bash (grep)
    Preconditions: Sync applied
    Steps:
      1. grep "bsp_flash_test" boot_update_test/Mod/External_Interfaces/bsp_flash.h
    Expected Result: bsp_flash_test declared
    Failure Indicators: Declaration removed
    Evidence: .sisyphus/evidence/task-6-test-func-preserved.txt
  ```

  **Evidence to Capture**:
  - [ ] task-6-sync-app-c.txt
  - [ ] task-6-stm32-impl-preserved.txt
  - [ ] task-6-test-func-preserved.txt

  **Commit**: YES (with T7, T8, T9 — Wave 2 commit)
  - Message: `chore(test): sync test project with Mod/ fixes and add verification scripts`

---

- [x] 7. MDK-ARM 工程编译验证

  **What to do**:
  - 尝试编译 boot_update_test 的 MDK-ARM 工程（或至少验证工程文件配置正确）
  - 如果无法直接运行 Keil MDK，则手动验证：
    - 检查 UVProjx 中的 IncludePath 指向正确的目录
    - 检查 Group 中的文件列表是正确的
    - 确保没有引用不存在的文件
  - 验证工程能否正确找到所有头文件

  **Must NOT do**:
  - 不安装/运行 Keil MDK（除非环境可用）
  - 不修改 mdk 工程文件的二进制部分

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: 需要理解MDK-ARM工程结构

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on T6)
  - **Parallel Group**: Wave 2
  - **Blocks**: None (验证任务，不阻塞其他)
  - **Blocked By**: T6

  **References**:
  - `boot_update_test/MDK-ARM/` - MDK工程目录
  - `boot_update_test/Mod/` - 同步后的源文件

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Verify project file references are valid
    Tool: Bash
    Preconditions: T6 completed
    Steps:
      1. Check UVProjx file for file paths and include paths
      2. Verify each referenced .c file exists
      3. Verify each include path directory exists
    Expected Result: All file references and include paths resolve to existing files/directories
    Failure Indicators: Any referenced file or path does not exist
    Evidence: .sisyphus/evidence/task-7-project-files-valid.txt

  Scenario: Verify no broken includes
    Tool: Bash (grep)
    Preconditions: Project verified
    Steps:
      1. grep -rn "#include.*mid_eeprom" boot_update_test/
    Expected Result: grep returns empty (no broken includes)
    Failure Indicators: mid_eeprom.h still referenced
    Evidence: .sisyphus/evidence/task-7-no-broken-includes.txt
  ```

  **Evidence to Capture**:
  - [ ] task-7-project-files-valid.txt
  - [ ] task-7-no-broken-includes.txt

  **Commit**: YES (with T6, T8, T9 — Wave 2 commit)

---

- [x] 8. 创建一致性检查脚本 tool/check_mod_consistency.py

  **What to do**:
  - 创建 Python 脚本 `tool/check_mod_consistency.py`，执行以下检查并报告 PASS/FAIL：
    1. 检查 `Mod/App/app.c` 中的 extern 声明与 `Mod/BOOT1.c` 中的定义一致：
       - `bin_buf` 类型匹配（FlashBandwidthType_t[] ↔ 与生成的数组类型兼容）
       - `bin_buf_elem_len` 存在且类型兼容
    2. 检查 `Mod/` 下所有 `#include` 指令都能解析到实际文件
    3. 检查 `Mod/App/app.h` 中路径大小写正确（Mod 大写M）
    4. 检查 `Mod/External_Interfaces/api_flash_port.h` 中没有 `mid_eeprom.h`
    5. 检查 `Mod/App/app.c` 中调用了 `bsp_cmp_flash`（写入后校验）
    6. 检查 `Mod/External_Interfaces/api_flash_port.h` 包含 `FlashBandwidthType_t` 类型定义
    7. （可选）检查 `Mod/` 与 `boot_update_test/Mod/` 的内容一致性（跳过 api_flash_port.c）
  - 输出格式：每项检查一行 `[PASS/FAIL] description`，最后输出 `All checks passed` 或 `N checks failed`
  - 返回 exit code 0（全部通过）或 1（有失败）

  **Must NOT do**:
  - 不自动修复问题（只报告不修改）
  - 不删除已存在的检查项（可追加不可删）
  - 不依赖任何第三方Python库（只用标准库）

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: 编写Python自动化脚本

  **Parallelization**:
  - **Can Run In Parallel**: YES (with T6, T7, T9)
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: None (可以基于当前代码设计，但最终需要T1-T5完成后才能通过)

  **References**:
  - All Mod/* files - 检查目标
  - `Mod/BOOT1.c` - extern定义参考
  - `tool/bin2c/bin2c.py` - 脚本风格参考

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Run consistency check on current (unfixed) code — should FAIL
    Tool: Bash
    Preconditions: Script created, Mod/ code not yet fixed
    Steps:
      1. python tool/check_mod_consistency.py
    Expected Result: Reports FAIL for type mismatch and mid_eeprom.h issues
    Failure Indicators: Script passes when it should fail (defective)
    Evidence: .sisyphus/evidence/task-8-consistency-fail.txt

  Scenario: Script runs without Python errors
    Tool: Bash
    Preconditions: Script created
    Steps:
      1. python tool/check_mod_consistency.py 2>&1
    Expected Result: No Python traceback or ImportError
    Failure Indicators: Python runtime errors
    Evidence: .sisyphus/evidence/task-8-no-python-errors.txt
  ```

  **Evidence to Capture**:
  - [ ] task-8-consistency-fail.txt
  - [ ] task-8-no-python-errors.txt

  **Commit**: YES (with T6, T7, T9 — Wave 2 commit)

---

- [x] 9. 创建编译验证脚本 tool/check_compile.py

  **What to do**:
  - 创建 Python 脚本 `tool/check_compile.py`，对 Mod/ 核心代码进行编译检查：
    1. 检测系统上是否有可用的C编译器（gcc/clang）
    2. 构造编译命令对每个 .c 文件做语法检查（-fsyntax-only）
    3. 编译参数示例：`gcc -fsyntax-only -c Mod/App/app.c -I Mod/External_Interfaces/ -std=c99`
    4. 如果无编译器可用，则输出警告并尝试用预处理检查（cpp）
    5. 对每个文件输出 `[PASS/FAIL] filename — error_count`
    6. 返回 exit code 0（全部通过）或 1（有编译错误）

  **Must NOT do**:
  - 不修改任何源文件
  - 不要求特定的交叉编译器（arm-none-eabi-gcc），仅做宿主机的语法检查
  - 不安装或下载编译器

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: 编写Python自动化脚本

  **Parallelization**:
  - **Can Run In Parallel**: YES (with T6, T7, T8)
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: None

  **References**:
  - Mod/*.c files - 编译目标
  - `tool/bin2c/bin2c.py` - 脚本风格参考

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Script runs without errors
    Tool: Bash
    Preconditions: Script created
    Steps:
      1. python tool/check_compile.py 2>&1
    Expected Result: Script executes, reports some result (PASS or FAIL depending on gcc availability and code state)
    Failure Indicators: Python traceback
    Evidence: .sisyphus/evidence/task-9-compile-script-run.txt
  ```

  **Evidence to Capture**:
  - [ ] task-9-compile-script-run.txt

  **Commit**: YES (with T6, T7, T8 — Wave 2 commit)

---

- [x] 10. 编写跨平台移植指南 doc/移植指南.md

  **What to do**:
  - 创建 `doc/移植指南.md`，包含以下章节：
    1. **概述** — Mod/ 架构说明和移植目标
    2. **目录结构** — Mod/ 各文件职责说明
    3. **第一步：复制Mod/** — 将Mod/目录复制到目标平台工程
    4. **第二步：配置宏定义** — 需修改的宏列表及说明：
       - `HAL_FLASH_BASE_ADDR` — Flash起始地址
       - `HAL_FLASH_END_ADDR` — Flash结束地址
       - `HAL_FLASH_PAGE_SIZE` — Flash页大小
       - `HAL_MIN_WRITE_BAYE` — 最小写入字节数（与api_flash_write的写大小一致）
       - `HAL_BAND_WIDTH` — 带宽（1/2/4/8字节）
       - `BOOT_START_ADDR` — Boot分区地址
       - `APP_START_ADDR` — App分区地址
       - `ENABLE_GOTO_FLAG` — 可选：启用跳转标志功能
       - `HAL_GOTO_FLAG_BASE_ADDR` — 标志存储地址（如启用）
    5. **第三步：实现api_flash_port.c** — 5个必须实现的函数说明：
       - `api_flash_lock()` / `api_flash_unlock()` — Flash锁定/解锁
       - `api_flash_write()` — 写入（确保写入HAL_MIN_WRITE_BAYE字节）
       - `api_flash_page_erase()` — 页擦除
       - `api_irq_enable()` / `api_irq_disable()` — 中断控制
       - （可选）`api_flash_write_eepro()` / `api_flash_page_erase_eepro()` — EEPROM操作
    6. **第四步：生成BOOT1.c** — 使用bin2c工具的说明：
       - 准备boot.bin文件
       - 运行 `python tool/bin2c/bin2c.py`
       - 选择与HAL_BAND_WIDTH匹配的位宽
       - 将生成的.c文件放入Mod/目录
    7. **第五步：构建集成** — 将Mod/文件加入工程构建系统的说明（IncludePath设置、文件列表）
    8. **验证** — 在目标平台上运行升级流程的测试步骤
    9. **常见问题** — 可能的移植陷阱和解决方案
    10. **示例参考** — 指向 boot_update_test/ 作为参考实现
    
  **Must NOT do**:
  - 不包含专利文档内容（专利申请表是独立的）
  - 不提供具体MCU的寄存器级代码示例（只描述接口语义）
  - 不编写与实际移植无关的通用嵌入式教程

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: 技术文档编写

  **Parallelization**:
  - **Can Run In Parallel**: YES (with T6, T8, T9 — 不依赖T7)
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: T1, T2, T3, T4, T5 (需要最终API的确定状态)

  **References**:
  - All Mod/* files - 用于描述接口
  - `doc/功能描述.txt` - 系统功能描述参考
  - `boot_update_test/Mod/` - 作为具体实现示例引用

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: Verify porting guide covers all mandatory sections
    Tool: Bash (grep)
    Preconditions: doc/移植指南.md created
    Steps:
      1. Check for each required section header
    Expected Result: All sections present (概述, 第一步~第五步, 验证, 常见问题, 示例参考)
    Failure Indicators: Missing sections
    Evidence: .sisyphus/evidence/task-10-guide-sections.txt

  Scenario: Verify porting guide covers all HAL macros
    Tool: Bash (grep)
    Preconditions: Guide created
    Steps:
      1. Check for HAL_FLASH_BASE_ADDR, HAL_FLASH_END_ADDR, HAL_FLASH_PAGE_SIZE, HAL_MIN_WRITE_BAYE, HAL_BAND_WIDTH, BOOT_START_ADDR, APP_START_ADDR
    Expected Result: All 7 mandatory macros documented
    Failure Indicators: One or more macros missing from documentation
    Evidence: .sisyphus/evidence/task-10-guide-macros.txt

  Scenario: Verify porting guide covers API functions
    Tool: Bash (grep)
    Preconditions: Guide created
    Steps:
      1. Check for api_flash_lock, api_flash_unlock, api_flash_write, api_flash_page_erase, api_irq_enable, api_irq_disable
    Expected Result: All 6 API functions documented
    Failure Indicators: One or more functions missing
    Evidence: .sisyphus/evidence/task-10-guide-functions.txt
  ```

  **Evidence to Capture**:
  - [ ] task-10-guide-sections.txt
  - [ ] task-10-guide-macros.txt
  - [ ] task-10-guide-functions.txt

  **Commit**: YES
  - Message: `docs: add cross-platform porting guide`

---

## Final Verification Wave

> 4 review agents run in PARALLEL. ALL must APPROVE. Present consolidated results to user and get explicit "okay" before completing.

- [x] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. For each "Must Have": verify implementation exists (read file, grep pattern, run consistency check). For each "Must NOT Have": search codebase for forbidden patterns — reject with file:line if found. Check evidence files exist in .sisyphus/evidence/. Compare deliverables against plan.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [x] F2. **Code Quality Review** — `unspecified-high`
  Review all changed files for: C type safety issues, unused code, inconsistent naming, hardcoded paths or assumptions. Check AI slop: excessive comments, over-abstraction, generic names.
  Output: `Files [N clean/N issues] | Type Safety [PASS/FAIL] | VERDICT`

- [x] F3. **Real Manual QA — Verification Scripts** — `unspecified-high`
  Run consistency check script from clean state. Run compile verification script. Verify ALL CRITICAL and HIGH fixes via grep/read. Save evidence.
  Output: `Consistency Check [PASS/FAIL] | Compile Check [PASS/FAIL] | C1 [FIXED/FAIL] | C2 [FIXED/FAIL] | C3 [FIXED/FAIL] | C4 [FIXED/FAIL] | H1 [FIXED/FAIL] | H2 [FIXED/FAIL] | H3 [FIXED/FAIL] | VERDICT`

- [x] F4. **Scope Fidelity Check** — `deep`
  For each task: read "What to do", read actual diff (git log/diff). Verify 1:1 — everything in spec was built (no missing), nothing beyond spec was built (no creep). Check "Must NOT do" compliance. Detect cross-task contamination.
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N issues] | Unaccounted [CLEAN/N files] | VERDICT`

---

## Commit Strategy

- **Wave 1 (T1-T5)**: Single commit `fix(mod): repair type mismatches, extern declarations, file references and add write verification`
- **Wave 2 (T6-T9)**: Single commit `chore(test): sync test project, add verification scripts`
- **Wave 3 (T10)**: Single commit `docs: add cross-platform porting guide`
- **Final**: `chore: finalize after verification`

---

## Success Criteria

### Verification Commands
```bash
python tool/check_mod_consistency.py
# Expected: "All checks passed" + exit code 0

python tool/check_compile.py
# Expected: "Compile check passed" + exit code 0

# CRITICAL fix verification
grep -n "extern.*FlashBandwidthType_t" Mod/App/app.c
# Expected: shows FlashBandwidthType_t bin_buf[] not unsigned long bin_buf[]

grep -n "extern.*bin_buf_elem_len" Mod/BOOT1.c
# Expected: shows bin_buf_elem_len defined (or correct matching name)

grep -rn "mid_eeprom.h" Mod/
# Expected: returns empty (no matches)

grep -rn "#include.*bsp_flash.h" Mod/App/app.h
# Expected: shows correct case (Mod/ not mod/)

# HIGH fix verification
grep -n "bsp_cmp_flash" Mod/App/app.c
# Expected: shows bsp_cmp_flash called after bsp_flash_write

grep -n "#include.*main.h" Mod/External_Interfaces/api_flash_port.h
# Expected: NOT present (Mod/ should NOT depend on platform HAL)
```

### Final Checklist
- [x] All 4 CRITICAL fixes verified (C1-C4)
- [x] All 3 HIGH fixes verified (H1-H3)
- [x] Consistency check script passes
- [x] Compile verification script passes
- [x] boot_update_test/Mod/ synchronized with Mod/
- [x] Porting guide covers all necessary topics
