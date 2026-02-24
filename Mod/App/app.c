#include "app.h"
#include "stdio.h"
extern const unsigned long bin_buf[];
extern const unsigned long bin_buf_elem_len;

static uint32_t total_pages = (APP_START_ADDR-BOOT_START_ADDR)/HAL_FLASH_PAGE_SIZE;

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
	}
	else
	{
		printf("write error\r\n");
	}
	printf("cmp flash and bin_buf...\r\n");
	if(bsp_cmp_flash(BOOT_START_ADDR,(FlashBandwidthType_t *)bin_buf,(uint32_t)bin_buf_elem_len) == FLASH_IS_EQU)
	{
		printf("update completed\r\n");
	}
	else
	{
		printf("cmp error\r\n");
	}
}

