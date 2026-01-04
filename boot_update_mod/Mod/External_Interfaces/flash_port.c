#include "flash_port.h"
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
RUN_StatusTypeDef hal_flash_write(uint32_t addr, uint8_t* data, uint32_t size)
{
	
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

RUN_StatusTypeDef bsp_flash_write(uint32_t addr, uint8_t* data, uint32_t size)
{
	
}

void bsp_flash_read(void)
{
	
}

void bsp_flash_page_erase(uint32_t addr)
{
	//判断是否在地址范围内并且地址是每一页的首地址
	if((HAL_FLASH_BASE_ADDR <= addr) && (addr <= HAL_FLASH_END_ADDR) && (addr % HAL_FLASH_PAGE_SIZE == 0))
	{
		hal_flash_unlock();
		
		hal_flash_page_erase(addr);
		
		hal_flash_lock();
	}
}
