/**
 * @file    TIM5PWMOutput_reg.h
 * @brief   基于寄存器的 TIM5 驱动接口与寄存器定义
 *
 * 本文件提供 STM32F1 系列 MCU 的 TIM5 基础定时器底层寄存器访问定义、
 * 配置参数以及初始化和控制函数原型。通过直接操作寄存器，实现对 TIM5
 * 外设的高效、精细化控制。
 *
 * @note
 * - 该驱动不依赖 HAL/LL 库，完全通过寄存器位操作完成时钟使能、
 *   预分频、计数器模式等配置。
 *
 * @version 1.0.0
 * @date    2025-05-13
 * @author  Wallace.zhang
 *
 * @copyright
 * (C) 2025 Wallace.zhang。保留所有权利。
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


