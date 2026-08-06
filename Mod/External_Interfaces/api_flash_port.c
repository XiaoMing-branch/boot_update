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
RUN_StatusTypeDef api_flash_write(uint32_t addr, FlashBandwidthType_t data[])
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
  * @brief  API-flash write eepro
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
/* USER CODE BEGIN 1 */
RUN_StatusTypeDef api_flash_write_eepro(uint32_t addr, FlashBandwidthType_t data[])
{
    RUN_StatusTypeDef re = RUN_ERROR;

    return re;
}
/* USER CODE END 1 */

/**
  * @brief  API-flash page erase eepro
  * @param  None
  * @note   在使用该模块前,需先将底层函数放入接口函数中
  * @retval None
  */
/* USER CODE BEGIN 2 */
RUN_StatusTypeDef api_flash_page_erase_eepro(uint32_t addr)
{
    RUN_StatusTypeDef re = RUN_ERROR;

    return re;
}
/* USER CODE END 2 */

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
