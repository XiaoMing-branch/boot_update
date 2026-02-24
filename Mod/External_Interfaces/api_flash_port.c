#include "api_flash_port.h"

/**
  * @brief  API-flash lock
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
void api_flash_lock(void)
{

}

/**
  * @brief  API-flash unlock
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
void api_flash_unlock(void)	
{

}

/**
  * @brief  API-flash writr
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
RUN_StatusTypeDef api_flash_write(uint32_t addr, void* data[])
{
	RUN_StatusTypeDef re = RUN_ERROR;

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

}

/**
  * @brief  API-disable irq
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
void api_irq_disable(void)
{

}
