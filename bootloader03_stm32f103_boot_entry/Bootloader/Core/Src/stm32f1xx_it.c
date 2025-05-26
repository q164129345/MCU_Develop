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
  //! ��鲢ͳ��DMA���ʹ��� 
  if (__HAL_DMA_GET_FLAG(&hdma_usart1_tx, DMA_FLAG_TE4)) {
      __HAL_DMA_CLEAR_FLAG(&hdma_usart1_tx, DMA_FLAG_TE4);
      USART_DMA_Error_Recover(&gUsart1Drv, 1); //! DMA������TX����
  }
  //! ��������жϴ�������һ������HAL_UART_TxCpltCallback()�ﴦ���ٷ��Ƽ�����
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
  /* ��鲢ͳ��DMA���մ��� */
  if (__HAL_DMA_GET_FLAG(&hdma_usart1_rx, DMA_FLAG_TE5)) {
      __HAL_DMA_CLEAR_FLAG(&hdma_usart1_rx, DMA_FLAG_TE5);
      USART_DMA_Error_Recover(&gUsart1Drv, 0); //! DMA������RX����
  } else {
      USART_DMA_RX_Interrupt_Handler(&gUsart1Drv); //! DMA�����жϴ���
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
  * @brief  ����DMA������ɻص���������HAL���Զ����ã�
  * @param  huart  ָ������������¼���UART���
  * @note   �������ڴ���DMA������ɣ�TXE/TC��ʱ��HAL���Զ��ص���
  *         �����ڴ˻ص��е����û��Զ����DMA������ɴ�������
  *         �������DMAæ��־��������һ��DMA����Ȳ�����
  *         ���ж�·���ڿɸ��ݾ���ֱ���
  * @retval ��
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == gUsart1Drv.huart) {
        USART_DMA_TX_Interrupt_Handler(&gUsart1Drv); //! DMA�����жϴ���
    }
}

/**
  * @brief  ���ڽ���/���ʹ���ص���HAL���Զ����ã�
  * @param  huart UART���ָ��
  * @note   ��������HAL���ڷ���PE/FE/NE/ORE�ȴ���ʱ�Զ�����
  *         ���鰴ʵ��ָ���жϹ���
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == gUsart1Drv.huart) {
        gUsart1Drv.errorRX++;
        //! ��Ҳ�������������ϵͳ��־�������ȹ��ܡ����ߵ������ۼƵ�һ���̶ȣ�����USART
//        // 1. �ȹرմ��ڣ�ȥ��ʼ����
//        HAL_UART_DeInit(huart);
//        // 2. ���³�ʼ�����ڣ�CubeMX���ɵĳ�ʼ��������
//        MX_USART1_UART_Init();
//        // 3. ��������DMA���գ����ⶪʧ��������
//        HAL_UART_Receive_DMA(huart, gUsart1Drv.rxDMABuffer, gUsart1Drv.rxBufSize);
    }
    //! ���ж�·���ڣ��ɼ��� else if (huart == xx)
}


/* USER CODE END 1 */
