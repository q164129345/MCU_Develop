/*
 * @brief 这个库用于STM32F103的三相PWM输出控制
 * 
 * 该库提供了一个函数，用于设置三相PWM的占空比。
 * 用户可以通过调用_writeDutyCycle3PWM函数，传入A、B、C三相的占空比，
 * 来控制PWM输出。
 * 
 * 主要功能：
 * - 设置三相PWM的占空比
 * 
 * 使用示例：
 * @code
 * _writeDutyCycle3PWM(0.5, 0.5, 0.5); // 设置A、B、C三相的占空比为50%
 * @endcode
 */
#ifndef _BSP_PWM_H_
#define _BSP_PWM_H_

#include "main.h"
#include "tim.h"

#ifdef __cplusplus
extern "C" {
#endif


// public
void _writeDutyCycle3PWM(TIM_HandleTypeDef *htimx, float dc_a, float dc_b, float dc_c);


// private


#ifdef __cplusplus
} 
#endif

#endif
