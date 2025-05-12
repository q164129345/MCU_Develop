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
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_complete = 0;
volatile uint16_t recvd_length = 0;

volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
volatile uint8_t tx_dma_busy = 0;
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
#if (USE_LL_LIBRARY == 0)
// ����ʱ��
__STATIC_INLINE void Enable_Peripherals_Clock(void) {
    SET_BIT(RCC->APB2ENR, 1UL << 0UL);  // ����AFIOʱ��  // һ��Ĺ��̶�Ҫ��
    SET_BIT(RCC->APB1ENR, 1UL << 28UL); // ����PWRʱ��   // һ��Ĺ��̶�Ҫ��
    SET_BIT(RCC->APB2ENR, 1UL << 5UL);  // ����GPIODʱ�� // ����ʱ��
    SET_BIT(RCC->APB2ENR, 1UL << 2UL);  // ����GPIOAʱ�� // SWD�ӿ�
    __NOP(); // ��΢��ʱһ����
}

// ����DMA1��ͨ��4����ͨģʽ���ڴ浽����(flash->USART1_TX)�����ȼ��ߣ��洢����ַ���������ݴ�С����8bit
__STATIC_INLINE void DMA1_Channel4_Configure(void) {
    // ����ʱ��
    RCC->AHBENR |= (1UL << 0UL); // ����DMA1ʱ��
    // ���ò�����ȫ���ж�
    NVIC_SetPriority(DMA1_Channel4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),2, 0));
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    // ���ݴ��䷽��
    DMA1_Channel4->CCR &= ~(1UL << 14UL); // ���赽�洢��ģʽ
    DMA1_Channel4->CCR |= (1UL << 4UL); // DIRλ����1���ڴ浽���裩
    // ͨ�����ȼ�
    DMA1_Channel4->CCR &= ~(3UL << 12UL); // ���PLλ
    DMA1_Channel4->CCR |= (2UL << 12UL);  // PLλ����10�����ȼ���
    // ѭ��ģʽ
    DMA1_Channel4->CCR &= ~(1UL << 5UL);  // ���CIRCλ���ر�ѭ��ģʽ
    // ����ģʽ
    DMA1_Channel4->CCR &= ~(1UL << 6UL);  // ����洢��ַ������
    DMA1_Channel4->CCR |= (1UL << 7UL);   // �洢����ַ����
    // ���ݴ�С
    DMA1_Channel4->CCR &= ~(3UL << 8UL);  // �������ݿ��8λ�����PSIZEλ���൱��00
    DMA1_Channel4->CCR &= ~(3UL << 10UL); // �洢�����ݿ��8λ�����MSIZEλ���൱��00
    // �ж�
    DMA1_Channel4->CCR |= (1UL << 1UL);   // ������������ж�
}


__STATIC_INLINE void USART1_Configure(void) {
    /* 1. ʹ������ʱ�� */
    // RCC->APB2ENR �Ĵ������� APB2 ����ʱ��
    RCC->APB2ENR |= (1UL << 14UL); // ʹ�� USART1 ʱ�� (λ 14)
    RCC->APB2ENR |= (1UL << 2UL);  // ʹ�� GPIOA ʱ�� (λ 2)
    /* 2. ���� GPIO (PA9 - TX, PA10 - RX) */
    // GPIOA->CRH �Ĵ������� PA8-PA15 ��ģʽ������
    // PA9: ����������� (ģʽ: 10, CNF: 10)
    GPIOA->CRH &= ~(0xF << 4UL);        // ���� PA9 ������λ (λ 4-7)
    GPIOA->CRH |= (0xA << 4UL);         // PA9: 10MHz ����������� (MODE9 = 10, CNF9 = 10)
    // PA10: �������� (ģʽ: 00, CNF: 01)
    GPIOA->CRH &= ~(0xF << 8UL);        // ���� PA10 ������λ (λ 8-11)
    GPIOA->CRH |= (0x4 << 8UL);         // PA10: ����ģʽ���������� (MODE10 = 00, CNF10 = 01)
    // ����USART1ȫ���ж�
    NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0)); // ���ȼ�1�����ȼ�Խ���൱��Խ���ȣ�
    NVIC_EnableIRQ(USART1_IRQn);
    /* 3. ���� USART1 ���� */
    // (1) ���ò����� 115200 (ϵͳʱ�� 72MHz, ������ 16)
    // �����ʼ���: USART_BRR = fPCLK / (16 * BaudRate)
    // 72MHz / (16 * 115200) = 39.0625
    // ��������: 39 (0x27), С������: 0.0625 * 16 = 1 (0x1)
    USART1->BRR = (39 << 4UL) | 1;      // BRR = 0x271 (39.0625)
    // (2) ��������֡��ʽ (USART_CR1 �� USART_CR2)
    USART1->CR1 &= ~(1UL << 12UL);      // M λ = 0, 8 λ����
    USART1->CR2 &= ~(3UL << 12UL);      // STOP λ = 00, 1 ��ֹͣλ
    USART1->CR1 &= ~(1UL << 10UL);      // û��żУ��
    // (3) ���ô��䷽�� (�շ�˫��)
    USART1->CR1 |= (1UL << 3UL);        // TE λ = 1, ʹ�ܷ���
    USART1->CR1 |= (1UL << 2UL);        // RE λ = 1, ʹ�ܽ���
    // (4) ����Ӳ������ (USART_CR3)
    USART1->CR3 &= ~(3UL << 8UL);       // CTSE �� RTSE λ = 0, ������
    // (5) �����첽ģʽ (����޹�ģʽλ)
    USART1->CR2 &= ~(1UL << 14UL);      // LINEN λ = 0, ���� LIN ģʽ
    USART1->CR2 &= ~(1UL << 11UL);      // CLKEN λ = 0, ����ʱ�����
    USART1->CR3 &= ~(1UL << 5UL);       // SCEN λ = 0, �������ܿ�ģʽ
    USART1->CR3 &= ~(1UL << 1UL);       // IREN λ = 0, ���� IrDA ģʽ
    USART1->CR3 &= ~(1UL << 3UL);       // HDSEL λ = 0, ���ð�˫��
    // (6) �ж�
    USART1->CR1 |= (1UL << 4UL);        // ʹ��USART1�����ж� (IDLEIE, λ4)
    // (7) ����DMA�Ľ�������
    USART1->CR3 |= (1UL << 6UL);        // ʹ��USART1��DMA��������DMAR��λ6�� 
    // (7) ���� USART
    USART1->CR1 |= (1UL << 13UL);       // UE λ = 1, ���� USART
}

// ����USART1_RX��DMA1ͨ��5
__STATIC_INLINE void DMA1_Channel5_Configure(void) {
    // ʱ��
    RCC->AHBENR |= (1UL << 0UL); // ����DMA1ʱ��
    // ����ȫ���ж�
    NVIC_SetPriority(DMA1_Channel5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    /* 1. ����DMAͨ��5���ȴ�����ȫ�ر� */
    DMA1_Channel5->CCR &= ~(1UL << 0);  // ���ENλ
    while(DMA1_Channel5->CCR & 1UL);    // �ȴ�DMAͨ��5�ر�

    /* 2. ���������ַ�ʹ洢����ַ */
    DMA1_Channel5->CPAR = (uint32_t)&USART1->DR;  // �����ַΪUSART1���ݼĴ���
    DMA1_Channel5->CMAR = (uint32_t)rx_buffer;    // �洢����ַΪrx_buffer
    DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;        // �������ݳ���Ϊ��������С
    /* 3. ����DMA1ͨ��5 CCR�Ĵ���
         - DIR   = 0��������洢��
         - CIRC  = 1��ѭ��ģʽ
         - PINC  = 0�������ַ������
         - MINC  = 1���洢����ַ����
         - PSIZE = 00���������ݿ��8λ
         - MSIZE = 00���洢�����ݿ��8λ
         - PL    = 10�����ȼ���Ϊ��
         - MEM2MEM = 0���Ǵ洢�����洢��ģʽ
    */
    DMA1_Channel5->CCR = 0;  // ���֮ǰ������
    DMA1_Channel5->CCR |= (1UL << 5);       // ʹ��ѭ��ģʽ (CIRC��bit5)
    DMA1_Channel5->CCR |= (1UL << 7);       // ʹ�ܴ洢������ (MINC��bit7)
    DMA1_Channel5->CCR |= (3UL << 12);      // �������ȼ�Ϊ�ǳ��� (PL��Ϊ��11����bit12-13)
    
    // ���Ӵ�������봫������ж�
    DMA1_Channel5->CCR |= (1UL << 1);             // ��������ж� (TCIE)
    DMA1_Channel5->CCR |= (1UL << 2);             // ��������ж� (HTIE)
    /* 4. ʹ��DMAͨ��5 */
    DMA1_Channel5->CCR |= 1UL;  // ��ENλ����ͨ��
}

void USART1_SendString_DMA(const char *data, uint16_t len)
{
    if (len == 0 || len > TX_BUFFER_SIZE) { // ������DMA����0���ֽڣ��ᵼ��û�취���뷢������жϣ�Ȼ�������������
        return;
    }
    // �ȴ���һ��DMA������ɣ�Ҳ������ӳ�ʱ���ƣ�
    while(tx_dma_busy);
    tx_dma_busy = 1; // ���DMA���ڷ���
    memcpy((uint8_t*)tx_buffer, data, len);
    // ���DMAͨ��4����ʹ�ܣ����Ƚ����Ա���������
    if(DMA1_Channel4->CCR & 1UL) { // ���ENλ��bit0���Ƿ���λ
        DMA1_Channel4->CCR &= ~1UL;  // ����DMAͨ��4�����ENλ��
        while(DMA1_Channel4->CCR & 1UL); // �ȴ�DMAͨ��4��ȫ�ر�
    }
    // ����DMAͨ��4���ڴ��ַ�������ַ�����ݴ��䳤��
    DMA1_Channel4->CMAR  = (uint32_t)data;         // �����ڴ��ַ
    DMA1_Channel4->CPAR  = (uint32_t)&USART1->DR;  // ���������ַ
    DMA1_Channel4->CNDTR = len;                    // ���ô������ݳ���
    // ����USART1��DMA��������CR3��DMAT����7λ����1
    USART1->CR3 |= (1UL << 7UL);
    // ����DMAͨ��4������ENλ����DMA����
    DMA1_Channel4->CCR |= 1UL;
}

#else

// ����DMA1ͨ��5����USART1_RX
__STATIC_INLINE void DMA1_Channel5_Configure(void) {
    /* ����DMA1ͨ��5����USART1_RX���� */
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)rx_buffer);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)&USART1->DR);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
}

/**
  * @brief  ʹ��DMA�����ַ���������USART1_TX��Ӧ��DMA1ͨ��4
  * @param  data: ����������ָ�루����ָ��������ͻ�������
  * @param  len:  ���������ݳ���
  * @retval None
  */
void USART1_SendString_DMA(const char *data, uint16_t len)
{
    if (len == 0 || len > TX_BUFFER_SIZE) { // ������DMA����0���ֽڣ��ᵼ��û�취���뷢������жϣ�Ȼ�������������
        return;
    }
    // �ȴ���һ��DMA�������
    while(tx_dma_busy);
    tx_dma_busy = 1; // ���DMA���ڷ���
    memcpy((uint8_t*)tx_buffer, data, len); // ����Ҫ���͵����ݷ���DMA���ͻ�����
    
    // ���DMAͨ��4����ʹ�ܣ����Ƚ����Ա���������
    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4));
    }
    // ����DMAͨ��4���ڴ��ַ�������ַ�����ݴ��䳤��
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)data);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)&USART1->DR);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, len);
    // ����USART1��DMA��������CR3��DMAT��1��ͨ��Ϊ��7λ��
    LL_USART_EnableDMAReq_TX(USART1); // ����USART1��DMA��������
    // ����DMAͨ��4����ʼDMA����
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
}
#endif

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
#if (USE_LL_LIBRARY == 0)
  Enable_Peripherals_Clock(); // �������������ʱ��
#endif
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
  SysTick->CTRL |= 0x01UL << 1UL; // ����SysTick�ж�
#if (USE_LL_LIBRARY == 1)
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
#else
  USART1_Configure(); // USART1
  DMA1_Channel4_Configure(); // DMA1ͨ��4
#endif
  DMA1_Channel5_Configure(); // DMA1ͨ��5
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (rx_complete) {
        USART1_SendString_DMA((const char*)tx_buffer, recvd_length);
        rx_complete = 0; // �����־���ȴ���һ�ν���
    }
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
