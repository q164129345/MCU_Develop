/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
__STATIC_INLINE void Enable_Peripherals_Clock(void) {
    SET_BIT(RCC->APB2ENR, 1UL << 0UL);  // 启动AFIO时钟  // 一般的工程都要开
    SET_BIT(RCC->APB1ENR, 1UL << 28UL); // 启动PWR时钟   // 一般的工程都要开
    SET_BIT(RCC->APB2ENR, 1UL << 5UL);  // 启动GPIOD时钟 // 晶振时钟
    SET_BIT(RCC->APB2ENR, 1UL << 2UL);  // 启动GPIOA时钟 // SWD接口
    SET_BIT(RCC->APB2ENR, 1UL << 3UL);  // 启动GPIOB时钟 // 因为用了PB4
    __NOP(); // 稍微延时一下下
}

__STATIC_INLINE void GPIO_Configure(void) {
    /* PB4 */
    MODIFY_REG(GPIOB->CRL, 0x0F << 16UL, 0x08 << 16UL); // PB4输入模式
    SET_BIT(GPIOB->ODR, 0X01 << 4UL); // PB4设置上拉（因电路板设计原因，PB4外部还有一个10K的上拉电路）
}

__STATIC_INLINE void USART1_Configure(void) {
    /* 1. 使能外设时钟 */
    // RCC->APB2ENR 寄存器控制 APB2 外设时钟
    RCC->APB2ENR |= (1UL << 14UL); // 使能 USART1 时钟 (位 14)
    RCC->APB2ENR |= (1UL << 2UL);  // 使能 GPIOA 时钟 (位 2)
    /* 2. 配置 GPIO (PA9 - TX, PA10 - RX) */
    // GPIOA->CRH 寄存器控制 PA8-PA15 的模式和配置
    // PA9: 复用推挽输出 (模式: 10, CNF: 10)
    GPIOA->CRH &= ~(0xF << 4UL);        // 清零 PA9 的配置位 (位 4-7)
    GPIOA->CRH |= (0xA << 4UL);         // PA9: 10MHz 复用推挽输出 (MODE9 = 10, CNF9 = 10)
    // PA10: 浮空输入 (模式: 00, CNF: 01)
    GPIOA->CRH &= ~(0xF << 8UL);        // 清零 PA10 的配置位 (位 8-11)
    GPIOA->CRH |= (0x4 << 8UL);         // PA10: 输入模式，浮空输入 (MODE10 = 00, CNF10 = 01)
    
    /* 3. 配置 USART1 参数 */
    // (1) 设置波特率 115200 (系统时钟 72MHz, 过采样 16)
    // 波特率计算: USART_BRR = fPCLK / (16 * BaudRate)
    // 72MHz / (16 * 115200) = 39.0625
    // 整数部分: 39 (0x27), 小数部分: 0.0625 * 16 = 1 (0x1)
    USART1->BRR = (39 << 4UL) | 1;      // BRR = 0x271 (39.0625)
    // (2) 配置数据帧格式 (USART_CR1 和 USART_CR2)
    USART1->CR1 &= ~(1UL << 12UL);      // M 位 = 0, 8 位数据
    USART1->CR2 &= ~(3UL << 12UL);      // STOP 位 = 00, 1 个停止位
    USART1->CR1 &= ~(1UL << 10UL);      // 没奇偶校验
    
    // (3) 配置传输方向 (收发双向)
    USART1->CR1 |= (1UL << 3UL);        // TE 位 = 1, 使能发送
    USART1->CR1 |= (1UL << 2UL);        // RE 位 = 1, 使能接收
    // (4) 禁用硬件流控 (USART_CR3)
    USART1->CR3 &= ~(3UL << 8UL);       // CTSE 和 RTSE 位 = 0, 无流控
    // (5) 配置异步模式 (清除无关模式位)
    USART1->CR2 &= ~(1UL << 14UL);      // LINEN 位 = 0, 禁用 LIN 模式
    USART1->CR2 &= ~(1UL << 11UL);      // CLKEN 位 = 0, 禁用时钟输出
    USART1->CR3 &= ~(1UL << 5UL);       // SCEN 位 = 0, 禁用智能卡模式
    USART1->CR3 &= ~(1UL << 1UL);       // IREN 位 = 0, 禁用 IrDA 模式
    USART1->CR3 &= ~(1UL << 3UL);       // HDSEL 位 = 0, 禁用半双工
    // (6) 启用 USART
    USART1->CR1 |= (1UL << 13UL);       // UE 位 = 1, 启用 USART
}

// USART1发送函数
void USART1_SendString(const char *str) {
    /* LL库 */
//    while (*str) {
//        while (!LL_USART_IsActiveFlag_TXE(USART1));
//        LL_USART_TransmitData8(USART1, *str++);
//    }
    
    /* 寄存器方式 */
    while(*str) {
        while(!(USART1->SR & (0x01UL << 7UL))); // 等待TXE = 1
        USART1->DR = *str++;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  Enable_Peripherals_Clock(); // 启动所需外设的时钟
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  //LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
  //LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));

  /** NOJTAG: JTAG-DP Disabled and SW-DP Enabled
  */
  LL_GPIO_AF_Remap_SWJ_NOJTAG();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  SysTick->CTRL |= 0x01UL << 1UL; // 开启SysTick中断
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  //MX_GPIO_Init();
  //MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  GPIO_Configure();   // PB4
  USART1_Configure(); // USART1
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    USART1_SendString("Hello,World.\r\n");
    LL_mDelay(1000);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_2)
  {
  }
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_9);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_Init1msTick(72000000);
  LL_SetSystemCoreClock(72000000);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
