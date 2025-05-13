/**
 * @file    myTim6Drive_reg.h
 * @brief   ���ڼĴ����� TIM6 �����ӿ���Ĵ�������
 *
 * ���ļ��ṩ STM32F1 ϵ�� MCU �� TIM6 ������ʱ���ײ�Ĵ������ʶ��塢
 * ���ò����Լ���ʼ���Ϳ��ƺ���ԭ�͡�ͨ��ֱ�Ӳ����Ĵ�����ʵ�ֶ� TIM6
 * ����ĸ�Ч����ϸ�����ơ�
 *
 * @note
 * - ������������ HAL/LL �⣬��ȫͨ���Ĵ���λ�������ʱ��ʹ�ܡ�
 *   Ԥ��Ƶ��������ģʽ�����á�
 * - ʹ��ǰ������ RCC->APB1ENR ��ʹ�� TIM6 ʱ�ӡ�
 *
 * @version 1.0.0
 * @date    2025-04-29
 * @author  Wallace.zhang
 *
 * @copyright
 * (C) 2025 Wallace.zhang����������Ȩ����
 *
 * @license SPDX-License-Identifier: MIT
 */
#ifndef __MYTIM6DRIVE_REG_H
#define __MYTIM6DRIVE_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void TIM6_RegInit(void);
void TIM6_RegStartWithIRQ(void);




#ifdef __cplusplus
}
#endif

#endif /* __MYTIM6DRIVE_REG_H */


