#include "TIM5PWMOutput/TIM5PWMOutput_reg.h"

/**
 * @brief  启动 TIM5 通道1 的 PWM 输出
 * @retval 无
 */
void TIM5_PWM_Start(void)
{
    /* 使能 CC1 输出 */
    TIM5->CCER |= TIM_CCER_CC1E;
    /* 使能定时器计数 */
    TIM5->CR1 |= TIM_CR1_CEN;
    /* 立即生效：触发更新事件，加载预装载寄存器 */
    TIM5->EGR |= TIM_EGR_UG;
}

/**
 * @brief  停止 TIM5 通道1 的 PWM 输出
 * @retval 无
 */
void TIM5_PWM_Stop(void)
{
    /* 禁用定时器计数 */
    TIM5->CR1 &= ~TIM_CR1_CEN;
    /* 禁用 CC1 输出 */
    TIM5->CCER &= ~TIM_CCER_CC1E;
}

/**
 * @brief  设置 PWM 占空比（通道1）
 * @param  ccr: 比较寄存器值，范围 0 ~ (ARR+1)
 * @retval 无
 */
void TIM5_PWM_SetDuty(uint16_t ccr)
{
    TIM5->CCR1 = ccr;
    /* 若需立即生效，可取消注释触发更新事件 */
    // TIM5->EGR |= TIM_EGR_UG;
}

/**
 * @brief  设置 PWM 周期（自动重装载寄存器）
 * @param  arr: 自动重装载寄存器值（周期 - 1）
 * @retval 无
 */
void TIM5_PWM_SetPeriod(uint16_t arr)
{
    TIM5->ARR = arr;
    /* 若已使能 ARR 预装载，则需要触发更新事件 */
    TIM5->EGR |= TIM_EGR_UG;
}

/**
 * @brief  配置并初始化 TIM5 通道1 PWM 输出（PA0 = TIM5_CH1）
 * @param  arr: 自动重装载寄存器值（周期 - 1）
 * @param  psc: 预分频器值
 * @retval 无
 */
void TIM5_PWM_Init(uint16_t psc, uint16_t arr, uint16_t ccr)
{
    /* 1. 使能时钟：TIM5（APB1）, GPIOA（APB2） */
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /* 2. 配置 PA0 为复用推挽输出 */
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
    GPIOA->CRL |=  (GPIO_CRL_MODE0_0   /* 输出模式，10 MHz */
                  | GPIO_CRL_CNF0_1);  /* 复用推挽 */

    /* 3. 配置 TIM5 基础计数 */
    TIM5->PSC  = psc;    /* 预分频 */
    TIM5->ARR  = arr;    /* 自动重装载 */
    /* 使能 ARR 预装载 */
    TIM5->CR1 |= TIM_CR1_ARPE;

    /* 4. 配置通道1 为 PWM1 模式 */
    TIM5->CCMR1 &= ~TIM_CCMR1_OC1M; // 清0
    TIM5->CCMR1 |=  (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1); /* PWM 模式1 */
    /* 使能 CCR1 预装载 */
    TIM5->CCMR1 |= TIM_CCMR1_OC1PE;
    /* 默认占空比 0 */
    TIM5->CCR1 = ccr;

    /* 5. 设置极性（高电平有效）并暂时禁用输出 */
    TIM5->CCER &= ~TIM_CCER_CC1P;
    TIM5->CCER &= ~TIM_CCER_CC1E;
}





