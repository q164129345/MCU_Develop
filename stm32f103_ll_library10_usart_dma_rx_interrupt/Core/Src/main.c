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
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_complete = 0;
volatile uint16_t recvd_length = 0;

volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
volatile uint8_t tx_dma_busy = 0;
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
#if (USE_LL_LIBRARY == 0)
// 外设时钟
__STATIC_INLINE void Enable_Peripherals_Clock(void) {
    SET_BIT(RCC->APB2ENR, 1UL << 0UL);  // 启动AFIO时钟  // 一般的工程都要开
    SET_BIT(RCC->APB1ENR, 1UL << 28UL); // 启动PWR时钟   // 一般的工程都要开
    SET_BIT(RCC->APB2ENR, 1UL << 5UL);  // 启动GPIOD时钟 // 晶振时钟
    SET_BIT(RCC->APB2ENR, 1UL << 2UL);  // 启动GPIOA时钟 // SWD接口
    __NOP(); // 稍微延时一下下
}

// 配置DMA1的通道4：普通模式，内存到外设(flash->USART1_TX)，优先级高，存储器地址递增、数据大小都是8bit
__STATIC_INLINE void DMA1_Channel4_Configure(void) {
    // 开启时钟
    RCC->AHBENR |= (1UL << 0UL); // 开启DMA1时钟
    // 设置并开启全局中断
    NVIC_SetPriority(DMA1_Channel4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),2, 0));
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    // 数据传输方向
    DMA1_Channel4->CCR &= ~(1UL << 14UL); // 外设到存储器模式
    DMA1_Channel4->CCR |= (1UL << 4UL); // DIR位设置1（内存到外设）
    // 通道优先级
    DMA1_Channel4->CCR &= ~(3UL << 12UL); // 清除PL位
    DMA1_Channel4->CCR |= (2UL << 12UL);  // PL位设置10，优先级高
    // 循环模式
    DMA1_Channel4->CCR &= ~(1UL << 5UL);  // 清除CIRC位，关闭循环模式
    // 增量模式
    DMA1_Channel4->CCR &= ~(1UL << 6UL);  // 外设存储地址不递增
    DMA1_Channel4->CCR |= (1UL << 7UL);   // 存储器地址递增
    // 数据大小
    DMA1_Channel4->CCR &= ~(3UL << 8UL);  // 外设数据宽度8位，清除PSIZE位，相当于00
    DMA1_Channel4->CCR &= ~(3UL << 10UL); // 存储器数据宽度8位，清除MSIZE位，相当于00
    // 中断
    DMA1_Channel4->CCR |= (1UL << 1UL);   // 开启发送完成中断
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
    // 开启USART1全局中断
    NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0)); // 优先级1（优先级越低相当于越优先）
    NVIC_EnableIRQ(USART1_IRQn);
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
    // (6) 中断
    USART1->CR1 |= (1UL << 4UL);        // 使能USART1空闲中断 (IDLEIE, 位4)
    // (7) 关联DMA的接收请求
    USART1->CR3 |= (1UL << 6UL);        // 使能USART1的DMA接收请求（DMAR，位6） 
    // (7) 启用 USART
    USART1->CR1 |= (1UL << 13UL);       // UE 位 = 1, 启用 USART
}

// 配置USART1_RX的DMA1通道5
__STATIC_INLINE void DMA1_Channel5_Configure(void) {
    // 时钟
    RCC->AHBENR |= (1UL << 0UL); // 开启DMA1时钟
    // 开启全局中断
    NVIC_SetPriority(DMA1_Channel5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    /* 1. 禁用DMA通道5，等待其完全关闭 */
    DMA1_Channel5->CCR &= ~(1UL << 0);  // 清除EN位
    while(DMA1_Channel5->CCR & 1UL);    // 等待DMA通道5关闭

    /* 2. 配置外设地址和存储器地址 */
    DMA1_Channel5->CPAR = (uint32_t)&USART1->DR;  // 外设地址为USART1数据寄存器
    DMA1_Channel5->CMAR = (uint32_t)rx_buffer;    // 存储器地址为rx_buffer
    DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;        // 传输数据长度为缓冲区大小
    /* 3. 配置DMA1通道5 CCR寄存器
         - DIR   = 0：外设→存储器
         - CIRC  = 1：循环模式
         - PINC  = 0：外设地址不自增
         - MINC  = 1：存储器地址自增
         - PSIZE = 00：外设数据宽度8位
         - MSIZE = 00：存储器数据宽度8位
         - PL    = 10：优先级设为高
         - MEM2MEM = 0：非存储器到存储器模式
    */
    DMA1_Channel5->CCR = 0;  // 清除之前的配置
    DMA1_Channel5->CCR |= (1UL << 5);       // 使能循环模式 (CIRC，bit5)
    DMA1_Channel5->CCR |= (1UL << 7);       // 使能存储器自增 (MINC，bit7)
    DMA1_Channel5->CCR |= (3UL << 12);      // 设置优先级为非常高 (PL置为“11”，bit12-13)
    
    // 增加传输完成与传输过半中断
    DMA1_Channel5->CCR |= (1UL << 1);             // 传输完成中断 (TCIE)
    DMA1_Channel5->CCR |= (1UL << 2);             // 传输过半中断 (HTIE)
    /* 4. 使能DMA通道5 */
    DMA1_Channel5->CCR |= 1UL;  // 置EN位启动通道
}

void USART1_SendString_DMA(const char *data, uint16_t len)
{
    if (len == 0 || len > TX_BUFFER_SIZE) { // 不能让DMA发送0个字节，会导致没办法进入发送完成中断，然后卡死这个函数。
        return;
    }
    // 等待上一次DMA传输完成（也可以添加超时机制）
    while(tx_dma_busy);
    tx_dma_busy = 1; // 标记DMA正在发送
    memcpy((uint8_t*)tx_buffer, data, len);
    // 如果DMA通道4正在使能，则先禁用以便重新配置
    if(DMA1_Channel4->CCR & 1UL) { // 检查EN位（bit0）是否置位
        DMA1_Channel4->CCR &= ~1UL;  // 禁用DMA通道4（清除EN位）
        while(DMA1_Channel4->CCR & 1UL); // 等待DMA通道4完全关闭
    }
    // 配置DMA通道4：内存地址、外设地址及数据传输长度
    DMA1_Channel4->CMAR  = (uint32_t)data;         // 配置内存地址
    DMA1_Channel4->CPAR  = (uint32_t)&USART1->DR;  // 配置外设地址
    DMA1_Channel4->CNDTR = len;                    // 配置传输数据长度
    // 开启USART1的DMA发送请求：CR3中DMAT（第7位）置1
    USART1->CR3 |= (1UL << 7UL);
    // 启动DMA通道4：设置EN位启动DMA传输
    DMA1_Channel4->CCR |= 1UL;
}

#else

// 配置DMA1通道5用于USART1_RX
__STATIC_INLINE void DMA1_Channel5_Configure(void) {
    /* 配置DMA1通道5用于USART1_RX接收 */
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)rx_buffer);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)&USART1->DR);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
}

/**
  * @brief  使用DMA发送字符串，采用USART1_TX对应的DMA1通道4
  * @param  data: 待发送数据指针（必须指向独立发送缓冲区）
  * @param  len:  待发送数据长度
  * @retval None
  */
void USART1_SendString_DMA(const char *data, uint16_t len)
{
    if (len == 0 || len > TX_BUFFER_SIZE) { // 不能让DMA发送0个字节，会导致没办法进入发送完成中断，然后卡死这个函数。
        return;
    }
    // 等待上一次DMA传输完成
    while(tx_dma_busy);
    tx_dma_busy = 1; // 标记DMA正在发送
    memcpy((uint8_t*)tx_buffer, data, len); // 将需要发送的数据放入DMA发送缓存区
    
    // 如果DMA通道4正在使能，则先禁用以便重新配置
    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4));
    }
    // 配置DMA通道4：内存地址、外设地址及数据传输长度
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)data);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)&USART1->DR);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, len);
    // 开启USART1的DMA发送请求（CR3中DMAT置1，通常为第7位）
    LL_USART_EnableDMAReq_TX(USART1); // 开启USART1的DMA发送请求
    // 启动DMA通道4，开始DMA传输
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
}
#endif

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
#if (USE_LL_LIBRARY == 0)
  Enable_Peripherals_Clock(); // 启动所需外设的时钟
#endif
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
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
#if (USE_LL_LIBRARY == 1)
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
#else
  USART1_Configure(); // USART1
  DMA1_Channel4_Configure(); // DMA1通道4
#endif
  DMA1_Channel5_Configure(); // DMA1通道5
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (rx_complete) {
        USART1_SendString_DMA((const char*)tx_buffer, recvd_length);
        rx_complete = 0; // 清除标志，等待下一次接收
    }
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
