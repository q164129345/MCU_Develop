#include "myTIM6Drive/myTIM6Drive_reg.h"
#include "myUsartDrive/myUsartDrive_reg.h"

/**
  * @brief  TIM6 寄存器级初始化
  * @note   - 使能时钟：RCC->APB1ENR.TIM6EN (bit4)  
  *         - 设置预分频器 PSC = 71，对应 72 分频，使得定时器计数时钟 = Fpclk1/72  
  *         - 设置自动重装载 ARR = 999，溢出产生频率 = 时钟/1000 = 1 kHz  
  *         - 使能 ARR 预装载：CR1.ARPE (bit7)  
  * @retval None
  */
void TIM6_RegInit(void)
{
    /* 1. 使能 TIM6 时钟 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;  
    /* 2. 配置 PSC 和 ARR */
    TIM6->PSC = 71;      /* PSC 寄存器：计数时钟 = PCLK1/(PSC+1) */
    TIM6->ARR = 999;     /* ARR 寄存器：计数到 ARR 后产生更新事件 */
    /* 3. 使能 ARR 预装载 */
    TIM6->CR1 |= TIM_CR1_ARPE;  

    /* 4. 触发输出与主从模式保持默认（RESET/关闭） */
    /*    TIM6->CR2 默认 TRGO=000，MSM=0 */
}

/**
  * @brief  启动 TIM6 并使能更新中断
  * @note   - 清除可能的 UIF 残留标志：SR.UIF (bit0) 写 0 清除  
  *         - 使能更新中断：DIER.UIE (bit0) =1  
  *         - 通过 NVIC 使能 TIM6/DAC 中断  
  *         - 启动计数：CR1.CEN (bit0)=1  
  * @retval None
  */
void TIM6_RegStartWithIRQ(void)
{
    /* 1. 清除更新中断标志，避免残留中断立即触发 */
    TIM6->SR &= ~TIM_SR_UIF;
    /* 2. 使能更新中断 */
    TIM6->DIER |= TIM_DIER_UIE;
    /* 3. NVIC 配置：使能 TIM6_DAC 中断，优先级 PL=2, SP=0 */
    /*    CMSIS 提供的 NVIC_SetPriority 和 NVIC_EnableIRQ 也可用 */
    NVIC_SetPriority(TIM6_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),2, 0));
    NVIC_EnableIRQ(TIM6_IRQn);
    /* 4. 启动定时器 */
    TIM6->CR1 |= TIM_CR1_CEN;
}

/**
  * @brief  TIM6 更新中断服务函数
  * @note   如果使用 DAC，TIM6 与 DAC 共享中断，需同时判断
  */
void TIM6_IRQHandler(void)
{
    /* 检查 UIF 标志 (更新中断) */
    if (TIM6->SR & TIM_SR_UIF) {
        /* 清除 UIF 标志 */
        TIM6->SR &= ~TIM_SR_UIF;
        /* 在此添加 1 kHz 触发时的处理代码 */
        const char *msg = "Hello,World\n";
        uint16_t status = USART1_Put_TxData_To_Ringbuffer(msg, strlen(msg));
    }
}


























