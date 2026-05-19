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
RUN_StatusTypeDef api_flash_write(uint32_t addr, FlashBandwidthType_t data[])
{
	RUN_StatusTypeDef re = RUN_ERROR;
	/* USER CODE BEGIN */
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr,(uint64_t)data[0]) == HAL_OK)
	{
		re = RUN_OK;
	}
	else
	{
		re = RUN_ERROR;
	}
	/* USER CODE END */
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
	/* USER CODE BEGIN */
	uint32_t PageError;
	FLASH_EraseInitTypeDef pEraseInit;
	pEraseInit.Banks		= FLASH_BANK_1;
	pEraseInit.NbPages		= 1;
	pEraseInit.PageAddress = addr;
	pEraseInit.TypeErase	= FLASH_TYPEERASE_PAGES;
	if(HAL_FLASHEx_Erase(&pEraseInit,&PageError) == HAL_OK)
	{
		re = RUN_OK;
	}
	else
	{
		re = RUN_ERROR;
	}
	/* USER CODE END */
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
	/* USER CODE BEGIN */
	__enable_irq();
	/* USER CODE END */
}

/**
  * @brief  API-disable irq
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
void api_irq_disable(void)
{
	/* USER CODE BEGIN */
	__disable_irq();
	/* USER CODE END */
}
