#include "drv_flash.h"
#include "drv_flash_port.h"

typedef enum
{
	FLASH_IS_EQU		= 0,    //Flash内容和待写入的数据相等，不需要擦除和写操作
	FLASH_REQ_WRITE		= 1,	//Flash不需要擦除，直接写
	FLASH_REQ_ERASE		= 2,	//Flash需要先擦除,再写
	FLASH_PARAM_ERR		= 3	    //函数参数错误
}flash_cmp_t;

flash_cmp_t bsp_CmpCpuFlash(uint32_t addr, uint8_t *buf, uint32_t size)
{
	flash_cmp_t re = FLASH_PARAM_ERR;
	uint8_t eq_flag = 1;//相等标志位,先假设所有字节和待写入的数据相等，如果遇到任何一个不相等，则设置为 0
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

//uint8_t bsp_flash_write(uint32_t addr, uint8_t *data, uint32_t size)
//{
//	uint32_t i;
//	flash_cmp_t cmp_result;

//	/* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
//	if ((addr + size < HAL_FLASH_BASE_ADDR + HAL_FLASH_SIZE) && (size == 0))
//	{
//		return 1;
//	}
//	else
//	{
//		cmp_result = bsp_CmpCpuFlash(addr, data, size);
//		if (cmp_result == FLASH_IS_EQU)
//		{
//			return 0;
//		}
//		else
//		{
//			irq_disable();
//			
//			HAL_FLASH_Unlock();
//			//写入整的部分
//			for (i = 0; i < size / HAL_MIN_WRITE_baye; i++)	
//			{
//				uint64_t FlashWord[4];
//				memcpy((char *)FlashWord, data, HAL_MIN_WRITE_baye);
//				data += HAL_MIN_WRITE_baye;
//				
//				if (hal_flash_write(addr, (uint64_t)((uint32_t)FlashWord)) == HAL_OK)
//				{
//					addr = addr + HAL_MIN_WRITE_baye; /* 递增，操作下一个256bit */				
//				}		
//				else
//				{
//					goto err;
//				}
//			}
//			//写入剩余部分，其余位补0
//			if (size % HAL_MIN_WRITE_baye)
//			{
//				uint64_t FlashWord[4];
//				
//				FlashWord[0] = 0;
//				FlashWord[1] = 0;
//				FlashWord[2] = 0;
//				FlashWord[3] = 0;
//				memcpy((char *)FlashWord, data, size % HAL_MIN_WRITE_baye);
//				if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr, (uint64_t)((uint32_t)FlashWord)) == HAL_OK)
//				{
//					; // addr = addr + HAL_MIN_WRITE_baye;		
//				}		
//				else
//				{
//					goto err;
//				}
//			}
//			
//			/* Flash 加锁，禁止写Flash控制寄存器 */
//			HAL_FLASH_Lock();

//			irq_enable();  		/* 开中断 */

//			return 0;
//			
//		err:
//			/* Flash 加锁，禁止写Flash控制寄存器 */
//			HAL_FLASH_Lock();

//			irq_enable();  		/* 开中断 */

//			return 1;
//		}
//	}
//}

RUN_StatusTypeDef bsp_flash_read(uint32_t addr, uint8_t *buf, uint32_t size)
{
	RUN_StatusTypeDef re = RUN_ERROR;
	if((HAL_FLASH_BASE_ADDR <= addr) && (addr <= HAL_FLASH_END_ADDR) && (size != 0) && ((addr + size - 1) <= HAL_FLASH_END_ADDR))
	{
		for(uint32_t i=0;i<size;i++)
		{
			buf[i++] = *(uint8_t*)addr++;  
		}
		re = RUN_OK;
	}
	else
	{
		re = RUN_ERROR;
	}
	return re;
}

RUN_StatusTypeDef bsp_flash_page_erase(uint32_t addr)
{
	RUN_StatusTypeDef re = RUN_ERROR;
	//判断是否在地址范围内并且地址是每一页的首地址
	if((HAL_FLASH_BASE_ADDR <= addr) && (addr <= HAL_FLASH_END_ADDR) && (addr % HAL_FLASH_PAGE_SIZE == 0))
	{
		hal_flash_unlock();
		re = hal_flash_page_erase(addr);
		hal_flash_lock();
	}
	else
	{
		re = RUN_ERROR;
	}
	return re;
}
