/**
 * @file    TIM1_FOC_PWM_reg.h
 * @brief   基于寄存器的 TIM1 高级定时器驱动接口与寄存器定义，用于 FOC 控制
 *
 * 本文件提供 STM32F1 系列 MCU 的 TIM1 高级定时器底层寄存器访问定义、
 * 配置参数以及初始化和控制函数原型，支持中心对齐模式1与 PWM 模式2，
 * 并可设置死区时间以满足 FOC 算法对对称 PWM 波形的严格要求。
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
#ifndef __TIM1_FOC_PWM_REG_H
#define __TIM1_FOC_PWM_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
  * @brief  启动 TIM1，开始输出 PWM
  */
void TIM1_PWM_Start(void);

/**
  * @brief  停止 TIM1 PWM 输出
  */
void TIM1_PWM_Stop(void);

/**
  * @brief      设置 TIM1 通道 1/2/3 的比较值，并触发更新事件使新值立即生效
  */
void TIM1_Set_Compare_Value(uint32_t CH1CompareValue, uint32_t CH2CompareValue, uint32_t CH3CompareValue);

/**
  * @brief  基于寄存器初始化 TIM1 产生 3 路 PWM 互补波形
  */
void TIM1_PWM_Init(uint16_t period, uint16_t duty1, uint16_t duty2, uint16_t duty3);

#ifdef __cplusplus
}
#endif

#endif /* __TIM1_FOC_PWM_REG_H */


