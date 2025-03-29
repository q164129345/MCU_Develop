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
  * @brief  ͨ��CAN���߷��͹̶��������ݣ������кţ���
  * @note   �˺������ñ�׼CAN����֡��ID: 0x123��������8�ֽڹ̶�����(0x01-0x08)��
  *         ��������������������ӳ�1ms�ȴ�������ʧ��ʱ����������
  *         ��������Ҫ�򵥿ɿ����͹̶���ʽ���ݵĳ�����
  * @retval None
  */
void CAN_Send_Msg_Serial(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    /* ����CAN����֡���� */
    TxHeader.StdId = 0x123;          // ��׼��ʶ�����ɸ���ʵ�������޸�
    TxHeader.ExtId = 0;              // ���ڱ�׼֡����չ��ʶ����Ч
    TxHeader.IDE = CAN_ID_STD;       // ��׼֡
    TxHeader.RTR = CAN_RTR_DATA;     // ����֡
    TxHeader.DLC = 8;                // ���ݳ���Ϊ8�ֽ�
    TxHeader.TransmitGlobalTime = DISABLE; // ������ȫ��ʱ��
    /* ���û�з������䣬�ӳٵȴ��������ʵʱ����ϵͳ����������Ǹ��Ӻõķ����� */
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0) {
        LL_mDelay(1);
    }
    /* ����CAN��Ϣ */
    if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
    /* ����ʧ�ܣ���������� */
        Error_Handler();
    }
}
/**
  * @brief  ͨ��CAN���߷��͹̶��������ݣ������кţ�������״̬��飩��
  * @note   �˺����ڷ����������ʱ��txmail_free > 0�����ñ�׼CAN����֡��ID: 0x200����
  *         ����8�ֽڹ̶�����(0x01-0x08)���ɹ�����ٿ������������
  *         ����ʧ��ʱ�������������䲻����ʱ���ط���ʧ��״̬��
  * @retval uint8_t ����״̬��0��ʾ�ɹ���1��ʾ���䲻���ã�����ʧ��
  */
uint8_t CAN_Send_Msg_No_Serial(void)
{
    if (txmail_free > 0) {
        CAN_TxHeaderTypeDef TxHeader;
        uint32_t TxMailbox;
        uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        /* ����CAN����֡���� */
        TxHeader.StdId = 0x200;          // ��׼��ʶ�����ɸ���ʵ�������޸�
        TxHeader.ExtId = 0;              // ���ڱ�׼֡����չ��ʶ����Ч
        TxHeader.IDE = CAN_ID_STD;       // ��׼֡
        TxHeader.RTR = CAN_RTR_DATA;     // ����֡
        TxHeader.DLC = 8;                // ���ݳ���Ϊ8�ֽ�
        TxHeader.TransmitGlobalTime = DISABLE; // ������ȫ��ʱ��
        /* ����CAN��Ϣ */
        if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
        /* ����ʧ�ܣ���������� */
            Error_Handler();
        }
        txmail_free--; // �õ�һ����������
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
  txmail_free = HAL_CAN_GetTxMailboxesFreeLevel(&hcan); // ��ȡ��������Ŀ���������һ�㶼��3��
  HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY); // ������������ж�
  if (HAL_CAN_Start(&hcan) != HAL_OK) {
      Error_Handler(); // ����ʧ�ܣ����������
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
        canSendError++; // �����������ˣ����Է���ʧ��
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
