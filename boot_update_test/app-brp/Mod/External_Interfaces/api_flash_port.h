#ifndef __API_FLASH_PORT_H__
#define __API_FLASH_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "main.h"

#define HAL_FLASH_BASE_ADDR       (uint32_t)(FLASH_BASE)               //FLASH起始地址(0x08000000)
#define HAL_FLASH_END_ADDR        (uint32_t)(FLASH_BANK1_END)          //FLASH结束地址(0x0807FFFF)
#ifdef ENABLE_GOTO_FLAG
/**
  * @brief GOTO_FLAG 地址配置
  * @note  用户必须根据目标平台的实际Flash映射修改以下地址!
  *        这些地址用于存储跳转标志位,请确保不与其他Flash区域重叠。
  */
#define HAL_GOTO_FLAG_BASE_ADDR   (uint32_t)(0x0807F800U)             //跳转标志位的基地址(页对齐,按实际定义)
#define HAL_GOTO_FLAG_END_ADDR    (uint32_t)(HAL_FLASH_END_ADDR)      //跳转标志位的结束地址---请按实际定义
#define HAL_GOTO_FLAG_OFFSET      1                                    //跳转标志位偏移量（单位 FlashBandwidthType_t）
#define HAL_GOTO_FLAG_PARAM       0x00000005
#endif /* ENABLE_GOTO_FLAG */
#define HAL_FLASH_PAGE_SIZE       (1 * 2048)                          //页大小(STM32F103ZE 2KB)
#define HAL_MIN_WRITE_BAYE        4                                   //最小写入字节数---api_flash_write 单次必须写这么多字节,须与底层实现一致
#define HAL_BAND_WIDTH            4                                   //带宽字节数(FlashBandwidthType_t 的字节数)

#define HAL_FLASH_SIZE      (HAL_FLASH_END_ADDR - HAL_FLASH_BASE_ADDR + 1)    //FLASH总大小(含末尾字节)
#define HAL_FLASH_PAGE_NUMBER (HAL_FLASH_SIZE/HAL_FLASH_PAGE_SIZE)         //页数

#if (HAL_BAND_WIDTH == 1)
typedef uint8_t  FlashBandwidthType_t;
#elif (HAL_BAND_WIDTH == 2)
typedef uint16_t FlashBandwidthType_t;
#elif (HAL_BAND_WIDTH == 4)
typedef uint32_t FlashBandwidthType_t;
#elif (HAL_BAND_WIDTH == 8)
typedef uint64_t FlashBandwidthType_t;
#else
#error "HAL_BAND_WIDTH不支持1/2/4/8字节"
#endif


#ifndef HAL_FLASH_BASE_ADDR
    #error "请在当前文件或用户定义文件中定义 HAL_FLASH_BASE_ADDR，FLASH起始地址。"
#else
    // 示例值，仅供参考，用户请替换。
    //#define HAL_FLASH_BASE_ADDR     0x08000000UL  //FLASH起始地址---请按实际定义
#endif

#ifndef HAL_FLASH_END_ADDR
    #error "请在当前文件或用户定义文件中定义 HAL_FLASH_END_ADDR，FLASH结束地址。"
#else
    // 示例值，仅供参考，用户请替换。
    //#define HAL_FLASH_END_ADDR     0x0807FFFFUL  //FLASH结束地址---请按实际定义
#endif

#ifndef HAL_FLASH_PAGE_SIZE
    #error "请在当前文件或用户定义文件中定义 HAL_FLASH_PAGE_SIZE，页大小。"
#else
    // 示例值，仅供参考，用户请替换。
    //#define HAL_FLASH_PAGE_SIZE     (1 * 2048)
#endif

#ifndef HAL_MIN_WRITE_BAYE
    #error "请在当前文件或用户定义文件中定义 HAL_MIN_WRITE_baye，最小写入字节数,与bsp_flash_write接口的写大小对应。"
#else
    // 示例值，仅供参考，用户请替换。
    //#define HAL_MIN_WRITE_BAYE     4
#endif

typedef enum 
{
    RUN_OK         = 0,
    RUN_ERROR     = 1,
} RUN_StatusTypeDef;

void api_flash_lock(void);
void api_flash_unlock(void);
/* 契约: 单次必须写入 HAL_MIN_WRITE_BAYE 字节(bsp_flash_write 调一次即写满一段),addr 须按 HAL_MIN_WRITE_BAYE 对齐 */
RUN_StatusTypeDef api_flash_write(uint32_t addr, FlashBandwidthType_t data[]);
RUN_StatusTypeDef api_flash_page_erase(uint32_t addr);
void api_irq_enable(void);
void api_irq_disable(void);
RUN_StatusTypeDef api_flash_write_eepro(uint32_t addr, FlashBandwidthType_t data[]);
RUN_StatusTypeDef api_flash_page_erase_eepro(uint32_t addr);
#ifdef __cplusplus
}
#endif

#endif
