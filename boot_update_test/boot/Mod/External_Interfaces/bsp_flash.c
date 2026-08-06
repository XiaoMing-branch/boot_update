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
  * @param  data 存放写入数据的首地址
  * @param  size 读取长度
  * @note   None  
  * @retval None
  */
RUN_StatusTypeDef bsp_flash_write(uint32_t addr, FlashBandwidthType_t *data, uint32_t size)
{
	RUN_StatusTypeDef re = RUN_ERROR;
	
	FlashBandwidthType_t FlashWord[HAL_MIN_WRITE_BAYE/HAL_BAND_WIDTH] = {0};//用于存放不够一次写入flash的数据载体
	if ((size == 0) || (addr % HAL_MIN_WRITE_BAYE != 0) || (addr + size * sizeof(FlashBandwidthType_t) > HAL_FLASH_BASE_ADDR + HAL_FLASH_SIZE))
	{
		re = RUN_ERROR;
	}
	else
	{
		api_irq_disable();
		api_flash_unlock();
		
		//写入整数部分,底层接口函数为HAL_MIN_WRITE_baye字节写入 实际写入次数 = 总字节/写入字节/带宽
		for(uint32_t i=0;i<(size*HAL_BAND_WIDTH/HAL_MIN_WRITE_BAYE);i++)
		{
			if(api_flash_write(addr,data) != RUN_OK)
			{
				goto end;
			}
			data += HAL_MIN_WRITE_BAYE/HAL_BAND_WIDTH;//地址往后偏移到下一个变量
			addr += HAL_MIN_WRITE_BAYE;
		}
		
		//写入零散部分,对长度取余数得到剩下的长度
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
  * @brief  读取flash内容
  * @param  addr 读取的flash首地址
  * @param  buf  存放读取数据的首地址
  * @param  size 读取长度
  * @note   None
  * @retval None
  */
RUN_StatusTypeDef bsp_flash_read(uint32_t addr, FlashBandwidthType_t *buf, uint32_t size)
{
	RUN_StatusTypeDef re = RUN_ERROR;
	if((HAL_FLASH_BASE_ADDR <= addr) && (addr <= HAL_FLASH_END_ADDR) && (size != 0) && ((addr + size - 1) <= HAL_FLASH_END_ADDR))
	{
		for(uint32_t i=0;i<size;i++)
		{
			buf[i] = *(uint8_t*)addr++;  
		}
		re = RUN_OK;
	}
	else
	{
		re = RUN_ERROR;
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

/**
  * @brief  用于flash功能验证
  * @param  None
  * @note   None
  * @retval None
  */
void bsp_flash_test(void)
{
	FlashBandwidthType_t data_32[13] = 
	{
		// 0x0102（高16位） + 0x0304（低16位） → 0x01020304
		0x01020304,
		// 0x0506 + 0x0708 → 0x05060708
		0x05060708,
		// 0x090A + 0x0B0C → 0x090A0B0C
		0x090A0B0C,
		// 0x0D0E + 0x0F10 → 0x0D0E0F10
		0x0D0E0F10,
		// 0x1112 + 0x1314 → 0x11121314
		0x11121314,
		// 0x1516 + 0x1718 → 0x15161718
		0x15161718,
		// 0x191A + 0x1B1C → 0x191A1B1C
		0x191A1B1C,
		// 0x1D1E + 0x1F20 → 0x1D1E1F20
		0x1D1E1F20,
		// 0x2122 + 0x2324 → 0x21222324
		0x21222324,
		// 0x2526 + 0x2728 → 0x25262728
		0x25262728,
		// 0x292A + 0x2B2C → 0x292A2B2C
		0x292A2B2C,
		// 0x2D2E + 0x2F30 → 0x2D2E2F30
		0x2D2E2F30,
		// 剩余1个uint16_t（0x3132） + 补0 → 0x31320000
		0x31320000
	};
	FlashBandwidthType_t r_data[25];
	printf("开始擦除\r\n");
	bsp_flash_page_erase(0x08007C00);
	printf("擦除完成\r\n");
	//读取测试
	bsp_flash_read(HAL_FLASH_BASE_ADDR+0x7c00,r_data,25);
	for(uint8_t i=0;i<25;i++)
	{
		printf("读取第%d个数值：%x\r\n",i,r_data[i]);
	}
	//写入测试
	printf("开始写入\r\n");
	bsp_flash_write(HAL_FLASH_BASE_ADDR+0x7c00,data_32,13);
	printf("写入完成\r\n");
	
	printf("cmp:%d\r\n",bsp_cmp_flash(0x08007C00,data_32,52));
	//写入验证
	bsp_flash_read(HAL_FLASH_BASE_ADDR+0x7c00,r_data,25);
	for(uint8_t i=0;i<25;i++)
	{
		printf("读取第%d个数值：%x\r\n",i,r_data[i]);
	}
}
