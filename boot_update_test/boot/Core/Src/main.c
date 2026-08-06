/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : BootLoader 主程序:串口 YMODEM IAP 升级 + 跳转 APP
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "bsp_flash.h"
#include "app.h"
#include "ymodem.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BOOT_VERSION        "V1.0.0"
#define BOOT_WAIT_MS        (5000U)                 /* 上电等待升级窗口: 5s */
#define APP_MAX_SIZE        (HAL_FLASH_END_ADDR - APP_START_ADDR + 1)  /* APP 区大小 */
/* USER CODE END PD */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void JumpToApp(void);
static int  CheckAppValid(void);
static int  uart_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms);
static int  uart_tx(const uint8_t *buf, uint16_t len);
static int  flash_erase(uint32_t addr, uint32_t size);
static int  flash_write(uint32_t addr, const uint8_t *data, uint32_t len);
static int  flash_verify(uint32_t addr, const uint8_t *data, uint32_t len);
/* USER CODE END PFP */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  SCB->VTOR = FLASH_BASE | 0x00000000U;             /* Boot 位于 0x08000000 */
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  uint8_t first = 0;
  int     ret;
  ymodem_handle_t ym;

  printf("\r\n===== STM32 Boot %s =====\r\n", BOOT_VERSION);
  printf("APP: 0x%08X, max %lu KB\r\n", (unsigned int)APP_START_ADDR,
         (unsigned long)APP_MAX_SIZE / 1024UL);
  printf("Waiting %u s for YMODEM upgrade...\r\n", BOOT_WAIT_MS / 1000U);

  ym.flash_addr   = APP_START_ADDR;
  ym.max_size     = APP_MAX_SIZE;
  ym.uart_rx      = uart_rx;
  ym.uart_tx      = uart_tx;
  ym.flash_erase  = flash_erase;
  ym.flash_write  = flash_write;
  ym.flash_verify = flash_verify;

  /* 等待窗口:周期发送 'C',一旦收到帧首字符即进入升级 */
  {
    uint32_t t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < BOOT_WAIT_MS)
    {
      uint8_t c = YMODEM_CRC;
      uart_tx(&c, 1);
      if (uart_rx(&first, 1, 300U) == 0)
        break;
      first = 0;                                    /* 忽略干扰字节 */
    }
  }

  if (first != 0)
  {
    ret = ymodem_receive(&ym, first);
    if (ret == YMODEM_OK)
    {
      printf("\r\nUpgrade OK, size=%lu bytes.\r\n", (unsigned long)ym.file_size);
    }
    else
    {
      printf("\r\nUpgrade failed (code=%d).\r\n", ret);
    }
  }

  if (CheckAppValid())
  {
    JumpToApp();
  }

  /* APP 无效或无固件:循环等待升级 */
  while (1)
  {
    printf("\r\nNo valid APP firmware. Waiting for YMODEM upgrade...\r\n");
    ret = ymodem_receive(&ym, 0);
    if (ret == YMODEM_OK)
    {
      printf("\r\nUpgrade OK, size=%lu bytes. Jump to APP.\r\n",
             (unsigned long)ym.file_size);
      if (CheckAppValid())
        JumpToApp();
    }
  }
  /* USER CODE END 2 */
}

/**
  * @brief System Clock Configuration (HSE 8MHz, PLL x9 -> 72MHz)
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    Error_Handler();
}

/* USER CODE BEGIN 4 */
/**
  * @brief  校验 APP 区是否存在有效固件(向量表首字为 RAM 内 MSP,复位向量在 Flash 内)
  */
static int CheckAppValid(void)
{
  uint32_t msp = *(volatile uint32_t *)APP_START_ADDR;
  uint32_t rst = *(volatile uint32_t *)(APP_START_ADDR + 4U);

  return (msp >= 0x20000000U && msp < 0x20010000U) &&
         (rst >= APP_START_ADDR && rst <= HAL_FLASH_END_ADDR);
}

/**
  * @brief  跳转到 APP 程序
  */
static void JumpToApp(void)
{
  uint32_t i = 0;
  void (*AppJump)(void);

  printf("\r\nJump to APP @ 0x%08X...\r\n", (unsigned int)APP_START_ADDR);

  api_irq_disable();

  /* 时钟切换到默认状态 */
  HAL_RCC_DeInit();
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL  = 0;

  /* 关闭所有中断,清除所有中断挂起标志 */
  for (i = 0; i < 8; i++)
  {
    NVIC->ICER[i] = 0xFFFFFFFF;
    NVIC->ICPR[i] = 0xFFFFFFFF;
  }

  api_irq_enable();

  /* 取 APP 复位中断向量并跳转 */
  AppJump = (void (*)(void))(*((uint32_t *)(APP_START_ADDR + 4U)));
  __set_MSP(*(uint32_t *)APP_START_ADDR);
  __set_CONTROL(0);
  AppJump();

  while (1)
  {
    printf("jump error\r\n");
  }
}

/* ========================= YMODEM 平台适配 ========================= */
static int uart_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms)
{
  return HAL_UART_Receive(&huart1, buf, len, timeout_ms) == HAL_OK ? 0 : -1;
}

static int uart_tx(const uint8_t *buf, uint16_t len)
{
  return HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 1000U) == HAL_OK ? 0 : -1;
}

static int flash_erase(uint32_t addr, uint32_t size)
{
  uint32_t pages = (size + HAL_FLASH_PAGE_SIZE - 1U) / HAL_FLASH_PAGE_SIZE;
  uint32_t i;

  for (i = 0; i < pages; i++)
  {
    if (bsp_flash_page_erase(addr + i * HAL_FLASH_PAGE_SIZE) != RUN_OK)
      return -1;
  }
  return 0;
}

static int flash_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
  uint32_t n = (len + 3U) & ~3U;       /* 对齐到 4 字节(FLASH 为擦除态 0xFF,补齐无害) */
  if (n == 0)
    return 0;
  return bsp_flash_write(addr, (FlashBandwidthType_t *)data,
                         n / sizeof(FlashBandwidthType_t)) == RUN_OK ? 0 : -1;
}

static int flash_verify(uint32_t addr, const uint8_t *data, uint32_t len)
{
  uint32_t n = (len + 3U) & ~3U;
  if (n == 0)
    return 0;
  return bsp_cmp_flash(addr, (FlashBandwidthType_t *)data,
                       n / sizeof(FlashBandwidthType_t)) == FLASH_UGC_EQU ? 0 : -1;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
