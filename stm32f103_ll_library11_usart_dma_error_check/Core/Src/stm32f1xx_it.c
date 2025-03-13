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

extern volatile uint16_t usart1Error;
extern volatile uint16_t dma1Channel4Error;
extern volatile uint16_t dma1Channel5Error;
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
#if USE_LL_LIBRARY == 1
/**
  * @brief  检查USART1错误标志，并清除相关错误标志（ORE、NE、FE、PE）。
  * @note   此函数基于LL库实现。当检测到USART1错误标志时，通过读取SR和DR清除错误，
  *         并返回1表示存在错误；否则返回0。
  * @retval uint8_t 错误状态：1表示检测到错误并已清除，0表示无错误。
  */
static __inline uint8_t USART1_Error_Handler(void) {
    // 错误处理：检查USART错误标志（ORE、NE、FE、PE）
    if (LL_USART_IsActiveFlag_ORE(USART1) ||
        LL_USART_IsActiveFlag_NE(USART1)  ||
        LL_USART_IsActiveFlag_FE(USART1)  ||
        LL_USART_IsActiveFlag_PE(USART1))
    {
        // 通过读SR和DR来清除错误标志
        volatile uint32_t tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        return 1;
    } else {
        return 0;
    } 
}
/**
  * @brief  检查DMA1通道4传输错误，并处理错误状态。
  * @note   此函数基于LL库实现。主要用于检测USART1_TX传输过程中DMA1通道4是否发生传输错误（TE）。
  *         如果检测到错误，则清除错误标志、禁用DMA1通道4，并关闭USART1的DMA发送请求，
  *         从而终止当前传输。返回1表示错误已处理；否则返回0。
  * @retval uint8_t 错误状态：1表示检测到并处理了错误，0表示无错误。
  */
static __inline uint8_t DMA1_Channel4_Error_Handler(void) {
    // 检查通道4是否发生传输错误（TE）
    if (LL_DMA_IsActiveFlag_TE4(DMA1)) {
        // 清除传输错误标志
        LL_DMA_ClearFlag_TE4(DMA1);
        // 禁用DMA通道4，停止当前传输
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        // 清除USART1的DMA发送请求（DMAT位）
        LL_USART_DisableDMAReq_TX(USART1);
        // 清除发送标志！！
        tx_dma_busy = 0;
        return 1;
    } else {
        return 0;
    }
}
/**
  * @brief  检查DMA1通道5传输错误，并恢复DMA接收。
  * @note   此函数基于LL库实现。主要用于检测USART1_RX传输过程中DMA1通道5是否发生传输错误（TE）。
  *         如果检测到错误，则清除错误标志、禁用DMA1通道5、重置传输长度为RX_BUFFER_SIZE，
  *         并重新使能DMA通道5以恢复数据接收。返回1表示错误已处理；否则返回0。
  * @retval uint8_t 错误状态：1表示检测到并处理了错误，0表示无错误。
  */
static __inline uint8_t DMA1_Channel5_Error_Hanlder(void) {
    // 检查通道5是否发生传输错误（TE）
    if (LL_DMA_IsActiveFlag_TE5(DMA1)) {
        // 清除传输错误标志
        LL_DMA_ClearFlag_TE5(DMA1);
        // 禁用DMA通道5，停止当前传输
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
        // 重新设置传输长度，恢复到初始状态
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
        // 重新使能DMA通道5，恢复接收
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
        return 1;
    } else {
        return 0;
    }
}

#else
static __inline uint8_t USART1_Error_Handler(void) {
    // 寄存器方式：检查错误标志（PE、FE、NE、ORE分别位0~3）
    if (USART1->SR & ((1UL << 0) | (1UL << 1) | (1UL << 2) | (1UL << 3))) {
        // 清除错误标志
        volatile uint32_t tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        return 1;
    } else {
        return 0;
    }
}
static __inline uint8_t DMA1_Channel4_Error_Handler(void) {
    // 检查传输错误（TE）标志，假设TE对应位(1UL << 15)（请根据具体芯片参考手册确认）
    if (DMA1->ISR & (1UL << 15)) {
        // 清除TE错误标志
        DMA1->IFCR |= (1UL << 15);
        // 禁用DMA通道4
        DMA1_Channel4->CCR &= ~(1UL << 0);
        // 清除USART1中DMAT位
        USART1->CR3 &= ~(1UL << 7);
        // 清除发送标志！！
        tx_dma_busy = 0;
        return 1;
    } else {
        return 0;
    }
}
static __inline uint8_t DMA1_Channel5_Error_Hanlder(void) {
    // 检查传输错误（TE）标志，假设TE对应位(1UL << 19)（请确认具体位）
    if (DMA1->ISR & (1UL << 19)) {
        // 清除错误标志
        DMA1->IFCR |= (1UL << 19);
        // 禁用DMA通道5
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // 重置传输计数
        DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;
        // 重新使能DMA通道5
        DMA1_Channel5->CCR |= 1UL;
        return 1;
    } else {
        return 0;
    }
}

#endif
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
    if (DMA1_Channel4_Error_Handler()) { // 监控传输错误
        dma1Channel4Error++;
    } else if(LL_DMA_IsActiveFlag_TC4(DMA1)) {// 检查传输完成标志（TC）是否置位（LL库提供TC4接口）
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
    if (DMA1_Channel4_Error_Handler()) { // 监控传输错误
        dma1Channel4Error++;
    } else if (DMA1->ISR & (1UL << 13)) {
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
#if USE_LL_LIBRARY == 1
    if (DMA1_Channel5_Error_Hanlder()) { // 监控传输失败
        dma1Channel5Error++;
    } else if(LL_DMA_IsActiveFlag_HT5(DMA1)) { // 判断是否产生半传输中断（前半区完成）
        // 清除半传输标志
        LL_DMA_ClearFlag_HT5(DMA1);
        // 处理前 512 字节数据（偏移 0~511）
        memcpy((void*)tx_buffer, (const void*)rx_buffer, RX_BUFFER_SIZE/2);
        recvd_length = RX_BUFFER_SIZE/2;
        rx_complete = 1;
    } else if(LL_DMA_IsActiveFlag_TC5(DMA1)) { // 判断是否产生传输完成中断（后半区完成）
        // 清除传输完成标志
        LL_DMA_ClearFlag_TC5(DMA1);
        // 处理后 512 字节数据（偏移 512~1023）
        memcpy((void*)tx_buffer, (const void*)(rx_buffer + RX_BUFFER_SIZE/2), RX_BUFFER_SIZE/2);
        recvd_length = RX_BUFFER_SIZE/2;
        rx_complete = 1;
    }
#else
    if (DMA1_Channel5_Error_Hanlder()) { // 监控传输错误
        dma1Channel5Error++;
    } else if (DMA1->ISR & (1UL << 18)) { // 半传输中断
        DMA1->IFCR |= (1UL << 18);
        // 前半缓冲区处理
        memcpy((void*)tx_buffer, (const void*)rx_buffer, RX_BUFFER_SIZE / 2);
        recvd_length = RX_BUFFER_SIZE / 2;
        rx_complete = 1;
    } else if (DMA1->ISR & (1UL << 17)) { // 传输完成中断
        DMA1->IFCR |= (1UL << 17);
        // 后半缓冲区处理
        memcpy((void*)tx_buffer, (const void*)(rx_buffer + RX_BUFFER_SIZE / 2), RX_BUFFER_SIZE / 2);
        recvd_length = RX_BUFFER_SIZE / 2;
        rx_complete = 1;
    }
#endif
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
    if (USART1_Error_Handler()) { // 检查错误标志
        usart1Error++; // 有错误，记录事件
    } else if (LL_USART_IsActiveFlag_IDLE(USART1)) {  // 检查 USART1 是否因空闲而中断
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
    if (USART1_Error_Handler()) { // 监控串口错误
        usart1Error++; // 有错误，记录事件
    } else if (USART1->SR & (1UL << 4)) { // 检查USART1 SR寄存器的IDLE标志（bit4）
        uint32_t tmp;
        // 清除IDLE标志：先读SR再读DR
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        // 禁用DMA1通道5：清除CCR寄存器的EN位（bit0）
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // 获取剩余的数据量
        uint16_t remaining = DMA1_Channel5->CNDTR;
        uint16_t count = 0;

        // 根据剩余字节判断当前正在哪个半区
        // 注意：为避免当数据长度刚好为512字节或1024字节时，与DMA半传输/传输完成中断冲突，
        // 在count不为0时才进行数据复制
        if (remaining > (RX_BUFFER_SIZE / 2)) {
            // 还在接收前半区：接收数据量 = (总长度 - remaining)，不足512字节
            count = RX_BUFFER_SIZE - remaining;
            if (count != 0) {  // 避免与传输完成中断冲突，多复制一次
                memcpy((void*)tx_buffer, (const void*)rx_buffer, count);
            }
        } else {
            // 前半区已满，当前在后半区：后半区接收数据量 = (前半区长度 - remaining)
            count = (RX_BUFFER_SIZE / 2) - remaining;
            if (count != 0) {  // 避免与传输过半中断冲突，多复制一次
                memcpy((void*)tx_buffer, (const void*)rx_buffer + (RX_BUFFER_SIZE / 2), count);
            }
        }
        if (count != 0) {
            recvd_length = count;
            rx_complete = 1;
        }
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
