/**
 * @file    app_jump.h
 * @brief   应用程序跳转模块头文件
 * @author  Wallace.zhang
 * @date    2025-06-27
 * @version 1.0.0
 * 
 * @details
 * 本模块负责Bootloader到应用程序的跳转控制，主要功能包括：
 * - 固件升级标志位的读取和设置
 * - 安全的应用程序跳转流程
 * - MCU环境清理和复位控制
 * - 中断向量表重定位
 * 
 * @note
 * 跳转流程采用"软复位+标志再入"方案：
 * 1. 第一次调用设置标志位并复位MCU
 * 2. 第二次调用清除标志位并跳转到App
 * 这种方案确保了硬件环境的完全清理，提高了跳转的可靠性。
 * 
 * @attention
 * - 固件升级标志变量存储在指定RAM地址，需与App程序保持一致
 * - 跳转前会自动处理MSP设置、特权级切换、中断向量表重定位等
 * - 跳转函数执行后不会返回，若跳转失败将进入死循环保护
 * 
 * @copyright
 * (C) 2025 Wallace.zhang. 保留所有权利.
 * @license SPDX-License-Identifier: MIT
 */

#ifndef __APP_JUMP_H
#define __APP_JUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/**
  * @brief  获取固件升级标志位
  */
uint64_t IAP_GetUpdateFlag(void);

/**
  * @brief  设置固件升级标志位
  */
void IAP_SetUpdateFlag(uint64_t flag);

/**
  * @brief  准备跳转到App程序
  */
void IAP_Ready_To_Jump_App(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_JUMP_H */

