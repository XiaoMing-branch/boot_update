#include "app.h"
#include "stdio.h"
extern const FlashBandwidthType_t bin_buf[];
extern const unsigned long long bin_buf_elem_len;

static uint32_t total_pages = (APP_START_ADDR-BOOT_START_ADDR)/HAL_FLASH_PAGE_SIZE;

FlashBandwidthType_t read_buf[HAL_MIN_WRITE_BAYE];

void boot_update(void)
{
	printf("updata start....\r\n");
	printf("Prepare to erase %lu page\r\n",total_pages);
	for(uint32_t i=0;i<total_pages;i++)
	{
		bsp_flash_page_erase(BOOT_START_ADDR+(i*HAL_FLASH_PAGE_SIZE));
	}
	printf("write boot bin in all %lu...\r\n",bin_buf_elem_len);
	if(bsp_flash_write(BOOT_START_ADDR,(FlashBandwidthType_t *)bin_buf,(uint32_t)bin_buf_elem_len) == RUN_OK)
	{
		printf("write ok\r\n");
		if(bsp_cmp_flash(BOOT_START_ADDR, bin_buf, (uint32_t)bin_buf_elem_len) == FLASH_UGC_EQU)
		{
			printf("cmp ok\r\n");
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
    
#ifdef ENABLE_GOTO_FLAG
	printf("read flag zone:\r\n");
    bsp_flash_read(HAL_GOTO_FLAG_BASE_ADDR,read_buf,HAL_MIN_WRITE_BAYE);
    for(uint32_t i=0;i<HAL_MIN_WRITE_BAYE;i++)
    {
        printf("0x%08x  ",read_buf[i]);
    }
	printf("\r\n");

    printf("erase flag\r\n");
    bsp_flash_page_erase(HAL_GOTO_FLAG_BASE_ADDR);
    
    read_buf[HAL_GOTO_FLAG_OFFSET] = HAL_GOTO_FLAG_PARAM;
    printf("write\r\n");
    bsp_flash_write(HAL_GOTO_FLAG_BASE_ADDR,read_buf,HAL_MIN_WRITE_BAYE);

    printf("read flag zone verify:\r\n");
    bsp_flash_read(HAL_GOTO_FLAG_BASE_ADDR,read_buf,HAL_MIN_WRITE_BAYE);
    for(uint32_t i=0;i<HAL_MIN_WRITE_BAYE;i++)
    {
        printf("0x%08x ",read_buf[i]);
    }
	printf("\r\n");

	printf("go to boot\r\n");
    NVIC_SystemReset();
#endif
}
