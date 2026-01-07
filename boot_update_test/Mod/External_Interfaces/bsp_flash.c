#include "bsp_flash.h"
#include "usart.h"
/**
  * @brief  比较指定地址的flash与buf内容
  * @param  addr 需要比较的flash首地址
  * @param  data 比较的数据
  * @param  size 比较长度
  * @note   flash_cmp_t
  * @retval None
  */
flash_cmp_t bsp_cmp_flash(uint32_t addr, uint8_t *buf, uint32_t size)
{
	flash_cmp_t re = FLASH_PARAM_ERR;
	uint8_t eq_flag = 1;
	uint8_t now_byte = 0;
	uint8_t exit_flag = 0;

	//如果偏移地址超过芯片容量，则不改写输出缓冲区
	if((addr + size < HAL_FLASH_BASE_ADDR + HAL_FLASH_SIZE) && (size != 0))
	{
		for (uint32_t i = 0; i < size && !exit_flag; i++)
		{
			now_byte = *(uint8_t *)addr;
			//判断当前地址数值与buf是否相等
			if (now_byte != *buf)
			{
				//判断该地址编程状态
				if (now_byte != 0xFF)
				{
					re = FLASH_REQ_ERASE;
					exit_flag = 1;
				}
				else
				{
					eq_flag = 0;
				}
			}
			addr++;
			buf++;
		}

		if(!exit_flag)
		{
			if (eq_flag == 1)
			{
				re = FLASH_IS_EQU;
			}
			else
			{
				re = FLASH_REQ_WRITE;
			}
		}
	}
	else
	{
		re = FLASH_PARAM_ERR;
	}
	return re;
}

/**
  * @brief  向flash写入数据
  * @param  addr 写入的flash首地址
  * @param  data 存放读取数据的容器
  * @param  size 读取长度
  * @note   None  
  * @retval None
  */
RUN_StatusTypeDef bsp_flash_write(uint32_t addr, uint8_t *data, uint32_t size)
{
	RUN_StatusTypeDef re = RUN_ERROR;
	flash_cmp_t cmp_result = FLASH_PARAM_ERR;
	uint64_t FlashWord = 0;//用于存放即将写入flash的变量载体
	/* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
	if ((addr + size > HAL_FLASH_BASE_ADDR + HAL_FLASH_SIZE) || (size == 0) || (addr % HAL_MIN_WRITE_baye != 0))
	{
		re = RUN_ERROR;
	}
	else
	{
		cmp_result = bsp_cmp_flash(addr, data, size);
		if (cmp_result == FLASH_IS_EQU)
		{
			re = RUN_OK;
		}
		else
		{
			api_irq_disable();
			api_flash_unlock();
			
			//写入整数部分,底层接口函数为4字节写入 实际写入次数 = 总字节/4字节
			for(uint32_t i=0;i<(size/HAL_MIN_WRITE_baye);i++)
			{
				FlashWord = 0;
				memcpy(&FlashWord,data,HAL_MIN_WRITE_baye);//将uint8_t数组的内容拷贝4个元素组合到uint32中
				if(api_flash_write(addr, FlashWord) != RUN_OK)
				{
					goto end;
				}
				data += HAL_MIN_WRITE_baye;//地址往后偏移到下一个变量
				addr += HAL_MIN_WRITE_baye;
			}
			
			//写入零散部分,对长度取余数得到剩下的长度
			if (size % HAL_MIN_WRITE_baye)
			{
				FlashWord = 0;
				memcpy(&FlashWord,data,size % HAL_MIN_WRITE_baye);
				if(api_flash_write(addr, FlashWord) != RUN_OK)
				{
					goto end;
				}
			}
			
			re = RUN_OK;
			
			end:
			api_flash_lock();
			api_irq_enable();/* 开中断 */
		}
	}
	return re;
}

/**
  * @brief  读取flash内容
  * @param  addr 读取的flash首地址
  * @param  buf  存放读取数据的容器
  * @param  size 读取长度
  * @note   None
  * @retval None
  */
RUN_StatusTypeDef bsp_flash_read(uint32_t addr, uint8_t *buf, uint32_t size)
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
	//判断是否在地址范围内并且地址是每一页的首地址
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
	uint8_t data_50[50] = 
	{
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
		0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
		0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E,
		0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
		0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32
	};
	
	uint8_t r_data[50];
	printf("开始擦除\r\n");
	bsp_flash_page_erase(0x08007C00);
	printf("擦除完成\r\n");
	//读取测试
	bsp_flash_read(HAL_FLASH_BASE_ADDR+0x7c00,r_data,50);
	for(uint8_t i=0;i<50;i++)
	{
		printf("读取第%d个数值：%x\r\n",i,r_data[i]);
	}
	//写入测试
	printf("开始写入\r\n");
	bsp_flash_write(HAL_FLASH_BASE_ADDR+0x7c00,data_50,50);
	printf("写入完成\r\n");
	//写入验证
	bsp_flash_read(HAL_FLASH_BASE_ADDR+0x7c00,r_data,50);
	for(uint8_t i=0;i<50;i++)
	{
		printf("读取第%d个数值：%x\r\n",i,r_data[i]);
	}
}
