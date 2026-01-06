#ifndef __API_FLASH_PORT_H__
#define __API_FLASH_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stdint.h"

#define HAL_FLASH_BASE_ADDR      (uint32_t)(FLASH_BASE)		 			//FLASH起始地址---需按照实际定义
#define HAL_FLASH_END_ADDR       (uint32_t)(FLASH_BANK1_END) 			//FLASH结束地址---需按照实际定义
#define HAL_FLASH_SIZE      (HAL_FLASH_END_ADDR - HAL_FLASH_BASE_ADDR)	//FLASH总容量
#define HAL_FLASH_PAGE_SIZE	(1 * 1024)									//页大小---需按照实际定义
#define HAL_FLASH_PAGE_NUMBER (HAL_FLASH_SIZE/HAL_FLASH_PAGE_SIZE) 		//页数
#define HAL_MIN_WRITE_baye 4 											//最小写入字节数---需与bsp_flash_write接口的写入量对应

typedef enum 
{
	RUN_OK 		= 0,
	RUN_ERROR 	= 1,
} RUN_StatusTypeDef;

void api_flash_lock(void);
void api_flash_unlock(void);
RUN_StatusTypeDef api_flash_write(uint32_t addr, uint64_t data);
RUN_StatusTypeDef api_flash_page_erase(uint32_t addr);
void api_irq_enable(void);
void api_irq_disable(void);

#ifdef __cplusplus
}
#endif

#endif
