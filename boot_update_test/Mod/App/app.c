#include "app.h"
#include "usart.h"
extern const unsigned char bin_buf[];
extern const unsigned long long bin_buf_len;

static uint32_t total_pages = (APP_START_ADDR-BOOT_START_ADDR)/HAL_FLASH_PAGE_SIZE;

void boot_update(void)
{
	printf("updata start....\r\n");
	printf("Prepare to erase %d page\r\n",total_pages);
	//擦除待写入区域
	for(uint32_t i=0;i<total_pages;i++)
	{
		bsp_flash_page_erase(BOOT_START_ADDR+(i*HAL_FLASH_PAGE_SIZE));
	}
	printf("write boot bin...\r\n");
	//写入待更新boot
	bsp_flash_write(BOOT_START_ADDR,(uint8_t *)bin_buf,(uint32_t)bin_buf_len);
	//比较结果
	if(bsp_cmp_flash(BOOT_START_ADDR,(uint8_t *)bin_buf,bin_buf_len) == FLASH_IS_EQU)
	{
		printf("update completed\r\n");
	}
	else
	{
		printf("error\r\n");
	}
}
