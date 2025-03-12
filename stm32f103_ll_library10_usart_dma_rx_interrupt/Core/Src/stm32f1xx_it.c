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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
__IO uint32_t gTickCount = 0;
__IO uint8_t pinStatus = 0;

extern volatile uint8_t rx_buffer[];
extern volatile uint8_t rx_complete;
extern volatile uint16_t recvd_length;

extern volatile uint8_t tx_dma_busy;
extern volatile uint8_t tx_buffer[];
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
  gTickCount++;
  /* USER CODE END SysTick_IRQn 0 */

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

  /* USER CODE BEGIN DMA1_Channel4_IRQn 1 */
#if (USE_LL_LIBRARY == 1)
    // 检查传输完成标志（TC）是否置位（LL库提供TC4接口）
    if(LL_DMA_IsActiveFlag_TC4(DMA1))
    {
        // 清除DMA传输完成标志
        LL_DMA_ClearFlag_TC4(DMA1);
        // 禁用DMA通道4，确保下次传输前重新配置
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        // 清除USART1中DMAT位，关闭DMA发送请求
        LL_USART_DisableDMAReq_TX(USART1);
        // 标记DMA发送完成
        tx_dma_busy = 0;
    }
#else
    // 寄存器方式
    if(DMA1->ISR & (1UL << 13)) {
        // 清除DMA传输完成标志：在IFCR寄存器中写1清除对应标志
        DMA1->IFCR |= (1UL << 13);
        // 禁用DMA通道4（清除CCR寄存器的EN位，位0）
        DMA1_Channel4->CCR &= ~(1UL << 0);
        // 清除USART1中DMAT位，关闭DMA发送请求（CR3寄存器的位7）
        USART1->CR3 &= ~(1UL << 7);
        // 标记DMA发送完成
        tx_dma_busy = 0;
    }
#endif
  /* USER CODE END DMA1_Channel4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel5_IRQn 0 */

  /* USER CODE END DMA1_Channel5_IRQn 0 */

  /* USER CODE BEGIN DMA1_Channel5_IRQn 1 */
    // 判断是否产生半传输中断（前半区完成）
    if(LL_DMA_IsActiveFlag_HT5(DMA1)) {
        // 清除半传输标志
        LL_DMA_ClearFlag_HT5(DMA1);
        // 处理前 512 字节数据（偏移 0~511）
        memcpy((void*)tx_buffer, (const void*)rx_buffer, RX_BUFFER_SIZE/2);
        recvd_length = RX_BUFFER_SIZE/2;
        rx_complete = 1;
    }
  
    // 判断是否产生传输完成中断（后半区完成）
    if(LL_DMA_IsActiveFlag_TC5(DMA1)) {
        // 清除传输完成标志
        LL_DMA_ClearFlag_TC5(DMA1);
        // 处理后 512 字节数据（偏移 512~1023）
        memcpy((void*)tx_buffer, (const void*)(rx_buffer + RX_BUFFER_SIZE/2), RX_BUFFER_SIZE/2);
        recvd_length = RX_BUFFER_SIZE/2;
        rx_complete = 1;
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
  /* USER CODE BEGIN USART1_IRQn 1 */
#if (USE_LL_LIBRARY == 1)
    // LL库方式
    // 检查 USART1 是否因空闲而中断
    if (LL_USART_IsActiveFlag_IDLE(USART1)) {
        uint32_t tmp;
        /* 清除IDLE标志：必须先读SR，再读DR */
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        
        // 禁用 DMA1 通道5，防止数据继续写入
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
        
        uint16_t remaining = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_5); // 获取剩余的容量
        uint16_t count = 0;
        // 根据剩余字节判断当前正在哪个半区
        // 还有，避免当数据长度刚好512字节与1024字节时，传输过半中断与空闲中断复制两遍数据，与传输完成中断与空闲中断复制两遍数据。
        if (remaining > (RX_BUFFER_SIZE/2)) {
            // 还在接收前半区：接收数据量 = (1K - remaining)，但肯定不足 512 字节
            count = RX_BUFFER_SIZE - remaining;
            if (count != 0) { // 避免与传输完成中断冲突，多复制一次
                memcpy((void*)tx_buffer, (const void*)rx_buffer, count);
            }
        } else {
            // 前半区已写满，当前在后半区：后半区接收数据量 = (RX_BUFFER_SIZE/2 - remaining)
            count = (RX_BUFFER_SIZE/2) - remaining;
            if (count != 0) { // 避免与传输过半中断冲突，多复制一次
                memcpy((void*)tx_buffer, (const void*)(rx_buffer + RX_BUFFER_SIZE/2), count);
            }
        }
        
        if (count != 0) {
            recvd_length = count;
            rx_complete = 1;
        }

        // 重新设置 DMA 传输长度并使能 DMA
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
    }
#else
    // 寄存器方式
    // 检查USART1 SR寄存器的IDLE标志（bit4）
    if (USART1->SR & (1UL << 4)) {
        uint32_t tmp;
        // 清除IDLE标志：先读SR再读DR
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        
        // 禁用DMA1通道5：清除CCR寄存器的EN位（bit0）
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // (可选)等待通道确实关闭：while(DMA1_Channel5->CCR & (1UL << 0));
        // 计算本次接收的字节数
        recvd_length = RX_BUFFER_SIZE - DMA1_Channel5->CNDTR;
        // 将接收到的数据复制到发送缓冲区
        memcpy((void*)tx_buffer, (const void*)rx_buffer, recvd_length);
        // 标记接收完成
        rx_complete = 1;
        // 重置DMA接收：设置CNDTR寄存器为RX_BUFFER_SIZE
        DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;
        // 重新使能DMA1通道5：设置EN位（bit0）
        DMA1_Channel5->CCR |= 1UL;
    }
#endif
  /* USER CODE END USART1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
