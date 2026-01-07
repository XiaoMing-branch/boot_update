#ifndef __APP_H__
#define __APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_flash.h"

#ifndef APP_START_ADDR
    #error "请在当前文件或用户配置文件中定义 APP_START_ADDR（APP起始地址）"
#else
    // 示例值（仅作参考，用户需替换）
    //#define HAL_FLASH_BASE_ADDR     0x08000000UL  //FLASH起始地址---需按照实际定义
#endif

#ifndef BOOT_START_ADDR
    #error "请在当前文件或用户配置文件中定义 BOOT_START_ADDR（BOOT起始地址）"
#else
    // 示例值（仅作参考，用户需替换）
    //#define HAL_FLASH_BASE_ADDR     0x08000000UL  //FLASH起始地址---需按照实际定义
#endif

void boot_update(void);

#ifdef __cplusplus
}
#endif

#endif
