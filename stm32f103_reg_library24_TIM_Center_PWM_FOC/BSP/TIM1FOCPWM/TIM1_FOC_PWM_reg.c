#include "TIM1FOCPWM/TIM1_FOC_PWM_reg.h"

/**
  * @brief  将 TIM1 完整 remap 到 PE7/PE8~PE15，并配置 PE8~PE13 为
  *         TIM1_CH1N/CH1, CH2N/CH2, CH3N/CH3
  */
static void TIM1_GPIO_Init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOE);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    /**TIM1 GPIO Configuration
        PE8     ------> TIM1_CH1N
        PE9     ------> TIM1_CH1
        PE10     ------> TIM1_CH2N
        PE11     ------> TIM1_CH2
        PE12     ------> TIM1_CH3N
        PE13     ------> TIM1_CH3
        */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_8|LL_GPIO_PIN_9|LL_GPIO_PIN_10|LL_GPIO_PIN_11
                              |LL_GPIO_PIN_12|LL_GPIO_PIN_13;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    LL_GPIO_AF_EnableRemap_TIM1();
}

/**
  * @brief  设置 TIM1 的死区时间（Dead-Time Generator）
  * @param  dtg 死区生成器值（0～255），对应 BDTR.DTG[7:0]
  *            - 当 dtg < 128 时，DeadTime ≈ dtg × T_DTS
  *            - 大于等于 128 则进入模式2/3，如需超长死区才用
  * @note   使用前请确保 TIM1_BDTR.MOE = 1，且定时器已使能时钟
  */
static void TIM1_SetDeadTime(uint8_t dtg)
{
    /* 1. 清除旧的 DTG 字段 */
    TIM1->BDTR &= ~TIM_BDTR_DTG;
    /* 2. 写入新的死区值 */
    TIM1->BDTR |= (uint32_t)dtg;
}

/**
  * @brief  触发定时器的更新事件（等价于向 EGR 寄存器写入 UG 位）
  * @note   EGR 寄存器写 1 触发事件，写 0 无效果，因此直接赋值或 SET_BIT 都可
  */
__STATIC_INLINE void TIM1_GenerateEvent_UPDATE(void)
{
    /* 方式一：直接赋值，触发 UG */
    TIM1->EGR = TIM_EGR_UG;
    
    /*  
    // 或者方式二：使用 CMSIS 宏置位
    SET_BIT(TIMx->EGR, TIM_EGR_UG);
    */
}

/**
  * @brief      设置 TIM1 通道 1/2/3 的比较值，并触发更新事件使新值立即生效
  * @param[in]  CH1CompareValue  通道 1 的比较值（写入 CCR1）
  * @param[in]  CH2CompareValue  通道 2 的比较值（写入 CCR2）
  * @param[in]  CH3CompareValue  通道 3 的比较值（写入 CCR3）
  * @note       写入 CCRx 后，需要调用 TIM1_GenerateEvent_UPDATE()  
  *             向 EGR 寄存器写入 UG 位，才能将预装载的比较值刷新到实际计数比较单元
  */
void TIM1_Set_Compare_Value(uint32_t CH1CompareValue, uint32_t CH2CompareValue, uint32_t CH3CompareValue)
{
    TIM1->CCR1 = CH1CompareValue;
    TIM1->CCR2 = CH2CompareValue;
    TIM1->CCR3 = CH3CompareValue;
    TIM1_GenerateEvent_UPDATE(); // 及时更新比较值
}

/**
  * @brief  启动 TIM1，开始输出 PWM
  * @note   会重新触发一次更新事件，确保预装载值写入立即生效；  
  *         并重新打开主输出，使 PWM 端口有效。
  */
void TIM1_PWM_Start(void)
{
    /* 1. 触发更新事件，刷新 ARR/CCR */
    TIM1->EGR = TIM_EGR_UG;
    /* 2. 使能主输出 */
    TIM1->BDTR |= TIM_BDTR_MOE;
    /* 3. 使能定时器计数 */
    TIM1->CR1 |= TIM_CR1_CEN;
}

/**
  * @brief  停止 TIM1 PWM 输出
  * @note   关闭计数并禁止主输出，确保 PWM 输出端口失效
  */
void TIM1_PWM_Stop(void)
{
    /* 1. 禁止定时器计数 */
    TIM1->CR1 &= ~TIM_CR1_CEN;
    /* 2. 禁用主输出 */
    TIM1->BDTR &= ~TIM_BDTR_MOE;
}

/**
  * @brief  基于寄存器初始化 TIM1 产生 3 路 PWM 互补波形
  * @param  period   定时器重装载值 (ARR)，决定 PWM 周期
  * @param  duty1    通道 1 的比较值 (CCR1)，决定占空比
  * @param  duty2    通道 2 的比较值 (CCR2)
  * @param  duty3    通道 3 的比较值 (CCR3)
  * @note   产生 PWM 模式 3，带互补输出，自动重载、比较寄存器均作预装载，高电平有效
  */
void TIM1_PWM_Init(uint16_t period,
                   uint16_t duty1,
                   uint16_t duty2,
                   uint16_t duty3)
{
    /* 1. 使能时钟：TIM1、GPIOE、AFIO */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    
    /* 2. 初始化GPIO */
    TIM1_GPIO_Init();
    
    /* 3. 时间基准配置：无分频，上升计数 */
    TIM1->PSC = 0;               /* PSC = 0，即分频 1 */
    TIM1->ARR = period;          /* 自动重载值 */
    TIM1->CR1 |= TIM_CR1_ARPE    /* ARR 预装载 */
               | TIM_CR1_CMS_0;  /* CMS = 01：中心对齐模式1 */

    /* 4. PWM 模式2 + 预装载 (OCxM = 111, OCxPE = 1) */
    TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC1PE
                   | TIM_CCMR1_OC2M | TIM_CCMR1_OC2PE);
    TIM1->CCMR1 |=  TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE
                 | TIM_CCMR1_OC2M_0 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE;

    TIM1->CCMR2 &= ~(TIM_CCMR2_OC3M | TIM_CCMR2_OC3PE);
    TIM1->CCMR2 |=  TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3PE;

    /* 5. 写入比较值 */
    TIM1->CCR1 = duty1;
    TIM1->CCR2 = duty2;
    TIM1->CCR3 = duty3;
    
    /* 6. 清除所有极性位，高电平有效 */
    TIM1->CCER &= ~(
         TIM_CCER_CC1P | TIM_CCER_CC1NP
       | TIM_CCER_CC2P | TIM_CCER_CC2NP
       | TIM_CCER_CC3P | TIM_CCER_CC3NP
    );
    
    /* 7. 使能通道输出及互补输出 */
    TIM1->CCER |= TIM_CCER_CC1E  | TIM_CCER_CC1NE
                | TIM_CCER_CC2E  | TIM_CCER_CC2NE
                | TIM_CCER_CC3E  | TIM_CCER_CC3NE;
    
    /* 8. 设置死区时间（100 个定时器时钟周期，大约 1.39μs） */
    TIM1_SetDeadTime(100);
    
    /* 9. 主输出使能（BDTR 寄存器的 MOE 位）*/
    TIM1->BDTR |= TIM_BDTR_MOE;

    /* 10. 触发一次更新事件，立即把 ARR/CCR/CCMR1 等预装载寄存器写入工作寄存器 */
    TIM1_GenerateEvent_UPDATE();
}


