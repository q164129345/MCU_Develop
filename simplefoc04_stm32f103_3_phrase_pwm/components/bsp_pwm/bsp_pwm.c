#include "bsp_pwm.h"

/**
 * @brief 设置三相PWM的占空比
 * 
 * @param htimx 
 * @param dc_a 
 * @param dc_b 
 * @param dc_c 
 */
void _writeDutyCycle3PWM(TIM_HandleTypeDef *htimx, float dc_a, float dc_b, float dc_c)
{
    // 计算CCR值
    uint32_t ccr_a = (uint32_t)(dc_a * __HAL_TIM_GET_AUTORELOAD(htimx));
    uint32_t ccr_b = (uint32_t)(dc_b * __HAL_TIM_GET_AUTORELOAD(htimx));
    uint32_t ccr_c = (uint32_t)(dc_c * __HAL_TIM_GET_AUTORELOAD(htimx));

    // 设置PWM占空比
    __HAL_TIM_SET_COMPARE(htimx, TIM_CHANNEL_1, ccr_a);
    __HAL_TIM_SET_COMPARE(htimx, TIM_CHANNEL_2, ccr_b);
    __HAL_TIM_SET_COMPARE(htimx, TIM_CHANNEL_3, ccr_c);

}


























