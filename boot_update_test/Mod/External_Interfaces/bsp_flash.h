#ifndef __BSP_FLASH_H__
#define __BSP_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "api_flash_port.h"
#include <string.h>

typedef enum
{
	FLASH_IS_EQU		= 0, 	//Flash内容和待写入的数据相等，不需要擦除和写操作
	FLASH_REQ_WRITE		= 1,	//Flash不需要擦除，直接写
	FLASH_REQ_ERASE		= 2,	//Flash需要先擦除,再写
	FLASH_PARAM_ERR		= 3	    //函数参数错误
}flash_cmp_t;

flash_cmp_t bsp_cmp_flash(uint32_t addr, FlashBandwidthType_t *buf, uint32_t size);
RUN_StatusTypeDef bsp_flash_write(uint32_t addr, void *data, uint32_t size);
RUN_StatusTypeDef bsp_flash_read(uint32_t addr, FlashBandwidthType_t *buf, uint32_t size);
RUN_StatusTypeDef bsp_flash_page_erase(uint32_t addr);
void bsp_flash_test(void);

#ifdef __cplusplus
}
#endif

#endif
