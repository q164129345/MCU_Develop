/**
 * @file    myTim6Drive_reg.h
 * @brief   基于寄存器的 TIM6 驱动接口与寄存器定义
 *
 * 本文件提供 STM32F1 系列 MCU 的 TIM6 基础定时器底层寄存器访问定义、
 * 配置参数以及初始化和控制函数原型。通过直接操作寄存器，实现对 TIM6
 * 外设的高效、精细化控制。
 *
 * @note
 * - 该驱动不依赖 HAL/LL 库，完全通过寄存器位操作完成时钟使能、
 *   预分频、计数器模式等配置。
 * - 使用前需先在 RCC->APB1ENR 中使能 TIM6 时钟。
 *
 * @version 1.0.0
 * @date    2025-04-29
 * @author  Wallace.zhang
 *
 * @copyright
 * (C) 2025 Wallace.zhang。保留所有权利。
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


