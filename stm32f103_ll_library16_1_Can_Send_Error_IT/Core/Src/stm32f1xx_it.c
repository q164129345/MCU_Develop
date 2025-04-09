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
#include "myCanDrive_reg.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
extern volatile uint8_t txmail_free;
extern volatile uint8_t g_ErrorEventCanBusoff;
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

  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */
void USB_HP_CAN1_TX_IRQHandler(void)
{
    // ��鷢��������жϱ�־��һ��Ҫ�����־λ��
    if(CAN1->TSR & CAN_TSR_TME0) // ����0��
    {
        // ��Ĵ������...
        CAN1->TSR |= CAN_TSR_TME0; // �����־��ͨ��д1�����
    }
    if(CAN1->TSR & CAN_TSR_TME1) // ����1��
    {
        // ��Ĵ������...
        CAN1->TSR |= CAN_TSR_TME1;
    }
    if(CAN1->TSR & CAN_TSR_TME2) // ����2��
    {
        // ��Ĵ������...
        CAN1->TSR |= CAN_TSR_TME2;
    }
    
    // �����ٽ������������׳��
    __disable_irq();
    // ͨ���Ĵ�����ʽ�����·�������Ŀ�������
    txmail_free = ((CAN1->TSR & CAN_TSR_TME0) ? 1 : 0) +
                 ((CAN1->TSR & CAN_TSR_TME1) ? 1 : 0) +
                 ((CAN1->TSR & CAN_TSR_TME2) ? 1 : 0);
    __enable_irq();
}

/**
  * @brief  CAN����״̬�ı��жϴ�����
  *         ��������Bus-Off������ʱ���жϷ����������ȫ�ֱ�־
  * @note   �������ж���ֻ����־���ã������ӻָ�����������ѭ������
  */
void CAN1_SCE_IRQHandler(void)
{
    uint32_t esr = CAN1->ESR;

    // ������߹رմ���Bus-Off��
    if (esr & CAN_ESR_BOFF) {
        g_ErrorEventCanBusoff = 1; // ���ô����־����ѭ�����ݴ˴���ָ�
        CAN1->ESR &= ~CAN_ESR_BOFF; // д0��������־
    }
    CAN1->IER &= ~CAN_IER_ERRIE; // �ر�ȫ�ִ����жϣ�����һֱ���ж���ѭ����������ѭ����ҵ���ܱ�ִ��
    // ������������󱻶������󾯸棩Ҳ�����������¼��ͳ��
    CAN1->ESR &= ~CAN_ESR_LEC;  // ���LEC�������ֶΣ�д0�����
    NVIC_ClearPendingIRQ(CAN1_SCE_IRQn);
}

/* USER CODE END 1 */
