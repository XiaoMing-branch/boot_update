#include "drv_flash_port.h"
#include "main.h"
/*
*********************************************************************************************************
*	函 数 名: hal_flash_lock
*	功能说明: 中间层接口函数
*********************************************************************************************************
*/
void hal_flash_lock(void)
{
	HAL_FLASH_Lock();
}
/*
*********************************************************************************************************
*	函 数 名: hal_flash_unlock
*	功能说明: 中间层接口函数
*********************************************************************************************************
*/
void hal_flash_unlock(void)	
{
	HAL_FLASH_Unlock();
}
/*
*********************************************************************************************************
*	函 数 名: hal_flash_write
*	功能说明: 中间层接口函数
*********************************************************************************************************
*/
RUN_StatusTypeDef hal_flash_write(uint32_t addr, uint8_t* data)
{
	RUN_StatusTypeDef re = RUN_ERROR;
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, (uint64_t)((uint32_t)data)) == HAL_OK)
	{
		re = RUN_OK;
	}
	else
	{
		re = RUN_ERROR;
	}
	return re;
}
/*
*********************************************************************************************************
*	函 数 名: hal_flash_read
*	功能说明: 中间层接口函数
*********************************************************************************************************
*/
void hal_flash_read(void)
{
	
}
/*
*********************************************************************************************************
*	函 数 名: hal_flash_page_erase
*	功能说明: 中间层接口函数
*********************************************************************************************************
*/
RUN_StatusTypeDef hal_flash_page_erase(uint32_t addr)
{
	RUN_StatusTypeDef re = RUN_ERROR;
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError = 0;
	
	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks         = FLASH_BANK_1;
	EraseInitStruct.PageAddress   = addr;
	EraseInitStruct.NbPages     = 1;
	
	if(HAL_FLASHEx_Erase(&EraseInitStruct,&SECTORError) == HAL_OK)
	{
		re = RUN_ERROR;
	}
	return re;
}

void irq_enable(void)
{
	__set_PRIMASK(0);
}

void irq_disable(void)
{
	__set_PRIMASK(1);
}
