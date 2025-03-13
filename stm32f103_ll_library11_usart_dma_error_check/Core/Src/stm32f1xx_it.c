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
  * @brief  ���USART1�����־���������ش����־��ORE��NE��FE��PE����
  * @note   �˺�������LL��ʵ�֡�����⵽USART1�����־ʱ��ͨ����ȡSR��DR�������
  *         ������1��ʾ���ڴ��󣻷��򷵻�0��
  * @retval uint8_t ����״̬��1��ʾ��⵽�����������0��ʾ�޴���
  */
static __inline uint8_t USART1_Error_Handler(void) {
    // ���������USART�����־��ORE��NE��FE��PE��
    if (LL_USART_IsActiveFlag_ORE(USART1) ||
        LL_USART_IsActiveFlag_NE(USART1)  ||
        LL_USART_IsActiveFlag_FE(USART1)  ||
        LL_USART_IsActiveFlag_PE(USART1))
    {
        // ͨ����SR��DR����������־
        volatile uint32_t tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        return 1;
    } else {
        return 0;
    } 
}
/**
  * @brief  ���DMA1ͨ��4������󣬲��������״̬��
  * @note   �˺�������LL��ʵ�֡���Ҫ���ڼ��USART1_TX���������DMA1ͨ��4�Ƿ����������TE����
  *         �����⵽��������������־������DMA1ͨ��4�����ر�USART1��DMA��������
  *         �Ӷ���ֹ��ǰ���䡣����1��ʾ�����Ѵ������򷵻�0��
  * @retval uint8_t ����״̬��1��ʾ��⵽�������˴���0��ʾ�޴���
  */
static __inline uint8_t DMA1_Channel4_Error_Handler(void) {
    // ���ͨ��4�Ƿ����������TE��
    if (LL_DMA_IsActiveFlag_TE4(DMA1)) {
        // �����������־
        LL_DMA_ClearFlag_TE4(DMA1);
        // ����DMAͨ��4��ֹͣ��ǰ����
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        // ���USART1��DMA��������DMATλ��
        LL_USART_DisableDMAReq_TX(USART1);
        // ������ͱ�־����
        tx_dma_busy = 0;
        return 1;
    } else {
        return 0;
    }
}
/**
  * @brief  ���DMA1ͨ��5������󣬲��ָ�DMA���ա�
  * @note   �˺�������LL��ʵ�֡���Ҫ���ڼ��USART1_RX���������DMA1ͨ��5�Ƿ����������TE����
  *         �����⵽��������������־������DMA1ͨ��5�����ô��䳤��ΪRX_BUFFER_SIZE��
  *         ������ʹ��DMAͨ��5�Իָ����ݽ��ա�����1��ʾ�����Ѵ������򷵻�0��
  * @retval uint8_t ����״̬��1��ʾ��⵽�������˴���0��ʾ�޴���
  */
static __inline uint8_t DMA1_Channel5_Error_Hanlder(void) {
    // ���ͨ��5�Ƿ����������TE��
    if (LL_DMA_IsActiveFlag_TE5(DMA1)) {
        // �����������־
        LL_DMA_ClearFlag_TE5(DMA1);
        // ����DMAͨ��5��ֹͣ��ǰ����
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
        // �������ô��䳤�ȣ��ָ�����ʼ״̬
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
        // ����ʹ��DMAͨ��5���ָ�����
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
        return 1;
    } else {
        return 0;
    }
}

#else
static __inline uint8_t USART1_Error_Handler(void) {
    // �Ĵ�����ʽ���������־��PE��FE��NE��ORE�ֱ�λ0~3��
    if (USART1->SR & ((1UL << 0) | (1UL << 1) | (1UL << 2) | (1UL << 3))) {
        // ��������־
        volatile uint32_t tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        return 1;
    } else {
        return 0;
    }
}
static __inline uint8_t DMA1_Channel4_Error_Handler(void) {
    // ��鴫�����TE����־������TE��Ӧλ(1UL << 15)������ݾ���оƬ�ο��ֲ�ȷ�ϣ�
    if (DMA1->ISR & (1UL << 15)) {
        // ���TE�����־
        DMA1->IFCR |= (1UL << 15);
        // ����DMAͨ��4
        DMA1_Channel4->CCR &= ~(1UL << 0);
        // ���USART1��DMATλ
        USART1->CR3 &= ~(1UL << 7);
        // ������ͱ�־����
        tx_dma_busy = 0;
        return 1;
    } else {
        return 0;
    }
}
static __inline uint8_t DMA1_Channel5_Error_Hanlder(void) {
    // ��鴫�����TE����־������TE��Ӧλ(1UL << 19)����ȷ�Ͼ���λ��
    if (DMA1->ISR & (1UL << 19)) {
        // ��������־
        DMA1->IFCR |= (1UL << 19);
        // ����DMAͨ��5
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // ���ô������
        DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;
        // ����ʹ��DMAͨ��5
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
    if (DMA1_Channel4_Error_Handler()) { // ��ش������
        dma1Channel4Error++;
    } else if(LL_DMA_IsActiveFlag_TC4(DMA1)) {// ��鴫����ɱ�־��TC���Ƿ���λ��LL���ṩTC4�ӿڣ�
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
    if (DMA1_Channel4_Error_Handler()) { // ��ش������
        dma1Channel4Error++;
    } else if (DMA1->ISR & (1UL << 13)) {
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
#if USE_LL_LIBRARY == 1
    if (DMA1_Channel5_Error_Hanlder()) { // ��ش���ʧ��
        dma1Channel5Error++;
    } else if(LL_DMA_IsActiveFlag_HT5(DMA1)) { // �ж��Ƿ�����봫���жϣ�ǰ������ɣ�
        // ����봫���־
        LL_DMA_ClearFlag_HT5(DMA1);
        // ����ǰ 512 �ֽ����ݣ�ƫ�� 0~511��
        memcpy((void*)tx_buffer, (const void*)rx_buffer, RX_BUFFER_SIZE/2);
        recvd_length = RX_BUFFER_SIZE/2;
        rx_complete = 1;
    } else if(LL_DMA_IsActiveFlag_TC5(DMA1)) { // �ж��Ƿ������������жϣ��������ɣ�
        // ���������ɱ�־
        LL_DMA_ClearFlag_TC5(DMA1);
        // ����� 512 �ֽ����ݣ�ƫ�� 512~1023��
        memcpy((void*)tx_buffer, (const void*)(rx_buffer + RX_BUFFER_SIZE/2), RX_BUFFER_SIZE/2);
        recvd_length = RX_BUFFER_SIZE/2;
        rx_complete = 1;
    }
#else
    if (DMA1_Channel5_Error_Hanlder()) { // ��ش������
        dma1Channel5Error++;
    } else if (DMA1->ISR & (1UL << 18)) { // �봫���ж�
        DMA1->IFCR |= (1UL << 18);
        // ǰ�뻺��������
        memcpy((void*)tx_buffer, (const void*)rx_buffer, RX_BUFFER_SIZE / 2);
        recvd_length = RX_BUFFER_SIZE / 2;
        rx_complete = 1;
    } else if (DMA1->ISR & (1UL << 17)) { // ��������ж�
        DMA1->IFCR |= (1UL << 17);
        // ��뻺��������
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
    // LL�ⷽʽ
    if (USART1_Error_Handler()) { // �������־
        usart1Error++; // �д��󣬼�¼�¼�
    } else if (LL_USART_IsActiveFlag_IDLE(USART1)) {  // ��� USART1 �Ƿ�����ж��ж�
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
    if (USART1_Error_Handler()) { // ��ش��ڴ���
        usart1Error++; // �д��󣬼�¼�¼�
    } else if (USART1->SR & (1UL << 4)) { // ���USART1 SR�Ĵ�����IDLE��־��bit4��
        uint32_t tmp;
        // ���IDLE��־���ȶ�SR�ٶ�DR
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        // ����DMA1ͨ��5�����CCR�Ĵ�����ENλ��bit0��
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // ��ȡʣ���������
        uint16_t remaining = DMA1_Channel5->CNDTR;
        uint16_t count = 0;

        // ����ʣ���ֽ��жϵ�ǰ�����ĸ�����
        // ע�⣺Ϊ���⵱���ݳ��ȸպ�Ϊ512�ֽڻ�1024�ֽ�ʱ����DMA�봫��/��������жϳ�ͻ��
        // ��count��Ϊ0ʱ�Ž������ݸ���
        if (remaining > (RX_BUFFER_SIZE / 2)) {
            // ���ڽ���ǰ���������������� = (�ܳ��� - remaining)������512�ֽ�
            count = RX_BUFFER_SIZE - remaining;
            if (count != 0) {  // �����봫������жϳ�ͻ���ิ��һ��
                memcpy((void*)tx_buffer, (const void*)rx_buffer, count);
            }
        } else {
            // ǰ������������ǰ�ں��������������������� = (ǰ�������� - remaining)
            count = (RX_BUFFER_SIZE / 2) - remaining;
            if (count != 0) {  // �����봫������жϳ�ͻ���ิ��һ��
                memcpy((void*)tx_buffer, (const void*)rx_buffer + (RX_BUFFER_SIZE / 2), count);
            }
        }
        if (count != 0) {
            recvd_length = count;
            rx_complete = 1;
        }
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
