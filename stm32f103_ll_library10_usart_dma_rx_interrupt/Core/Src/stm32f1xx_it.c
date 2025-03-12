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
    // ��鴫����ɱ�־��TC���Ƿ���λ��LL���ṩTC4�ӿڣ�
    if(LL_DMA_IsActiveFlag_TC4(DMA1))
    {
        // ���DMA������ɱ�־
        LL_DMA_ClearFlag_TC4(DMA1);
        // ����DMAͨ��4��ȷ���´δ���ǰ��������
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        // ���USART1��DMATλ���ر�DMA��������
        LL_USART_DisableDMAReq_TX(USART1);
        // ���DMA�������
        tx_dma_busy = 0;
    }
#else
    // �Ĵ�����ʽ
    if(DMA1->ISR & (1UL << 13)) {
        // ���DMA������ɱ�־����IFCR�Ĵ�����д1�����Ӧ��־
        DMA1->IFCR |= (1UL << 13);
        // ����DMAͨ��4�����CCR�Ĵ�����ENλ��λ0��
        DMA1_Channel4->CCR &= ~(1UL << 0);
        // ���USART1��DMATλ���ر�DMA��������CR3�Ĵ�����λ7��
        USART1->CR3 &= ~(1UL << 7);
        // ���DMA�������
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
    // �ж��Ƿ�����봫���жϣ�ǰ������ɣ�
    if(LL_DMA_IsActiveFlag_HT5(DMA1)) {
        // ����봫���־
        LL_DMA_ClearFlag_HT5(DMA1);
        // ����ǰ 512 �ֽ����ݣ�ƫ�� 0~511��
        memcpy((void*)tx_buffer, (const void*)rx_buffer, RX_BUFFER_SIZE/2);
        recvd_length = RX_BUFFER_SIZE/2;
        rx_complete = 1;
    }
  
    // �ж��Ƿ������������жϣ��������ɣ�
    if(LL_DMA_IsActiveFlag_TC5(DMA1)) {
        // ���������ɱ�־
        LL_DMA_ClearFlag_TC5(DMA1);
        // ����� 512 �ֽ����ݣ�ƫ�� 512~1023��
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
    // LL�ⷽʽ
    // ��� USART1 �Ƿ�����ж��ж�
    if (LL_USART_IsActiveFlag_IDLE(USART1)) {
        uint32_t tmp;
        /* ���IDLE��־�������ȶ�SR���ٶ�DR */
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        
        // ���� DMA1 ͨ��5����ֹ���ݼ���д��
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
        
        uint16_t remaining = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_5); // ��ȡʣ�������
        uint16_t count = 0;
        // ����ʣ���ֽ��жϵ�ǰ�����ĸ�����
        // ���У����⵱���ݳ��ȸպ�512�ֽ���1024�ֽ�ʱ����������ж�������жϸ����������ݣ��봫������ж�������жϸ����������ݡ�
        if (remaining > (RX_BUFFER_SIZE/2)) {
            // ���ڽ���ǰ���������������� = (1K - remaining)�����϶����� 512 �ֽ�
            count = RX_BUFFER_SIZE - remaining;
            if (count != 0) { // �����봫������жϳ�ͻ���ิ��һ��
                memcpy((void*)tx_buffer, (const void*)rx_buffer, count);
            }
        } else {
            // ǰ������д������ǰ�ں��������������������� = (RX_BUFFER_SIZE/2 - remaining)
            count = (RX_BUFFER_SIZE/2) - remaining;
            if (count != 0) { // �����봫������жϳ�ͻ���ิ��һ��
                memcpy((void*)tx_buffer, (const void*)(rx_buffer + RX_BUFFER_SIZE/2), count);
            }
        }
        
        if (count != 0) {
            recvd_length = count;
            rx_complete = 1;
        }

        // �������� DMA ���䳤�Ȳ�ʹ�� DMA
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
    }
#else
    // �Ĵ�����ʽ
    // ���USART1 SR�Ĵ�����IDLE��־��bit4��
    if (USART1->SR & (1UL << 4)) {
        uint32_t tmp;
        // ���IDLE��־���ȶ�SR�ٶ�DR
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        
        // ����DMA1ͨ��5�����CCR�Ĵ�����ENλ��bit0��
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // (��ѡ)�ȴ�ͨ��ȷʵ�رգ�while(DMA1_Channel5->CCR & (1UL << 0));
        // ���㱾�ν��յ��ֽ���
        recvd_length = RX_BUFFER_SIZE - DMA1_Channel5->CNDTR;
        // �����յ������ݸ��Ƶ����ͻ�����
        memcpy((void*)tx_buffer, (const void*)rx_buffer, recvd_length);
        // ��ǽ������
        rx_complete = 1;
        // ����DMA���գ�����CNDTR�Ĵ���ΪRX_BUFFER_SIZE
        DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;
        // ����ʹ��DMA1ͨ��5������ENλ��bit0��
        DMA1_Channel5->CCR |= 1UL;
    }
#endif
  /* USER CODE END USART1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
