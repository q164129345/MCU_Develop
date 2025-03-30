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
#include "can.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
volatile uint8_t txmail_free = 0;
volatile uint32_t canSendError = 0;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/**
  * @brief  通过CAN总线发送固定序列数据（带序列号）。
  * @note   此函数配置标准CAN数据帧（ID: 0x123），发送8字节固定数据(0x01-0x08)，
  *         如果发送邮箱已满，将延迟1ms等待，发送失败时触发错误处理。
  *         适用于需要简单可靠发送固定格式数据的场景。
  * @retval None
  */
void CAN_Send_Msg_Serial(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    /* 配置CAN发送帧参数 */
    TxHeader.StdId = 0x123;          // 标准标识符，可根据实际需求修改
    TxHeader.ExtId = 0;              // 对于标准帧，扩展标识符无效
    TxHeader.IDE = CAN_ID_STD;       // 标准帧
    TxHeader.RTR = CAN_RTR_DATA;     // 数据帧
    TxHeader.DLC = 8;                // 数据长度为8字节
    TxHeader.TransmitGlobalTime = DISABLE; // 不发送全局时间
    /* 如果没有发送邮箱，延迟等待（如果有实时操作系统，发起调度是更加好的方案） */
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0) {
        LL_mDelay(1);
    }
    /* 发送CAN消息 */
    if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
    /* 发送失败，进入错误处理 */
        Error_Handler();
    }
}
/**
  * @brief  通过CAN总线发送固定序列数据（无序列号，带邮箱状态检查）。
  * @note   此函数在发送邮箱可用时（txmail_free > 0）配置标准CAN数据帧（ID: 0x200），
  *         发送8字节固定数据(0x01-0x08)，成功后减少可用邮箱计数，
  *         发送失败时触发错误处理，邮箱不可用时返回发送失败状态。
  * @retval uint8_t 发送状态：0表示成功，1表示邮箱不可用，发送失败
  */
uint8_t CAN_Send_Msg_No_Serial(void)
{
    if (txmail_free > 0) {
        CAN_TxHeaderTypeDef TxHeader;
        uint32_t TxMailbox;
        uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        /* 配置CAN发送帧参数 */
        TxHeader.StdId = 0x200;          // 标准标识符，可根据实际需求修改
        TxHeader.ExtId = 0;              // 对于标准帧，扩展标识符无效
        TxHeader.IDE = CAN_ID_STD;       // 标准帧
        TxHeader.RTR = CAN_RTR_DATA;     // 数据帧
        TxHeader.DLC = 8;                // 数据长度为8字节
        TxHeader.TransmitGlobalTime = DISABLE; // 不发送全局时间
        /* 发送CAN消息 */
        if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
        /* 发送失败，进入错误处理 */
            Error_Handler();
        }
        txmail_free--; // 用掉一个发送邮箱
        return 0;
    } else {
        return 1;
    }
}


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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */
  txmail_free = HAL_CAN_GetTxMailboxesFreeLevel(&hcan); // 获取发送邮箱的空闲数量，一般都是3个
  HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY); // 启动发送完成中断
  if (HAL_CAN_Start(&hcan) != HAL_OK) {
      Error_Handler(); // 启动失败，进入错误处理
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    LL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
    if (CAN_Send_Msg_No_Serial()) {
        canSendError++; // 因发送邮箱满了，所以发送失败
    }
    LL_mDelay(100);
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
  LL_SetSystemCoreClock(72000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
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
