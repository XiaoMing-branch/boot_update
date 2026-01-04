#include "app.h"

typedef enum
{
	update_ready = 0,//进行一些初始化流程
	update_underway, //开始进行boot程序更新
	update_finish,   //完成
	update_error,    //失败
	update_leisure   //空闲
}boot_update_status;

void boot_update_task(void)
{
	switch(boot_update_status)
	{
		case update_ready:
			break;
		
		case update_underway:
			break;
		
		case update_finish:
			break;
		
		case update_error:
			break;
		
		default:break;
	}
}
