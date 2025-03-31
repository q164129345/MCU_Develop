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

volatile uint8_t txmail_free = 0;
volatile uint32_t canSendError = 0;

/**
  * @brief  使用直接寄存器操作的CAN初始化
  * @note   BTR解析:
  *         - SILM=0, LBKM=0  => 不使用静默/回环
  *         - SJW=0          => SJW=1Tq
  *         - TS2=0x02       => BS2=2 => 3Tq
  *         - TS1=0x0D       => BS1=13 => 14Tq
  *         - BRP=0x03       => 分频=4
  *         同时关闭自动重传(NART=1), 其他自动功能Disable
  */
void CAN_Config(void)
{
    /* 1. 使能 CAN1 和 GPIOA 时钟 */
    RCC->APB1ENR |= (1UL << 25);  // 使能 CAN1 时钟
    RCC->APB2ENR |= (1UL << 2);   // 使能 GPIOA 时钟

    /* 2. 配置 PA11 (CAN_RX) 为上拉输入、PA12 (CAN_TX) 为复用推挽输出 */
    // PA11: CRH[15:12], MODE=00, CNF=10 (上拉输入)
    GPIOA->CRH &= ~(0xF << 12);
    GPIOA->CRH |=  (0x8 << 12);
    GPIOA->ODR |=  (1UL << 11); // 上拉

    // PA12: CRH[19:16], MODE=11(50MHz), CNF=10(复用推挽)
    GPIOA->CRH &= ~(0xF << 16);
    GPIOA->CRH |=  (0xB << 16);

    /* 3. 退出 SLEEP 模式（若处于睡眠状态） */
    if (CAN1->MSR & (1UL << 1)) { // 检查 MSR.SLAK 是否为 1
        CAN1->MCR &= ~(1UL << 1);  // 清除 SLEEP 位
        while (CAN1->MSR & (1UL << 1));  // 等待 MSR.SLAK 变 0
    }

    /* 4. 进入初始化模式 */
    CAN1->MCR |= (1UL << 0);          // 请求进入 INIT 模式 (MCR.INRQ=1)
    while (!(CAN1->MSR & (1UL << 0))); // 等待 MSR.INAK 变 1

    /* 5. 关闭 TTCM/ABOM/AWUM/RFLM/TXFP(均为 Disable) */
    CAN1->MCR &= ~(1UL << 7);  // 关闭 TTCM（时间触发模式）
    CAN1->MCR &= ~(1UL << 6);  // 关闭 ABOM（自动离线管理模式）
    CAN1->MCR &= ~(1UL << 5);  // 关闭 AWUM（软件自动唤醒）
    CAN1->MCR &= ~(1UL << 3);  // 关闭 RFLM（FIFO 溢出时覆盖旧报文）
    CAN1->MCR &= ~(1UL << 2);  // 关闭 TXFP（发送 FIFO 优先级）

    /* 6. 关闭自动重传 (NART=1) */
    CAN1->MCR |= (1UL << 4);

    /*
    7. 设置 BTR=0x002D0003, 保持与 HAL 相同:
       - SJW=0  => 1Tq
       - TS2=0x02 => 2 => 3Tq
       - TS1=0x0D => 13 => 14Tq
       - BRP=0x03 => 3 => 分频=4
    */
    CAN1->BTR = (0x00 << 24) |  // SILM(31) | LBKM(30) = 0
                (0x00 << 22) |  // SJW(23:22) = 0 (SJW = 1Tq)
                (0x02 << 20) |  // TS2(22:20) = 2 (TS2 = 3Tq)
                (0x0D << 16) |  // TS1(19:16) = 13 (TS1 = 14Tq)
                (0x0003);       // BRP(9:0) = 3 (Prescaler = 4)

    /* 8. 退出初始化模式 */
    CAN1->MCR &= ~(1UL << 0);  // 清除 INRQ (进入正常模式)
    while (CAN1->MSR & (1UL << 0)); // 等待 MSR.INAK 变 0

    /* 9. 配置过滤器 0，FIFO0 */
    CAN1->FMR |= (1UL << 0);   // 进入过滤器初始化模式
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FFA1R &= ~(1UL << 0);  // 分配到 FIFO0
    CAN1->FA1R  |=  (1UL << 0);  // 激活过滤器 0
    CAN1->FMR   &= ~(1UL << 0);  // 退出过滤器初始化模式
    
    /* 10.CAN发送完成中断 */
    // 开启CAN发送完成的全局中断，并设置中断优先级
    NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),3, 0));
    NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
    CAN1->IER |= CAN_IER_TMEIE; // 发送邮箱空中断使能
    
    /* 11.CAN错误中断 */
    NVIC_SetPriority(CAN1_SCE_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),1, 0));
    NVIC_EnableIRQ(CAN1_SCE_IRQn);
    CAN1->IER |= CAN_IER_ERRIE; // 错误中断使能使能 
    CAN1->IER |= CAN_IER_EPVIE; // 错误被动中断使能
    CAN1->IER |= CAN_IER_EWGIE; // 错误警告中断使能
    CAN1->IER |= CAN_IER_LECIE; // 上一次错误号中断使能
    
    /* 12.获取一次发送邮箱的数量 */
    txmail_free = ((CAN1->TSR & CAN_TSR_TME0) ? 1 : 0) +
             ((CAN1->TSR & CAN_TSR_TME1) ? 1 : 0) +
             ((CAN1->TSR & CAN_TSR_TME2) ? 1 : 0);
}

/**
  * @brief  直接操作寄存器实现CAN消息发送(标准数据帧)，代码会阻塞，直到CAN消息被成功发送出去
  * @param  stdId: 标准ID(11位)
  * @param  data: 数据指针
  * @param  DLC: 数据长度(0~8)
  * @retval 0=发送成功; 1=无空闲邮箱; 2=超时/失败
  */
uint8_t CAN_SendMessage_Blocking(uint32_t stdId, uint8_t *data, uint8_t DLC)
{
    uint8_t mailbox;
    uint32_t timeout = 0xFFFF;
    /* 寻找空闲邮箱 */
    for(mailbox = 0; mailbox < 3; mailbox++) {
      if((CAN1->sTxMailBox[mailbox].TIR & (1UL << 0)) == 0)
         break;
    }
    if(mailbox >= 3)
        return 1; // 无空闲邮箱
    /* 清空该邮箱 */
    CAN1->sTxMailBox[mailbox].TIR  = 0;
    CAN1->sTxMailBox[mailbox].TDTR = 0;
    CAN1->sTxMailBox[mailbox].TDLR = 0;
    CAN1->sTxMailBox[mailbox].TDHR = 0;
    /* 标准ID写入TIR的[31:21]、标准帧, RTR=0, IDE=0 */
    CAN1->sTxMailBox[mailbox].TIR |= (stdId << 21);
    /* 配置DLC */
    CAN1->sTxMailBox[mailbox].TDTR = (DLC & 0x0F);  // 设置CAN报文的长度。使用&运算的目的是保证只有变量DLC的低四位写入TDTR寄存器，不会干涉到其他位。
    /* 填充数据 */
    if(DLC <= 4) {
        for(uint8_t i = 0; i < DLC; i++) {
          CAN1->sTxMailBox[mailbox].TDLR |= ((uint32_t)data[i]) << (8 * i);
        }
    } else {
        for(uint8_t i = 0; i < 4; i++) {
          CAN1->sTxMailBox[mailbox].TDLR |= ((uint32_t)data[i]) << (8 * i);
        }
        for(uint8_t i = 4; i < DLC; i++) {
          CAN1->sTxMailBox[mailbox].TDHR |= ((uint32_t)data[i]) << (8 * (i-4));
        }
    }
    /* 发起发送请求 */
    CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;
    /* 轮询等待TXRQ清零或超时 */
    while((CAN1->sTxMailBox[mailbox].TIR & CAN_TI0R_TXRQ) && --timeout);
    if(timeout == 0) {
        // 发送失败(无ACK或位错误), 返回2
        return 2;
    }
    return 0; // 发送成功
}

/**
  * @brief  直接操作寄存器实现CAN消息发送(标准数据帧)，代码不会阻塞
  * @param  stdId: 标准ID(11位)
  * @param  data: 数据指针
  * @param  DLC: 数据长度(0~8)
  * @retval 0=发送成功; 1=无空闲邮箱,没有发送数据
  */
uint8_t CAN_SendMessage_NonBlocking(uint32_t stdId, uint8_t *data, uint8_t DLC)
{
    uint8_t mailbox;
    if (txmail_free > 0) {
        /* 清空该邮箱并配置ID、DLC和数据 */
        CAN1->sTxMailBox[mailbox].TIR  = 0;
        CAN1->sTxMailBox[mailbox].TDTR = (DLC & 0x0F);
        CAN1->sTxMailBox[mailbox].TDLR = 0;
        CAN1->sTxMailBox[mailbox].TDHR = 0;
        CAN1->sTxMailBox[mailbox].TIR |= (stdId << 21);

        /* 填充数据 */
        for(uint8_t i = 0; i < DLC && i < 8; i++) {
            if(i < 4)
                CAN1->sTxMailBox[mailbox].TDLR |= ((uint32_t)data[i]) << (8 * i);
            else
                CAN1->sTxMailBox[mailbox].TDHR |= ((uint32_t)data[i]) << (8 * (i-4));
        }

        /* 发起发送请求并直接返回 */
        CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;
        txmail_free--;
        return 0; // 已发起发送请求，不等待完成
    } else {
        return 1; // 发送失败，因为发送邮箱满了
    }
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
      
    /* CAN发送 */
    uint8_t txData[8] = {0x11, 0x22, 0x33, 0x44,
                         0x55, 0x66, 0x77, 0x88};
    if (CAN_SendMessage_NonBlocking(0x123, txData, 8)) {
        canSendError++;  // 因为发送邮箱满了，所以当前消息发送失败
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
