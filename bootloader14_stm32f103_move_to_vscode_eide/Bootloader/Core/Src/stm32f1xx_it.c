/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32f1xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_usart_hal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
extern USART_Driver_t gUsart1Drv;
/* USER CODE END TD */

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
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern UART_HandleTypeDef huart1;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 channel4 global interrupt.
  */
void DMA1_Channel4_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel4_IRQn 0 */

  /* USER CODE END DMA1_Channel4_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_tx);
  /* USER CODE BEGIN DMA1_Channel4_IRQn 1 */
  //! 检查并统计DMA发送错误
  if (__HAL_DMA_GET_FLAG(&hdma_usart1_tx, DMA_FLAG_TE4)) {
      __HAL_DMA_CLEAR_FLAG(&hdma_usart1_tx, DMA_FLAG_TE4);
      USART_DMA_Error_Recover(&gUsart1Drv, 1); //! DMA错误处理，TX方向
  }
  //! 发送完成中断处理在另一个函数HAL_UART_TxCpltCallback()里处理，官方推荐做法
  /* USER CODE END DMA1_Channel4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel5_IRQn 0 */

  /* USER CODE END DMA1_Channel5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
  /* USER CODE BEGIN DMA1_Channel5_IRQn 1 */
  /* 检查并统计DMA接收错误 */
  if (__HAL_DMA_GET_FLAG(&hdma_usart1_rx, DMA_FLAG_TE5)) {
      __HAL_DMA_CLEAR_FLAG(&hdma_usart1_rx, DMA_FLAG_TE5);
      USART_DMA_Error_Recover(&gUsart1Drv, 0); //! DMA错误处理，RX方向
  }
  /* USER CODE END DMA1_Channel5_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */
  USART_RX_IDLE_Interrupt_Handler(&gUsart1Drv);
  /* USER CODE END USART1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/**
 * @brief UART接收半完成回调（HAL库自动调用）
 * 
 * @param huart UART句柄指针
 */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {  //! 判断是否是USART1
        USART_DMA_RX_Interrupt_Handler(&gUsart1Drv);
    }
}

/**
 * @brief UART接收完成回调（HAL库自动调用）
 * 
 * @param huart UART句柄指针
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {  //! 判断是否是USART1
        USART_DMA_RX_Interrupt_Handler(&gUsart1Drv);
    }
}

/**
  * @brief  串口DMA发送完成回调函数（由HAL库自动调用）
  * @param  huart  指向发生发送完成事件的UART句柄
  * @note   本函数在串口DMA发送完成（TXE/TC）时被HAL库自动回调。
  *         建议在此回调中调用用户自定义的DMA发送完成处理函数，
  *         用于清除DMA忙标志、启动下一次DMA传输等操作。
  *         如有多路串口可根据句柄分别处理。
  * @retval 无
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        USART_DMA_TX_Interrupt_Handler(&gUsart1Drv);  //! DMA发送中断处理
    }
}

/**
  * @brief  串口接收/发送错误回调（HAL库自动调用）
  * @param  huart UART句柄指针
  * @note   本函数被HAL库在发生PE/FE/NE/ORE等错误时自动调用
  *         建议按实例指针判断归属
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        gUsart1Drv.errorRX++;
        //! 你也可以在这里插入系统日志、报警等功能、或者当错误累计到一定程度，重启USART
//        // 1. 先关闭串口（去初始化）
//        HAL_UART_DeInit(huart);
//        // 2. 重新初始化串口（CubeMX生成的初始化函数）
//        MX_USART1_UART_Init();
//        // 3. 重新启动DMA接收，避免丢失后续数据
//        HAL_UART_Receive_DMA(huart, gUsart1Drv.rxDMABuffer, gUsart1Drv.rxBufSize);
    }
    //! 若有多路串口，可继续 else if (huart == xx)
}

/* USER CODE END 1 */
