#include "api_flash_port.h"

/**
  * @brief  API-flash lock
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
void api_flash_lock(void)
{
	HAL_FLASH_Lock();
}

/**
  * @brief  API-flash unlock
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
void api_flash_unlock(void)	
{
	HAL_FLASH_Unlock();
}

/**
  * @brief  API-flash writr
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
RUN_StatusTypeDef api_flash_write(uint32_t addr, uint64_t data)
{
	RUN_StatusTypeDef re = RUN_ERROR;
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr,data) == HAL_OK)
	{
		re = RUN_OK;
	}
	else
	{
		re = RUN_ERROR;
	}
	return re;
}

/**
  * @brief  API-flash page erase
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
RUN_StatusTypeDef api_flash_page_erase(uint32_t addr)
{
	RUN_StatusTypeDef re = RUN_ERROR;
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError = 0;
	
	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks         = FLASH_BANK_1;
	EraseInitStruct.PageAddress   = addr;
	EraseInitStruct.NbPages     = 1;
	
	api_irq_disable();
	if(HAL_FLASHEx_Erase(&EraseInitStruct,&SECTORError) != HAL_OK)
	{
		re = RUN_ERROR;
	}
	api_irq_enable();
	return re;
}

/**
  * @brief  API-enable irq
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
void api_irq_enable(void)
{
	__set_PRIMASK(0);
}

/**
  * @brief  API-disable irq
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
void api_irq_disable(void)
{
	__set_PRIMASK(1);
}
