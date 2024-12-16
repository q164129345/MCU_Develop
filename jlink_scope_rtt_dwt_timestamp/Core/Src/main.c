/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
volatile uint8_t JS_RTT_BufferUp1[2048] = {0,};
const uint8_t JS_RTT_Channel = 1;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define SINE_TABLE_SIZE 100
const uint8_t sineTable[SINE_TABLE_SIZE] = {
    128, 136, 144, 152, 160, 167, 175, 182, 189, 196, 202, 208, 214, 219, 224, 229, 233, 237, 240, 243,
    246, 248, 250, 251, 252, 251, 250, 248, 246, 243, 240, 237, 233, 229, 224, 219, 214, 208, 202, 196,
    189, 182, 175, 167, 160, 152, 144, 136, 128, 120, 112, 104,  96,  89,  81,  74, 67,  60,  54,  48,
    42,  37,  32,  27,  23,  19,  16,  13,  10,   8,   6,   5,   4,   3,   3,   3,  4,   5,   6,   8,
    10,  13,  16,  19,  23,  27,  32,  37,  42,  48,  54,  60,  67,  74,  81,  89,  96, 104, 112, 120
}; // 预先计算的正弦波数据

// JScope_t4u4u4的数据结构
typedef struct {
    volatile uint32_t timestamp;
    volatile uint32_t msg1;
    volatile uint32_t msg2;
}RTT_MSG_U1I1;

const uint8_t JscopeChannel = 1; // 通道1（J-Scope不能用通道0）
volatile uint8_t sineIndex = 0;
volatile uint8_t sineValue = 0;
volatile uint8_t numbool = 0;
volatile uint32_t gTimestamp = 0;
RTT_MSG_U1I1 rtt_JsMsg;
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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_TIM14_Init();
  /* USER CODE BEGIN 2 */
  // 配置MCU -> PC缓冲区（上行缓存区）
  SEGGER_RTT_ConfigUpBuffer(JscopeChannel,                  // 通道号
                            // 通道名字（命名有意义的，一定要按照官方文档“RTT channel naming convention”的规范来）
                            "JScope_t4u4u4",                // 数据包含1个32位的时间戳与1个uint32_t变量与1个uint32_t变量
                            (uint8_t*)&JS_RTT_BufferUp1[0], // 缓存地址
                            sizeof(JS_RTT_BufferUp1),       // 缓存大小
                            SEGGER_RTT_MODE_NO_BLOCK_SKIP); // 非阻塞
  DWT_Timer_Init(); // 初始化DWT定时器
  HAL_Delay(10);
  HAL_TIM_Base_Start_IT(&htim14); // 开启定时器中断
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_GPIO_TogglePin(RUN_LED_GPIO_Port,RUN_LED_Pin);
    //SEGGER_RTT_printf(0, "Dwt:%d\n",DWT_Get_Microsecond()); // 通过0，RTT默认初始化了缓存区，所以可以省去配置缓存区
    HAL_Delay(100);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == (&htim14))
    {
        // 输出正弦波数据
        sineValue = sineTable[sineIndex];
        // 更新索引
        sineIndex++;
        if (sineIndex >= SINE_TABLE_SIZE)
        {
            sineIndex = 0; // 重置索引
        }

        // 反转，产生方波
        if (numbool == 0) {
            numbool = 1;
        } else {
            numbool = 0;
        }
        // 将数据放入消息包
        rtt_JsMsg.timestamp = DWT_Get_Microsecond(); // 从dwt定时器获取时间戳
        rtt_JsMsg.msg1 = sineValue;
        rtt_JsMsg.msg2 = numbool;
        SEGGER_RTT_Write(JscopeChannel,&rtt_JsMsg,sizeof(rtt_JsMsg));
    }
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
