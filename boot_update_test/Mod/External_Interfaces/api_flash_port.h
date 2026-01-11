#ifndef __API_FLASH_PORT_H__
#define __API_FLASH_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mid_eeprom.h"

#define HAL_FLASH_BASE_ADDR      (uint32_t)(0x00000000U)		 		//FLASH起始地址---需按照实际定义
#define HAL_FLASH_END_ADDR       (uint32_t)(0x00020000U) 			    //FLASH结束地址---需按照实际定义
#define HAL_FLASH_PAGE_SIZE	     (1 * 64)								//页大小---需按照实际定义
#define HAL_MIN_WRITE_baye        64 									//最小写入字节数---需与bsp_flash_write接口的写入量对应
#define HAL_BAND_WIDTH            4                                     //带宽字节数

#define HAL_FLASH_SIZE      (HAL_FLASH_END_ADDR - HAL_FLASH_BASE_ADDR)	//FLASH总容量
#define HAL_FLASH_PAGE_NUMBER (HAL_FLASH_SIZE/HAL_FLASH_PAGE_SIZE) 		//页数

#if (HAL_BAND_WIDTH == 1)
typedef uint8_t  FlashBandwidthType_t;
#elif (HAL_BAND_WIDTH == 2)
typedef uint16_t FlashBandwidthType_t;
#elif (HAL_BAND_WIDTH == 4)
typedef uint32_t FlashBandwidthType_t;
#elif (HAL_BAND_WIDTH == 8)
typedef uint64_t FlashBandwidthType_t;
#else
#error "HAL_BAND_WIDTH仅支持1/2/4/8字节"
#endif


#ifndef HAL_FLASH_BASE_ADDR
    #error "请在当前文件或用户配置文件中定义 HAL_FLASH_BASE_ADDR（FLASH起始地址）"
#else
    // 示例值（仅作参考，用户需替换）
    //#define HAL_FLASH_BASE_ADDR     0x08000000UL  //FLASH起始地址---需按照实际定义
#endif

#ifndef HAL_FLASH_END_ADDR
    #error "请在当前文件或用户配置文件中定义 HAL_FLASH_END_ADDR（FLASH结束地址）"
#else
    // 示例值（仅作参考，用户需替换）
    //#define HAL_FLASH_END_ADDR     0x0807FFFFUL  //FLASH结束地址---需按照实际定义
#endif

#ifndef HAL_FLASH_PAGE_SIZE
    #error "请在当前文件或用户配置文件中定义 HAL_FLASH_PAGE_SIZE（页大小）"
#else
    // 示例值（仅作参考，用户需替换）
    //#define HAL_FLASH_PAGE_SIZE     (1 * 2048)
#endif

#ifndef HAL_MIN_WRITE_baye
    #error "请在当前文件或用户配置文件中定义 HAL_MIN_WRITE_baye（最小写入字节数,需与bsp_flash_write接口的写入量对应）"
#else
    // 示例值（仅作参考，用户需替换）
    //#define HAL_MIN_WRITE_baye     4  
#endif

typedef enum 
{
	RUN_OK 		= 0,
	RUN_ERROR 	= 1,
} RUN_StatusTypeDef;

void api_flash_lock(void);
void api_flash_unlock(void);
RUN_StatusTypeDef api_flash_write(uint32_t addr, void* data[]);
RUN_StatusTypeDef api_flash_page_erase(uint32_t addr);
void api_irq_enable(void);
void api_irq_disable(void);

#ifdef __cplusplus
}
#endif

#endif
