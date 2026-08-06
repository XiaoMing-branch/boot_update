#include "api_flash_port.h"

/**
  * @brief  API-flash lock
  * @retval None
  */
void api_flash_lock(void)
{
    HAL_FLASH_Lock();
}

/**
  * @brief  API-flash unlock
  * @retval None
  */
void api_flash_unlock(void)
{
    HAL_FLASH_Unlock();
}

/**
  * @brief  API-flash write
  * @note   STM32F1 以 32bit 字编程(HAL_FLASH_Program WORD),单次写 1 个字;
  *         与 HAL_MIN_WRITE_BAYE=4 匹配
  * @retval RUN_OK / RUN_ERROR
  */
RUN_StatusTypeDef api_flash_write(uint32_t addr, FlashBandwidthType_t data[])
{
    RUN_StatusTypeDef re = RUN_ERROR;
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, (uint64_t)data[0]) == HAL_OK)
    {
        re = RUN_OK;
    }
    return re;
}

/**
  * @brief  API-flash page erase
  * @param  addr 擦除页的首地址(须页对齐)
  * @retval RUN_OK / RUN_ERROR
  */
RUN_StatusTypeDef api_flash_page_erase(uint32_t addr)
{
    RUN_StatusTypeDef re = RUN_ERROR;
    uint32_t PageError;
    FLASH_EraseInitTypeDef pEraseInit;

    pEraseInit.Banks        = FLASH_BANK_1;
    pEraseInit.NbPages      = 1;
    pEraseInit.PageAddress  = addr;
    pEraseInit.TypeErase    = FLASH_TYPEERASE_PAGES;
    if (HAL_FLASHEx_Erase(&pEraseInit, &PageError) == HAL_OK)
    {
        re = RUN_OK;
    }
    return re;
}

/**
  * @brief  API-flash write eepro(预留,未实现)
  * @retval RUN_ERROR
  */
RUN_StatusTypeDef api_flash_write_eepro(uint32_t addr, FlashBandwidthType_t data[])
{
    RUN_StatusTypeDef re = RUN_ERROR;
    return re;
}

/**
  * @brief  API-flash page erase eepro(预留,未实现)
  * @retval RUN_ERROR
  */
RUN_StatusTypeDef api_flash_page_erase_eepro(uint32_t addr)
{
    RUN_StatusTypeDef re = RUN_ERROR;
    return re;
}

/**
  * @brief  API-enable irq
  * @retval None
  */
void api_irq_enable(void)
{
    __enable_irq();
}

/**
  * @brief  API-disable irq
  * @retval None
  */
void api_irq_disable(void)
{
    __disable_irq();
}
