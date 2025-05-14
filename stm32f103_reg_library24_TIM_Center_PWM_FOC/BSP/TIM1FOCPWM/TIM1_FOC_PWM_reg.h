/**
 * @file    TIM1_FOC_PWM_reg.h
 * @brief   ���ڼĴ����� TIM1 �߼���ʱ�������ӿ���Ĵ������壬���� FOC ����
 *
 * ���ļ��ṩ STM32F1 ϵ�� MCU �� TIM1 �߼���ʱ���ײ�Ĵ������ʶ��塢
 * ���ò����Լ���ʼ���Ϳ��ƺ���ԭ�ͣ�֧�����Ķ���ģʽ1�� PWM ģʽ2��
 * ������������ʱ�������� FOC �㷨�ԶԳ� PWM ���ε��ϸ�Ҫ��
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
#ifndef __TIM1_FOC_PWM_REG_H
#define __TIM1_FOC_PWM_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
  * @brief  ���� TIM1����ʼ��� PWM
  */
void TIM1_PWM_Start(void);

/**
  * @brief  ֹͣ TIM1 PWM ���
  */
void TIM1_PWM_Stop(void);

/**
  * @brief      ���� TIM1 ͨ�� 1/2/3 �ıȽ�ֵ�������������¼�ʹ��ֵ������Ч
  */
void TIM1_Set_Compare_Value(uint32_t CH1CompareValue, uint32_t CH2CompareValue, uint32_t CH3CompareValue);

/**
  * @brief  ���ڼĴ�����ʼ�� TIM1 ���� 3 · PWM ��������
  */
void TIM1_PWM_Init(uint16_t period, uint16_t duty1, uint16_t duty2, uint16_t duty3);

#ifdef __cplusplus
}
#endif

#endif /* __TIM1_FOC_PWM_REG_H */


