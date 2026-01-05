#ifndef __DRV_FLASH_PORT_H__
#define __DRV_FLASH_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stdint.h"

typedef enum 
{
	RUN_OK = 0,
	RUN_ERROR = 1,
} RUN_StatusTypeDef;

void hal_flash_lock(void);
void hal_flash_unlock(void);
RUN_StatusTypeDef hal_flash_write(uint32_t addr, uint8_t* data);
void hal_flash_read(void);
RUN_StatusTypeDef hal_flash_page_erase(uint32_t addr);
void irq_enable(void);
void irq_disable(void);
#ifdef __cplusplus
}
#endif

#endif
