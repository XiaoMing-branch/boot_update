#include "app.h"
#include "stdio.h"
extern const FlashBandwidthType_t bin_buf[];
extern const unsigned long long bin_buf_elem_len;

static uint32_t total_pages = (APP_START_ADDR-BOOT_START_ADDR)/HAL_FLASH_PAGE_SIZE;

#ifdef ENABLE_GOTO_FLAG
static FlashBandwidthType_t read_buf[HAL_MIN_WRITE_BAYE];
#endif

void boot_update(void)
{
    uint32_t boot_bytes = APP_START_ADDR - BOOT_START_ADDR;
    uint32_t bin_bytes  = (uint32_t)(bin_buf_elem_len * sizeof(FlashBandwidthType_t));

    printf("updata start....\r\n");

    /* 写前校验:固件非空且必须能放进 boot 区,否则直接返回,避免误擦除导致变砖 */
    if ((bin_buf_elem_len == 0) || (bin_bytes > boot_bytes))
    {
        printf("boot bin size invalid: %lu B > region %lu B, abort\r\n",
               (unsigned long)bin_bytes, (unsigned long)boot_bytes);
        return;
    }

    printf("Prepare to erase %lu page\r\n",(unsigned long)total_pages);
    for(uint32_t i=0;i<total_pages;i++)
    {
        if(bsp_flash_page_erase(BOOT_START_ADDR+(i*HAL_FLASH_PAGE_SIZE)) != RUN_OK)
        {
            printf("erase error\r\n");
            return;
        }
    }
    printf("write boot bin in all %lu...\r\n",(unsigned long)bin_buf_elem_len);
    if(bsp_flash_write(BOOT_START_ADDR,(FlashBandwidthType_t *)bin_buf,(uint32_t)bin_buf_elem_len) == RUN_OK)
    {
        printf("write ok\r\n");
		if(bsp_cmp_flash(BOOT_START_ADDR, (FlashBandwidthType_t *)bin_buf, (uint32_t)bin_buf_elem_len) == FLASH_UGC_EQU)
        {
            printf("cmp ok\r\n");
#ifdef ENABLE_GOTO_FLAG
            /* 仅在校验通过后才写跳转标志并复位,避免把写入失败的固件当成功 */
            printf("read flag zone:\r\n");
            bsp_flash_read(HAL_GOTO_FLAG_BASE_ADDR,read_buf,HAL_MIN_WRITE_BAYE);
            for(uint32_t i=0;i<HAL_MIN_WRITE_BAYE;i++)
            {
                printf("0x%08x  ",(unsigned int)read_buf[i]);
            }
            printf("\r\n");

            printf("erase flag\r\n");
            if(bsp_flash_page_erase(HAL_GOTO_FLAG_BASE_ADDR) != RUN_OK)
            {
                printf("erase flag error\r\n");
                return;
            }

            read_buf[HAL_GOTO_FLAG_OFFSET] = HAL_GOTO_FLAG_PARAM;
            printf("write\r\n");
            if(bsp_flash_write(HAL_GOTO_FLAG_BASE_ADDR,read_buf,HAL_MIN_WRITE_BAYE) != RUN_OK)
            {
                printf("write flag error\r\n");
                return;
            }

            printf("read flag zone verify:\r\n");
            bsp_flash_read(HAL_GOTO_FLAG_BASE_ADDR,read_buf,HAL_MIN_WRITE_BAYE);
            for(uint32_t i=0;i<HAL_MIN_WRITE_BAYE;i++)
            {
                printf("0x%08x ",(unsigned int)read_buf[i]);
            }
            printf("\r\n");

            printf("go to boot\r\n");
            NVIC_SystemReset();
#endif
        }
        else
        {
            printf("cmp error\r\n");
        }
    }
    else
    {
        printf("write error\r\n");
    }
}
