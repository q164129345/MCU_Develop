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
#include "bsp_usart_hal.h"
#include "retarget_rtt.h"
#include "app_jump.h"
#include "soft_crc32.h"
#include "op_flash.h"
#include "fw_verify.h"
#include "ymodem.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

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

//! YModem协议处理器实例
YModem_Handler_t gYModemHandler;

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

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  log_printf("Entering the main initialization of the bootloader.\n");
  uint32_t fre = 0;
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
  /* USER CODE BEGIN 2 */

  //! USART1初始化
  USART_Config(&gUsart1Drv,
               gUsart1RXDMABuffer, gUsart1RXRBBuffer, sizeof(gUsart1RXDMABuffer),
               gUsart1TXDMABuffer, gUsart1TXRBBuffer, sizeof(gUsart1TXDMABuffer));
  
  //! YModem协议处理器初始化（完全解耦版本）
  YModem_Init(&gYModemHandler);
  
  log_printf("Bootloader init successfully.\n"); //! bootloader初始化完成
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    //! 30ms
    if (0 == fre % 30) {
        HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin); //! 心跳灯快闪（在bootlaoder程序里，心跳灯快闪。App程序，心跳灯慢闪。肉眼区分当前跑什么程序）
    }
    
    //! 2ms
    if (0 == fre % 2) {
        //! 处理已经接收的数据
        //! YModem协议处理 - 逐字节从ringbuffer里拿出数据来解析
        while(USART_Get_The_Existing_Amount_Of_Data(&gUsart1Drv)) {
            uint8_t data;
            if (USART_Take_A_Piece_Of_Data(&gUsart1Drv, &data)) {
                YModem_Result_t ymodem_result = YModem_Run(&gYModemHandler, data);
                
                //! 检查YModem传输结果
                if (ymodem_result == YMODEM_RESULT_COMPLETE) {
                    log_printf("YModem: IAP upgrade completed. Prepare to verify and copy the firmware.\r\n");
                    //! 这里可以触发固件校验和复制流程
                    
                    //! 重要：立即重置YModem处理器，准备下次传输
                    log_printf("YModem: resetting for next transmission...\r\n");
                    YModem_Reset(&gYModemHandler);
                    
                    //! 清空串口接收缓冲区，防止残留数据干扰
                    //! 通过读取所有数据来清空RingBuffer
                    uint8_t dummy_data;
                    while(USART_Get_The_Existing_Amount_Of_Data(&gUsart1Drv)) {
                        USART_Take_A_Piece_Of_Data(&gUsart1Drv, &dummy_data);
                    }
                    
                } else if (ymodem_result == YMODEM_RESULT_ERROR) {
                    log_printf("YModem: transmission error, reset protocol processor.\r\n"); //! 传输出错，重置协议处理器
                    YModem_Reset(&gYModemHandler);
                    
                    //! 清空串口接收缓冲区，防止残留数据干扰
                    uint8_t dummy_data;
                    while(USART_Get_The_Existing_Amount_Of_Data(&gUsart1Drv)) {
                        USART_Take_A_Piece_Of_Data(&gUsart1Drv, &dummy_data);
                    }
                }
            }
        }
        
        //! 检查是否有YModem响应数据需要发送
        if (YModem_Has_Response(&gYModemHandler)) {
            uint8_t response_buffer[16];
            uint8_t response_length = YModem_Get_Response(&gYModemHandler, response_buffer, sizeof(response_buffer));
            if (response_length > 0) {
                //! 将响应数据发送给上位机
                USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, response_buffer, response_length);
            }
        }
        
        USART_Module_Run(&gUsart1Drv); //! Usart1模块运行
    }
    
    fre++;
    HAL_Delay(1);
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
