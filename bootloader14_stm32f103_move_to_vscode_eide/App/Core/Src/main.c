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
#include "MultiTimer.h"
#include "retarget_printf.h"
#include "bsp_usart_hal.h"
#include "retarget_printf.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
MultiTimer gTimer1;
MultiTimer gTimer2;
void Timer1_Callback(MultiTimer *timer, void *userData);
void Timer2_Callback(MultiTimer *timer, void *userData);
static uint64_t SysTick_GetTicks(void)
{
    return HAL_GetTick();
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

//! USART1缓存 RX方向
uint8_t gUsart1RXDMABuffer[2048];
uint8_t gUsart1RXRBBuffer[2048];
//! USART1缓存 TX方向
uint8_t gUsart1TXDMABuffer[2048];
uint8_t gUsart1TXRBBuffer[2048];
//! 实例化Usart1
USART_Driver_t gUsart1Drv = {
    .huart = &huart1,
    .hdma_rx = &hdma_usart1_rx,
    .hdma_tx = &hdma_usart1_tx,
};
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

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  __enable_irq(); //! 使能全局中断
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  
  MultiTimerInstall(SysTick_GetTicks); //! 给MultiTimer提供1ms的时间戳
  MultiTimerStart(&gTimer1, 5, Timer1_Callback, NULL);
  MultiTimerStart(&gTimer2, 500, Timer2_Callback, NULL);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    MultiTimerYield(); //! MultiTimer模块运行
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief  Timer1定时回调函数
  * @note   主要用于周期性运行USART1模块处理函数，并重新启动定时器，实现周期性调度
  * @param  timer    定时器指针，由MultiTimer库自动传递
  * @param  userData 用户数据指针（本函数未使用，可为NULL）
  * @retval None
  */
void Timer1_Callback(MultiTimer *timer, void *userData)
{
    //! USART1模块运行
    USART_Module_Run(&gUsart1Drv); //! Usart1模块运行
    
    //! 重新启动定时器（5ms)
    MultiTimerStart(timer, 5, Timer1_Callback, NULL);
}

/**
  * @brief  Timer2定时回调函数
  * @note   主要用于测试串口发送与LED心跳灯切换，并重新启动定时器
  * @param  timer    定时器指针，由MultiTimer库自动传递
  * @param  userData 用户数据指针（本函数未使用，可为NULL）
  * @retval None
  */
void Timer2_Callback(MultiTimer *timer, void *userData)
{    
    //! 心跳LED
    HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
    
    //! 打印RTT log
    printf("App Running.0123456789_App_Version:2025.09.03\r\n");
    
    //! 重新启动定时器(500ms)
    MultiTimerStart(timer, 500, Timer2_Callback, NULL);
}
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
