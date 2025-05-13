/**
 * @file    TIM5PWMOutput_reg.h
 * @brief   ���ڼĴ����� TIM5 �����ӿ���Ĵ�������
 *
 * ���ļ��ṩ STM32F1 ϵ�� MCU �� TIM5 ������ʱ���ײ�Ĵ������ʶ��塢
 * ���ò����Լ���ʼ���Ϳ��ƺ���ԭ�͡�ͨ��ֱ�Ӳ����Ĵ�����ʵ�ֶ� TIM5
 * ����ĸ�Ч����ϸ�����ơ�
 *
 * @note
 * - ������������ HAL/LL �⣬��ȫͨ���Ĵ���λ�������ʱ��ʹ�ܡ�
 *   Ԥ��Ƶ��������ģʽ�����á�
 *
 * @version 1.0.0
 * @date    2025-05-13
 * @author  Wallace.zhang
 *
 * @copyright
 * (C) 2025 Wallace.zhang����������Ȩ����
 *
 * @license SPDX-License-Identifier: MIT
 */
#ifndef __TIM5PWMOUTPUT_REG_H
#define __TIM5PWMOUTPUT_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void TIM5_PWM_Start(void);
void TIM5_PWM_Stop(void);
void TIM5_PWM_SetDuty(uint16_t ccr);
void TIM5_PWM_SetPeriod(uint16_t arr);
void TIM5_PWM_Init(uint16_t psc, uint16_t arr, uint16_t ccr);


#ifdef __cplusplus
}
#endif

#endif /* __TIM5PWMOUTPUT_REG_H */


