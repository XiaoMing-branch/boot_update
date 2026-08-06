#ifndef __BSP_FLASH_H__
#define __BSP_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "api_flash_port.h"
#include <string.h>

typedef enum
{
    FLASH_UGC_EQU        = 0,     //Flash内容和待写入的数据相等，不需要擦除和写操作
    FLASH_UGC_UNEQU        = 1,    //Flash不相等
}flash_cmp_t;

flash_cmp_t bsp_cmp_flash(uint32_t addr, FlashBandwidthType_t *buf, uint32_t size);
RUN_StatusTypeDef bsp_flash_write(uint32_t addr, FlashBandwidthType_t *data, uint32_t size);
RUN_StatusTypeDef bsp_flash_read(uint32_t addr, FlashBandwidthType_t *buf, uint32_t size);
RUN_StatusTypeDef bsp_flash_page_erase(uint32_t addr);
void bsp_flash_test(void);

#ifdef __cplusplus
}
#endif

#endif
