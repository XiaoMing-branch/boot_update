# Decisions - Boot固件升级系统重构

## Architecture Decisions
1. **eepro函数**: 保留在 api_flash_port.h/c 中，作为可选接口
2. **升级流程**: 擦除→写入→设置GOTO_FLAG→系统复位（标准完整流程）
3. **bin2c接口**: 按 HAL_BAND_WIDTH 位宽输出，变量名与 app.c 匹配
4. **目录关系**: Mod/ 和 boot_update_test/Mod/ 都保留，一致性脚本同步
5. **GOTO_FLAG**: 通过 #ifdef ENABLE_GOTO_FLAG 条件编译控制，地址由用户定义
6. **验证策略**: 自动编译验证 + 一致性检查脚本（非TDD）
7. **Scope OUT**: 回滚/擦除优化/printf格式/单元测试等本次不处理
