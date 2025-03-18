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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/**
  * @brief  ʹ��ֱ�ӼĴ���������CAN��ʼ��������HAL��BTR=0x002D0003��
  * @note   BTR����:
  *         - SILM=0, LBKM=0  => ��ʹ�þ�Ĭ/�ػ�
  *         - SJW=0          => SJW=1Tq
  *         - TS2=0x02       => BS2=2 => 3Tq
  *         - TS1=0x0D       => BS1=13 => 14Tq
  *         - BRP=0x03       => ��Ƶ=4
  *         ͬʱ�ر��Զ��ش�(NART=1), �����Զ�����Disable
  */
void CAN_Config(void)
{
    /* 1. ʹ��CAN1��GPIOAʱ�� */
    RCC->APB1ENR |= (1UL << 25);
    RCC->APB2ENR |= (1UL << 2);
    /* 2. ����PA11(CAN_RX)Ϊ�������롢PA12(CAN_TX)Ϊ����������� */
    // PA11: CRH[15:12], MODE=00, CNF=10(��������)
    GPIOA->CRH &= ~(0xF << 12);
    GPIOA->CRH |=  (0x8 << 12);
    GPIOA->ODR |=  (1UL << 11); // ����
    // PA12: CRH[19:16], MODE=11(50MHz), CNF=10(��������)
    GPIOA->CRH &= ~(0xF << 16);
    GPIOA->CRH |=  (0xB << 16);
    /* 3. �����ʼ��ģʽ */
    CAN1->MCR |= (1UL << 0);          // �������INIT
    while(!(CAN1->MSR & (1UL << 0))); // �ȴ�INAK=1
    /* �ر�TTCM/ABOM/AWUM/RFLM/TXFP(��ΪDisable) */
    //CAN1->MCR &= ~(CAN_MCR_TTCM | CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_RFLM | CAN_MCR_TXFP);
    CAN1->MCR &= ~(1UL << 7);  // ���TTCMλ��ʱ�䴥��ģʽ��
    CAN1->MCR &= ~(1UL << 6);  // ���ABOMλ���Զ����߹���ģʽ��
    CAN1->MCR &= ~(1UL << 5);  // ���AWUMλ������Զ����ѣ�
    CAN1->MCR &= ~(1UL << 3);  // ���RFLMλ������FIFO�����±��ĸ��Ǿɱ��ģ�
    CAN1->MCR &= ~(1UL << 2);  // ���TXFPλ������FIFO���ȼ��ɱ�ʶ����������
    /* �ر��Զ��ش�(NART=1) */
    CAN1->MCR |= (1UL << 4);
    /*
    4. ����BTR=0x002D0003 ��HAL����ͬ:
       - SJW=0  => 1Tq
       - TS2=0x02 => 2 => 3Tq
       - TS1=0x0D => 13 => 14Tq
       - BRP=0x03 => 3 => ��Ƶ=4
    */
    CAN1->BTR = (0x00 << 24) |  // SILM(31) | LBKM(30) = 0
            (0x00 << 22) |  // SJW(23:22) = 0 (SJW = 1Tq)
            (0x02 << 20) |  // TS2(22:20) = 2 (TS2 = 3Tq)
            (0x0D << 16) |  // TS1(19:16) = 13 (TS1 = 14Tq)
            (0x0003);       // BRP(9:0) = 3 (Prescaler = 4)
    /* 5. �˳���ʼ��ģʽ */
    CAN1->MCR &= ~CAN_MCR_INRQ;
    while(CAN1->MSR & CAN_MSR_INAK); // �ȴ�INAK=0
    /* ȷ������SLEEPģʽ */
    CAN1->MCR &= ~CAN_MCR_SLEEP;
    while(CAN1->MSR & CAN_MSR_SLAK);
    /* 6. ���ù�����0Ϊ�����ˣ�FIFO0 */
    CAN1->FMR |= CAN_FMR_FINIT; // �����������ʼ��
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FFA1R &= ~(1 << 0);  // ���䵽FIFO0
    CAN1->FA1R  |=  (1 << 0);  // ���������0
    CAN1->FMR   &= ~CAN_FMR_FINIT; // �˳���������ʼ��
}

/**
  * @brief  ֱ�Ӳ����Ĵ���ʵ��CAN��Ϣ����(��׼����֡)
  * @param  stdId: ��׼ID(11λ)
  * @param  data: ����ָ��
  * @param  DLC: ���ݳ���(0~8)
  * @retval 0=���ͳɹ�; 1=�޿�������; 2=��ʱ/ʧ��
  */
uint8_t CAN_SendMessage(uint32_t stdId, uint8_t *data, uint8_t DLC)
{
    uint8_t mailbox;
    uint32_t timeout = 0xFFFF;

    /* Ѱ�ҿ������� */
    for(mailbox = 0; mailbox < 3; mailbox++) {
      if((CAN1->sTxMailBox[mailbox].TIR & CAN_TI0R_TXRQ) == 0)
         break;
    }
    if(mailbox >= 3)
    return 1; // �޿�������

    /* ��ո����� */
    CAN1->sTxMailBox[mailbox].TIR  = 0;
    CAN1->sTxMailBox[mailbox].TDTR = 0;
    CAN1->sTxMailBox[mailbox].TDLR = 0;
    CAN1->sTxMailBox[mailbox].TDHR = 0;

    /* ��׼IDд��TIR��[31:21] */
    CAN1->sTxMailBox[mailbox].TIR |= (stdId << 21);
    /* ��׼֡, RTR=0, IDE=0, ����DLC */
    CAN1->sTxMailBox[mailbox].TDTR = (DLC & 0x0F);

    /* ������� */
    if(DLC <= 4) {
    for(uint8_t i=0; i<DLC; i++) {
      CAN1->sTxMailBox[mailbox].TDLR |= ((uint32_t)data[i]) << (8 * i);
    }
    } else {
    for(uint8_t i=0; i<4; i++) {
      CAN1->sTxMailBox[mailbox].TDLR |= ((uint32_t)data[i]) << (8 * i);
    }
    for(uint8_t i=4; i<DLC; i++) {
      CAN1->sTxMailBox[mailbox].TDHR |= ((uint32_t)data[i]) << (8 * (i-4));
    }
    }

    /* ���������� */
    CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;

    /* ��ѯ�ȴ�TXRQ�����ʱ */
    while((CAN1->sTxMailBox[mailbox].TIR & CAN_TI0R_TXRQ) && --timeout);
    if(timeout == 0) {
    // ����ʧ��(��ACK��λ����), ����2
    return 2;
    }
    return 0; // ���ͳɹ�
}
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

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));

  /** NOJTAG: JTAG-DP Disabled and SW-DP Enabled
  */
  LL_GPIO_AF_Remap_SWJ_NOJTAG();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  CAN_Config();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    LL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
    uint8_t txData[8] = {0x11, 0x22, 0x33, 0x44,
                         0x55, 0x66, 0x77, 0x88};
    CAN_SendMessage(0x123, txData, 8);
//    Test_CAN_Send_Msg();
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
  LL_Init1msTick(72000000);
  LL_SetSystemCoreClock(72000000);
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
