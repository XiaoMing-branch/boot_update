#ifndef __DRV_FLASH_H__
#define __DRV_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stdint.h"

#define HAL_FLASH_BASE_ADDR      (uint32_t)(FLASH_BASE)		 //FLASH起始地址
#define HAL_FLASH_END_ADDR       (uint32_t)(FLASH_BANK1_END) //FLASH结束地址

#define HAL_FLASH_SIZE      (HAL_FLASH_BASE_ADDR - HAL_FLASH_END_ADDR)	/* FLASH总容量 */
#define HAL_FLASH_PAGE_SIZE	(1 * 1024)		//页大小
#define HAL_FLASH_PAGE_NUMBER (HAL_FLASH_SIZE/HAL_FLASH_PAGE_SIZE) //页数
#define HAL_MIN_WRITE_baye 4

#ifdef __cplusplus
}
#endif

#endif
