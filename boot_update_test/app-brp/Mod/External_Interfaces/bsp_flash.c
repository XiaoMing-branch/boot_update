#include "bsp_flash.h"

/**
  * @brief  比较指定地址的flash与buf内容
  * @param  addr 需要比较的flash首地址
  * @param  data 比较的数据首地址
  * @param  size 比较长度
  * @note   flash_cmp_t
  * @retval None
  */
flash_cmp_t bsp_cmp_flash(uint32_t addr, FlashBandwidthType_t *buf, uint32_t size)
{
    flash_cmp_t re = FLASH_UGC_UNEQU;
    if((HAL_FLASH_BASE_ADDR <= addr) && (addr <= HAL_FLASH_END_ADDR) && (size != 0) && (addr + size * sizeof(FlashBandwidthType_t) - 1 <= HAL_FLASH_END_ADDR))
    {
    for(uint32_t i=0;i<size;i++)
    {
        FlashBandwidthType_t flash_data = *(volatile FlashBandwidthType_t *)addr;
        if(buf[i] != flash_data)
        {
            re = FLASH_UGC_UNEQU;
            return re;
        }
        addr += HAL_BAND_WIDTH;
    }
    re = FLASH_UGC_EQU;
    }
    return re;
}

/**
  * @brief  向flash写入数据
  * @param  addr 写入的flash首地址
  * @param  data 要写入数据的首地址
  * @param  size 读取长度
  * @note   None  
  * @retval None
  */
RUN_StatusTypeDef bsp_flash_write(uint32_t addr, FlashBandwidthType_t *data, uint32_t size)
{
    RUN_StatusTypeDef re = RUN_ERROR;
    
    FlashBandwidthType_t FlashWord[HAL_MIN_WRITE_BAYE/HAL_BAND_WIDTH] = {0};//用于存放不够一次写入flash的零头数据
    if ((size == 0) || (addr % HAL_MIN_WRITE_BAYE != 0) || ((addr + size * sizeof(FlashBandwidthType_t) - 1) > HAL_FLASH_END_ADDR))
    {
        re = RUN_ERROR;
    }
    else
    {
        api_irq_disable();
        api_flash_unlock();
        
        /* 注意契约:api_flash_write 单次必须写入 HAL_MIN_WRITE_BAYE 字节,
         * 移植时须与 api_flash_port.h 中的 HAL_MIN_WRITE_BAYE 保持一致,
         * 否则会出现"每隔 HAL_MIN_WRITE_BAYE 字节才写入一次"的稀疏写入 */
        for(uint32_t i=0;i<(size*HAL_BAND_WIDTH/HAL_MIN_WRITE_BAYE);i++)
        {
            if(api_flash_write(addr,data) != RUN_OK)
            {
                goto end;
            }
            data += HAL_MIN_WRITE_BAYE/HAL_BAND_WIDTH;//地址指针偏移到下一个写入点
            addr += HAL_MIN_WRITE_BAYE;
        }
        
        //写入余散数据,对长度取余得到剩下的长度
        if (size % (HAL_MIN_WRITE_BAYE/HAL_BAND_WIDTH))
        {
            uint32_t remaining_byte = (size * sizeof(FlashBandwidthType_t)) % HAL_MIN_WRITE_BAYE;//得到剩余字节数
            memset(FlashWord, 0xFF, sizeof(FlashWord));
            // 将剩余数据拷贝到FlashWord
            memcpy(FlashWord,data,remaining_byte);

            if(api_flash_write(addr,FlashWord) != RUN_OK)
            {
                goto end;
            }
        }
        
        re = RUN_OK;
        
        end:
        api_flash_lock();
        api_irq_enable();
    }
    return re;
}

/**
  * @brief  读取flash数据
  * @param  addr 读取的flash首地址
  * @param  buf  存放读取数据的首地址
  * @param  size 读取长度
  * @note   None
  * @retval None
  */
RUN_StatusTypeDef bsp_flash_read(uint32_t addr, FlashBandwidthType_t *buf, uint32_t size)
{
    RUN_StatusTypeDef re = RUN_ERROR;
    if((HAL_FLASH_BASE_ADDR <= addr) && (addr <= HAL_FLASH_END_ADDR) && (size != 0) && (addr + size * sizeof(FlashBandwidthType_t) - 1 <= HAL_FLASH_END_ADDR))
    {
		volatile FlashBandwidthType_t* n = (volatile FlashBandwidthType_t *)addr;
        for(uint32_t i=0;i<size;i++)
        {
            buf[i] = *n++;
        }
        re = RUN_OK;
    }
    return re;
}

/**
  * @brief  flash页擦除
  * @param  addr 擦除页的首地址
  * @note   None
  * @retval None
  */
RUN_StatusTypeDef bsp_flash_page_erase(uint32_t addr)
{
    RUN_StatusTypeDef re = RUN_ERROR;
    if((HAL_FLASH_BASE_ADDR <= addr) && (addr <= HAL_FLASH_END_ADDR) && (addr % HAL_FLASH_PAGE_SIZE == 0))
    {
        api_flash_unlock();
        re = api_flash_page_erase(addr);
        api_flash_lock();
    }
    else
    {
        re = RUN_ERROR;
    }
    return re;
}
