#include "bsp_pwm.h"
/**
 * @brief STM32F103 三相PWM输出
 * 
 * @param dc_a A相占空比
 * @param dc_b B相占空比
 * @param dc_c C相占空比
 */
void _writeDutyCycle3PWM(float dc_a, float dc_b, float dc_c)
{
    // 计算CCR值
    uint32_t ccr_a = (uint32_t)(dc_a * __HAL_TIM_GET_AUTORELOAD(&htim2));
    uint32_t ccr_b = (uint32_t)(dc_b * __HAL_TIM_GET_AUTORELOAD(&htim2));
    uint32_t ccr_c = (uint32_t)(dc_c * __HAL_TIM_GET_AUTORELOAD(&htim2));

    // 设置PWM占空比
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr_a);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, ccr_b);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, ccr_c);

}


























