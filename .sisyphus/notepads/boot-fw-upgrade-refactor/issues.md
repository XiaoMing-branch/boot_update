# Issues - Boot固件升级系统重构

## Open Issues
- (none yet)

## Resolved Issues
- C1: bin_buf 类型不匹配 (unsigned long[] vs unsigned char[]) → T1修复
- C2: extern变量名不匹配 (bin_buf_elem_len vs bin_buf_len) → T1修复
- C3: bin2c输出格式不匹配 → T5修复
- C4: api_flash_port.h 引用了 mid_eeprom.h → T2修复
- H1: Mod/ 和 boot_update_test/Mod/ 两份代码不一致 → T6同步
- H2: 写入后缺少校验 → T1添加
- H3: 路径大小写问题 → T1修复
