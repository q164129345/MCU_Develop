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
#include "myUsartDrive/myUsartDrive_reg.h"
#include "bsp_systick/bsp_systick.h"
#include "MultiTimer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

MultiTimer gTimer1;
MultiTimer gTimer2;
void Timer1_Callback(MultiTimer *timer, void *userData);
void Timer2_Callback(MultiTimer *timer, void *userData);
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
  
  SCB->VTOR = FLASH_APP_START_ADDR; //! �����ж�������
  __enable_irq(); //! ����ȫ���ж�
  
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  SysTick_Init();      //! ��ʼ��SysTick��ʹ��1ms�ж�
  //Retarget_RTT_Init(); //! ��ʼ��RTT��ͨ��0���ض���printf
  USART1_Configure();  //! ��ʼ��USART1
 
  MultiTimerInstall(SysTick_GetTicks); //! ��MultiTimer�ṩ1ms��ʱ���
  MultiTimerStart(&gTimer1, 5, Timer1_Callback, NULL);
  MultiTimerStart(&gTimer2, 500, Timer2_Callback, NULL);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
    MultiTimerYield(); //! MultiTimerģ������
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
/**
  * @brief  Timer1��ʱ�ص�����
  * @note   ��Ҫ��������������USART1ģ�鴦������������������ʱ����ʵ�������Ե���
  * @param  timer    ��ʱ��ָ�룬��MultiTimer���Զ�����
  * @param  userData �û�����ָ�루������δʹ�ã���ΪNULL��
  * @retval None
  */
void Timer1_Callback(MultiTimer *timer, void *userData)
{
    //! USART1ģ������
    USART1_Module_Run();
    
    //! ����������ʱ����5ms)
    MultiTimerStart(timer, 5, Timer1_Callback, NULL);
}

/**
  * @brief  Timer2��ʱ�ص�����
  * @note   ��Ҫ���ڲ��Դ��ڷ�����LED�������л���������������ʱ��
  * @param  timer    ��ʱ��ָ�룬��MultiTimer���Զ�����
  * @param  userData �û�����ָ�루������δʹ�ã���ΪNULL��
  * @retval None
  */
void Timer2_Callback(MultiTimer *timer, void *userData)
{
    //! ���Դ���
    const char *msg = "0123456789_App_Version:0.0.0.2";
    uint16_t status = USART1_Put_TxData_To_Ringbuffer(msg, strlen(msg));
    
    //! ����LED
    LL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
    
    //! ����������ʱ��(500ms)
    MultiTimerStart(timer, 500, Timer2_Callback, NULL);
}

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
